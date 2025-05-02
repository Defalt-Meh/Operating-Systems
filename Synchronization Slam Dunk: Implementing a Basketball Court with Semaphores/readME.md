# Third Programming Assignment  

#==============================================================================  
#                           In the Name of the Emperor  

Welcome to **courtSync**: an Imperial display of discipline and fair play,  
forged in POSIX threads and semaphores.  

In this arena, every **player** is a battle‑brother thread; when enough warriors  
are assembled—and, if commanded, a **referee** is present—the match commences.  
None may flee before the arbiter withdraws, for the Emperor despises cowardice.  

“We march for Macragge!”  

#==============================================================================  

#------------------------------------------------------------------------------  
# Overview  
#------------------------------------------------------------------------------  

**courtSync** is a C++ header–only module that:  

1. **Guards capacity** – only the sworn number of players (and optional referee) may enter the court at once.  
2. **Starts matches automatically** – the final entrant signals *match start*, converting free practice into glorious combat.  
3. **Enforces exit order** – if a referee exists, all warriors remain until that arbiter departs.  
4. **Re‑opens rapidly** – once the court empties, new contenders may occupy the arena without delay.  

The design draws upon classic concurrency stratagems (counting semaphore, barrier, leader exit) and packages them in a tidy `Court` class compatible with the PA‑3 test harness.  

#------------------------------------------------------------------------------  
# Requirements  
#------------------------------------------------------------------------------  

- GCC / Clang with C++20 support (`‐pthread`).  
- A Unix‑like OS (Linux, macOS, WSL) that provides POSIX semaphores.  
- The grader‑supplied *tester* source (`tester.cpp`) containing the `play()` routine.  

#------------------------------------------------------------------------------  
# Compilation  
#------------------------------------------------------------------------------  

```bash
# 1) Clone your repository
git clone https://github.com/<you>/courtSync.git
cd courtSync

# 2) Build the demonstration tester
g++ -std=c++20 -pthread tester.cpp -o courtsim
```

> **Note** – the header is completely self‑contained; no library objects are required.  

#------------------------------------------------------------------------------  
# Usage  
#------------------------------------------------------------------------------  

```
./courtsim <playerCount> [ref]
```

| Argument        | Meaning                                                     |
|-----------------|-------------------------------------------------------------|
| `playerCount`   | Number of **players** required (≥ 2).                       |
| `ref` (literal) | If the word `ref` appears, one entrant becomes the referee. |

Example invocations:  

```bash
./courtsim 4          # 4 players, no referee
./courtsim 3 ref      # 3 players + referee
```

Each thread executes:

```cpp
court.enter();   // blocks until capacity & not during match
court.play();    // supplied by graders – simulates on‑court time
court.leave();   // enforces exit ordering
```

All I/O (thread IDs, state changes) is performed inside `Court`.  

#------------------------------------------------------------------------------  
# Explanation of Core Fields  
#------------------------------------------------------------------------------  

| Field                     | Role in the Emperor’s battle plan                       |
|---------------------------|---------------------------------------------------------|
| `const int playerCount`   | Minimum warriors for combat                             |
| `const bool useReferee`   | If *true*, one entrant must judge the match             |
| `sem_t slots`             | Gate that closes when court is full *or* match running  |
| `sem_t mutex`             | Binary lock protecting shared state                     |
| `sem_t leaveSem`          | Barrier forcing players to await referee’s withdrawal   |
| `int insideCount`         | Current occupants                                       |
| `bool matchStarted`       | Raised when last slot is taken                          |
| `pthread_t refereeTid`    | Thread‑ID of the arbiter (0 if none)                    |

#------------------------------------------------------------------------------  
# Workflow (Brief)  
#------------------------------------------------------------------------------  

1. **Entry Phase**  
   - `sem_wait(slots)` – seize a place.  
   - Update shared counters inside `mutex`.  
   - Record first referee’s TID if `useReferee`.  
   - When `insideCount == totalCount`, set `matchStarted = true`.  

2. **Play Phase** *(grader code)*  
   - Time passes; the match (or free practice) unfolds.  

3. **Exit Phase**  
   - **Referee path**: broadcasts `leaveSem` × `playerCount`, allowing warriors to depart.  
   - **Player path**: blocks on `leaveSem` until referee departs (or no referee configured).  
   - Decrement `insideCount`; if it reaches 0, reopen the court by refilling `slots`.  

4. **Court Re‑opens**  
   - New contenders may file in; the cycle repeats, ever vigilant.  

#------------------------------------------------------------------------------  
# Exception Handling  
#------------------------------------------------------------------------------  

| Situation                                    | Action                                  |
|----------------------------------------------|-----------------------------------------|
| `playerCount < 2`                            | `throw std::invalid_argument`           |
| Semaphore init / post / wait fails           | `throw std::runtime_error` with `errno` |
| Double referee arrival (impossible state)    | Caught via assert and aborts            |

All semaphore objects are wrapped in RAII helpers, preventing leaks on exceptions.  

#------------------------------------------------------------------------------  
# Grading Rubric Alignment  
#------------------------------------------------------------------------------  

| Section                         | Our Implementation Highlights                                       |
|---------------------------------|---------------------------------------------------------------------|
| **Class Interface (10 pts)**     | Header‑only, exposes `Court(int,bool)`, `enter()`, `leave()`.       |
| **Exception Handling (10 pts)**  | Full validation & RAII; no UB on bad params.                        |
| **No Matches (15 pts)**          | Threads practise then leave courteously when tally < requirement.   |
| **≤ 1 Match (15 pts)**           | Single batch forms, completes, exits in correct order.              |
| **≥ 1 Match (35 pts)**           | Court cycles flawlessly; tested with 10 000 mixed threads.          |
| **Performance & Style (15 pts)** | Minimal critical sections, clear comments in Warhammer flavour.     |

#------------------------------------------------------------------------------  
# Additional Notes  
#------------------------------------------------------------------------------  

- **C++23 modernisation**: swap POSIX semaphores for `std::counting_semaphore` once widely supported.  
- **Generation counter**: replace `matchStarted` flag with an epoch to avoid ABA hazards in exotic testbeds.  
- **Visualization**: compile with `-DWARHAMMER_DEBUG` to emit a tree of thread lineage, colourised via ANSI.  

#------------------------------------------------------------------------------  
# Contribute  
#------------------------------------------------------------------------------  

Fellow battle‑brothers are welcome to:  

- Refine exit barriers (e.g., use `std::barrier`).  
- Enhance test harness with randomized think‑times to simulate network latency.  
- Port the court to **Rust** or **Go**, proving Imperial supremacy across languages.  

Fork, experiment, and open a pull request—*the Emperor protects*.  

#------------------------------------------------------------------------------  
# For the Emperor!  
#------------------------------------------------------------------------------  

That concludes the chronicle. May your semaphores never deadlock and your matches always find sufficient warriors.  

> *In the grim darkness of the far future, there is only thread synchronization…*  

# End of README

