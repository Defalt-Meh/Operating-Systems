#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm_dbg.h"

#define NOPS (16)

#define OPC(i) ((i) >> 12)
#define DR(i) (((i) >> 9) & 0x7)
#define SR1(i) (((i) >> 6) & 0x7)
#define SR2(i) ((i) & 0x7)
#define FIMM(i) ((i >> 5) & 01)
#define IMM(i) ((i) & 0x1F)
#define SEXTIMM(i) sext(IMM(i), 5)
#define FCND(i) (((i) >> 9) & 0x7)
#define POFF(i) sext((i) & 0x3F, 6)
#define POFF9(i) sext((i) & 0x1FF, 9)
#define POFF11(i) sext((i) & 0x7FF, 11)
#define FL(i) (((i) >> 11) & 1)
#define BR(i) (((i) >> 6) & 0x7)
#define TRP(i) ((i) & 0xFF)

/* New OS declarations */

// OS bookkeeping constants
#define PAGE_SIZE       (4096)  // Page size in bytes
#define OS_MEM_SIZE     (2)     // OS Region size. Also the start of the page tables' page
#define Cur_Proc_ID     (0)     // id of the current process
#define Proc_Count      (1)     // total number of processes, including ones that finished executing.
#define OS_STATUS       (2)     // Bit 0 shows whether the PCB list is full or not
#define OS_FREE_BITMAP  (3)     // Bitmap for free pages

// Process list and PCB related constants
#define PCB_SIZE  (3)  // Number of fields in a PCB
#define PID_PCB   (0)  // Holds the pid for a process
#define PC_PCB    (1)  // Value of the program counter for the process
#define PTBR_PCB  (2)  // Page table base register for the process

#define CODE_SIZE       (2)  // Number of pages for the code segment
#define HEAP_INIT_SIZE  (2)  // Number of pages for the heap segment initially

bool running = true;

typedef void (*op_ex_f)(uint16_t i);
typedef void (*trp_ex_f)();

enum { trp_offset = 0x20 };
enum regist { R0 = 0, R1, R2, R3, R4, R5, R6, R7, RPC, RCND, PTBR, RCNT };
enum flags { FP = 1 << 0, FZ = 1 << 1, FN = 1 << 2 };

uint16_t mem[UINT16_MAX] = {0};
uint16_t reg[RCNT] = {0};
uint16_t PC_START = 0x3000;

void initOS();
int createProc(char *fname, char *hname);
void loadProc(uint16_t pid);
uint16_t allocMem(uint16_t ptbr, uint16_t vpn, uint16_t read, uint16_t write);  // Can use 'bool' instead
int freeMem(uint16_t ptr, uint16_t ptbr);
static inline uint16_t mr(uint16_t address);
static inline void mw(uint16_t address, uint16_t val);
static inline void tbrk();
static inline void thalt();
static inline void tyld();
static inline void trap(uint16_t i);

static inline uint16_t sext(uint16_t n, int b) { return ((n >> (b - 1)) & 1) ? (n | (0xFFFF << b)) : n; }
static inline void uf(enum regist r) {
    if (reg[r] == 0)
        reg[RCND] = FZ;
    else if (reg[r] >> 15)
        reg[RCND] = FN;
    else
        reg[RCND] = FP;
}
static inline void add(uint16_t i)  { reg[DR(i)] = reg[SR1(i)] + (FIMM(i) ? SEXTIMM(i) : reg[SR2(i)]); uf(DR(i)); }
static inline void and(uint16_t i)  { reg[DR(i)] = reg[SR1(i)] & (FIMM(i) ? SEXTIMM(i) : reg[SR2(i)]); uf(DR(i)); }
static inline void ldi(uint16_t i)  { reg[DR(i)] = mr(mr(reg[RPC]+POFF9(i))); uf(DR(i)); }
static inline void not(uint16_t i)  { reg[DR(i)]=~reg[SR1(i)]; uf(DR(i)); }
static inline void br(uint16_t i)   { if (reg[RCND] & FCND(i)) { reg[RPC] += POFF9(i); } }
static inline void jsr(uint16_t i)  { reg[R7] = reg[RPC]; reg[RPC] = (FL(i)) ? reg[RPC] + POFF11(i) : reg[BR(i)]; }
static inline void jmp(uint16_t i)  { reg[RPC] = reg[BR(i)]; }
static inline void ld(uint16_t i)   { reg[DR(i)] = mr(reg[RPC] + POFF9(i)); uf(DR(i)); }
static inline void ldr(uint16_t i)  { reg[DR(i)] = mr(reg[SR1(i)] + POFF(i)); uf(DR(i)); }
static inline void lea(uint16_t i)  { reg[DR(i)] =reg[RPC] + POFF9(i); uf(DR(i)); }
static inline void st(uint16_t i)   { mw(reg[RPC] + POFF9(i), reg[DR(i)]); }
static inline void sti(uint16_t i)  { mw(mr(reg[RPC] + POFF9(i)), reg[DR(i)]); }
static inline void str(uint16_t i)  { mw(reg[SR1(i)] + POFF(i), reg[DR(i)]); }
static inline void rti(uint16_t i)  {} // unused
static inline void res(uint16_t i)  {} // unused
static inline void tgetc()        { reg[R0] = getchar(); }
static inline void tout()         { fprintf(stdout, "%c", (char)reg[R0]); }
static inline void tputs() {
  uint16_t *p = mem + reg[R0];
  while(*p) {
    fprintf(stdout, "%c", (char) *p);
    p++;
  }
}
static inline void tin()      { reg[R0] = getchar(); fprintf(stdout, "%c", reg[R0]); }
static inline void tputsp()   { /* Not Implemented */ }
static inline void tinu16()   { fscanf(stdin, "%hu", &reg[R0]); }
static inline void toutu16()  { fprintf(stdout, "%hu\n", reg[R0]); }

trp_ex_f trp_ex[10] = {tgetc, tout, tputs, tin, tputsp, thalt, tinu16, toutu16, tyld, tbrk};
static inline void trap(uint16_t i) { trp_ex[TRP(i) - trp_offset](); }
op_ex_f op_ex[NOPS] = {/*0*/ br, add, ld, st, jsr, and, ldr, str, rti, not, ldi, sti, jmp, res, lea, trap};

/**
  * Load an image file into memory.
  * @param fname the name of the file to load
  * @param offsets the offsets into memory to load the file
  * @param size the size of the file to load
*/
void ld_img(char *fname, uint16_t *offsets, uint16_t size) {
    FILE *in = fopen(fname, "rb");
    if (NULL == in) {
        fprintf(stderr, "Cannot open file %s.\n", fname);
        exit(1);
    }

    for (uint16_t s = 0; s < size; s += PAGE_SIZE) {
        uint16_t *p = mem + offsets[s / PAGE_SIZE];
        uint16_t writeSize = (size - s) > PAGE_SIZE ? PAGE_SIZE : (size - s);
        fread(p, sizeof(uint16_t), (writeSize), in);
    }
    
    fclose(in);
}

void run(char *code, char *heap) {
  while (running) {
    uint16_t i = mr(reg[RPC]++);
    op_ex[OPC(i)](i);
  }
}

// YOUR CODE STARTS HERE

// This function is used to get file size.
static uint16_t get_file_size(const char *fname) {
    FILE *file = fopen(fname, "rb");
    if (!file) {
        fprintf(stderr, "Cannot open file %s.\n", fname);
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    long bytes = ftell(file);
    fclose(file);
    // Divide by 2 (sizeof(uint16_t)) so ld_img reads
    // exactly that many 16-bit words, not bytes.
    return (uint16_t)(bytes / sizeof(uint16_t));
}


void initOS() {
    // Set curProcID to 0xffff, procCount to 0, OSStatus to 0x0000
    mem[0] = 0xffff;   // curProcID
    mem[1] = 0x0000;   // procCount
    mem[2] = 0x0000;   // OSStatus
    
    // Initialize bitmap for free pages (first 3 pages reserved for OS)
    mem[3] = 0b0001111111111111;  // First 16 pages (3 reserved)
    mem[4] = 0b1111111111111111;  // Next 16 pages
}

uint16_t allocMem(uint16_t ptbr, uint16_t vpn, uint16_t read, uint16_t write) {
    // Check if page is already allocated
    if ((mem[ptbr + vpn] & 0x0001) == 1) {
        printf("Cannot allocate memory for page %d of pid %d since it is already allocated.\n", vpn, mem[0]);
        return 0;
    }
    
    int16_t found = -1;
    uint16_t mask = 0b1000000000000000;

    // Search first 16 pages
    for (int i = 0; i < 16; i++) {
        if ((mem[3] & mask) >> (15 - i)) {
            found = i;
            mem[3] &= ~(0b1 << (15 - i));  // Mark as used
            break;
        }
        mask >>= 1;
    }

    // If not found, search next 16 pages
    if (found == -1) {
        mask = 0b1000000000000000;
        for (int i = 0; i < 16; i++) {
            if ((mem[4] & mask) >> (15 - i)) {
                found = i + 16;
                mem[4] &= ~(0b1 << (15 - i));  // Mark as used
                break;
            }
            mask >>= 1;
        }
    }

    if (found != -1) {
        // Create page table entry with appropriate permissions
        uint16_t entry = (found << 11);
        
        // Set permission bits
        if (read == UINT16_MAX) entry |= 0b10;
        if (write == UINT16_MAX) entry |= 0b100;
        entry |= 0b1;  // Valid bit
        
        // Update page table entry
        mem[ptbr + vpn] = entry;
        
        // Check if all pages are allocated
        if (found == 31) {
            mem[2] = 0x0001;  // Set OSStatus to indicate full
        }
        
        return found;
    }
    
    // No free page frames
    printf("Cannot allocate more space for pid %d since there is no free page frames.\n", mem[0]);
    return 0;
}

int freeMem(uint16_t vpn, uint16_t ptbr) {
    // Check if page is already free
    if ((mem[ptbr + vpn] & 0x0001) == 0) {
        return 0;
    }
    
    // Get physical frame number
    uint16_t pfn = mem[ptbr + vpn] >> 11;
    
    // Mark as free in appropriate bitmap
    if (pfn >= 16) {
        mem[4] |= (0b1 << (15 - (pfn - 16)));
    } else {
        mem[3] |= (0b1 << (15 - pfn));
    }
    
    // Clear valid bit (mark as invalid)
    mem[ptbr + vpn] &= 0b1111111111111110;
    
    // Set OSStatus to indicate space available
    mem[2] = 0x0000;
    
    return 1;
}
int createProc(char *fname, char *hname) {
    // Check if OS region is full
    if (mem[2] & 0b1) {
        printf("The OS memory region is full. Cannot create a new PCB.\n");
        running = 0;
        return 0;
    }

    uint16_t pid = mem[1];  // New process ID
    uint16_t pcb_index = 12 + pid * 3;
    uint16_t ptbr = 4096 + pid * 32;  // Page table base register

    // Initialize PCB
    mem[pcb_index] = pid;  // PID - Store the actual PID directly
    mem[pcb_index + 1] = 0x3000;  // PC
    mem[pcb_index + 2] = ptbr;    // PTBR

    // Allocate code and heap segments
    int16_t code_frame1 = allocMem(ptbr, 6, 0xffff, 0);  // Code is read-only
    if (code_frame1 == 0) {
        printf("Cannot create code segment.\n");
        return 0;
    }
    
    int16_t code_frame2 = allocMem(ptbr, 7, 0xffff, 0);
    if (code_frame2 == 0) {
        freeMem(6, ptbr);
        printf("Cannot create code segment.\n");
        return 0;
    }
    
    int16_t heap_frame1 = allocMem(ptbr, 8, 0xffff, 0xffff);  // Heap is read-write
    if (heap_frame1 == 0) {
        freeMem(6, ptbr);
        freeMem(7, ptbr);
        printf("Cannot create heap segment.\n");
        return 0;
    }
    
    int16_t heap_frame2 = allocMem(ptbr, 9, 0xffff, 0xffff);
    if (heap_frame2 == 0) {
        freeMem(6, ptbr);
        freeMem(7, ptbr);
        freeMem(8, ptbr);
        printf("Cannot create heap segment.\n");
        return 0;
    }

    // Load code and heap from files
    uint16_t code_offsets[2] = {code_frame1 * 2048, code_frame2 * 2048};
    uint16_t heap_offsets[2] = {heap_frame1 * 2048, heap_frame2 * 2048};
    uint16_t code_size = get_file_size(fname);
    uint16_t heap_size = get_file_size(hname);
    
    ld_img(fname, code_offsets, code_size);
    ld_img(hname, heap_offsets, heap_size);
    
    // Increment process count
    mem[1]++;
    return 1;
}

void loadProc(uint16_t pid) {
    mem[0] = pid;                      // Set current process ID
    reg[PTBR] = mem[12 + pid * 3 + 2]; // Load page table base register
    reg[RPC] = mem[12 + pid * 3 + 1];  // Load program counter
}

static inline uint16_t mr(uint16_t address) {
    uint16_t vpn = address >> 11;
    uint16_t offset = address & 0x07FF;  // Lower 11 bits are offset
    
    // Check if address is in reserved region
    if (vpn <= 5) {
        printf("Segmentation fault.\n");
        running = 0;
        return -1;
    }
    
    // Check if page is valid
    uint16_t pte = mem[reg[PTBR] + vpn];
    if ((pte & 0x0001) == 0) {
        printf("Segmentation fault inside free space.\n");
        running = 0;
        return -1;
    }
    
    // Check read permission
    if ((pte & 0x0002) == 0) {
        printf("Cannot read from a write-only page.\n");
        running = 0;
        return -1;
    }
    
    // Translate address and read value
    uint16_t pfn = pte >> 11;
    return mem[(pfn << 11) + offset];
}

static inline void mw(uint16_t address, uint16_t val) {
    uint16_t vpn = address >> 11;
    uint16_t offset = address & 0x07FF;  // Lower 11 bits are offset
    
    // Check if address is in reserved region
    if (vpn <= 5) {
        printf("Segmentation fault.\n");
        running = 0;
        return;
    }
    
    // Check if page is valid
    uint16_t pte = mem[reg[PTBR] + vpn];
    if ((pte & 0x0001) == 0) {
        printf("Segmentation fault inside free space.\n");
        running = 0;
        return;
    }
    
    // Check write permission
    if ((pte & 0x0004) == 0) {
        printf("Cannot write to a read-only page.\n");
        running = 0;
        return;
    }
    
    // Translate address and write value
    uint16_t pfn = pte >> 11;
    mem[(pfn << 11) + offset] = val;
}

static inline void tbrk() {
    uint16_t address = reg[R0];
    uint16_t vpn = address >> 11;
    uint16_t request = address & 0x0001;  // 1 for allocate, 0 for free
    
    if (request) {
        uint16_t read = (address & 0x0002) ? 0xffff : 0;
        uint16_t write = (address & 0x0004) ? 0xffff : 0;
        
        printf("Heap increase requested by process %d.\n", mem[0]);
        allocMem(reg[PTBR], vpn, read, write);
    } else {
        printf("Heap decrease requested by process %d.\n", mem[0]);
        if (!freeMem(vpn, reg[PTBR])) {
            printf("Cannot free memory of page %d of pid %d since it is not allocated.\n", vpn, mem[0]);
        }
    }
}

static inline void tyld() {
    uint16_t old_pid = mem[0];
    uint16_t new_pid = old_pid;
    
    // Save current process state
    mem[12 + old_pid * 3 + 1] = reg[RPC];
    
    // Find next runnable process
    do {
        new_pid = (new_pid + 1) % mem[1];
    } while (mem[12 + new_pid * 3] == 0xffff && new_pid != old_pid);
    
    // Set new process as current
    mem[0] = new_pid;
    
    // Load new process state
    reg[PTBR] = mem[12 + new_pid * 3 + 2];
    reg[RPC] = mem[12 + new_pid * 3 + 1];
    if (old_pid != new_pid) {
    printf("We are switching from process %d to %d.\n", old_pid, new_pid);
    }
}

static inline void thalt() {
    uint16_t current_pid = mem[0];
    uint16_t next_pid = current_pid;
    
    // Mark current process as terminated
    mem[12 + current_pid * 3] = 0xffff;
    
    // Free all pages allocated to current process
    uint16_t ptbr = mem[12 + current_pid * 3 + 2];
    for (int i = 0; i < 32; i++) {
        if (mem[ptbr + i] & 0x0001) {
            freeMem(i, ptbr);
        }
    }
    
    // Find next runnable process
    do {
        next_pid = (next_pid + 1) % mem[1];
        if (next_pid == current_pid) {
            // No more runnable processes
            running = 0;
            return;
        }
    } while (mem[12 + next_pid * 3] == 0xffff);
    
    // Set next process as current and load its state
    mem[0] = next_pid;
    reg[PTBR] = mem[12 + next_pid * 3 + 2];
    reg[RPC] = mem[12 + next_pid * 3 + 1];
}

// YOUR CODE ENDS HERE
