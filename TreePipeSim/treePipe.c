#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/*
 * In the name of the Emperor, we begin our sacred task.
 * Every process, like a battle-brother, must follow the orders precisely.
 */

//--------------------------
// Helper: Validate Arguments
//--------------------------
void validate_args(int argc, char *argv[],
                   int *current_depth, int *max_depth, int *calculation_program) {
    int error_num1, error_num2, flag;
    char extra;
    if (argc != 4) {
        printf("Usage: ./treePipe <current depth> <max depth> <left-right>\n");
        exit(1);
    }
    if (sscanf(argv[1], "%d%c", &error_num1, &extra) != 1 || error_num1 < 0) {
        printf("Usage: ./treePipe <current depth> <max depth> <left-right>\n");
        exit(1);
    }
    if (sscanf(argv[2], "%d%c", &error_num2, &extra) != 1 || error_num2 < 0) {
        printf("Usage: ./treePipe <current depth> <max depth> <left-right>\n");
        exit(1);
    }
    if (error_num2 < error_num1) {
        printf("Usage: ./treePipe <current depth> <max depth> <left-right>\n");
        exit(1);
    }
    if (sscanf(argv[3], "%d%c", &flag, &extra) != 1 || (flag != 0 && flag != 1)) {
        printf("Usage: ./treePipe <current depth> <max depth> <left-right>\n");
        exit(1);
    }
    *current_depth = error_num1;
    *max_depth = error_num2;
    *calculation_program = flag;
}

//--------------------------
// Helper: Fork a Subtree
//--------------------------
int fork_subtree(int current_depth, const char *max_depth_str, const char *child_flag, int input_value) {
    int fd_parent_to_child[2], fd_child_to_parent[2];
    if (pipe(fd_parent_to_child) == -1 || pipe(fd_child_to_parent) == -1) {
        perror("pipe");
        exit(1);
    }
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }
    if (pid == 0) { // Child process: a loyal servant of the Imperium
        close(fd_parent_to_child[1]);
        close(fd_child_to_parent[0]);
        dup2(fd_parent_to_child[0], STDIN_FILENO);
        dup2(fd_child_to_parent[1], STDOUT_FILENO);
        close(fd_parent_to_child[0]);
        close(fd_child_to_parent[1]);
        char new_depth[12];
        sprintf(new_depth, "%d", current_depth + 1);
        // Forward the orders to the next crusade.
        char *args[] = {"./treePipe", new_depth, (char *)max_depth_str, (char *)child_flag, NULL};
        execvp(args[0], args);
        perror("execvp subtree");
        exit(1);
    } else { // Parent process: the commanding officer
        close(fd_parent_to_child[0]);
        close(fd_child_to_parent[1]);
        // Dispatch the command.
        dprintf(fd_parent_to_child[1], "%d\n", input_value);
        close(fd_parent_to_child[1]);
        wait(NULL); // Await the outcome.
        char buffer[11]; // Use a buffer of size 11 as per guidelines.
        int n = read(fd_child_to_parent[0], buffer, 10);
        if (n < 0) { perror("read subtree"); exit(1); }
        buffer[n] = '\0';
        int result;
        sscanf(buffer, "%d", &result);
        close(fd_child_to_parent[0]);
        return result;
    }
}

//--------------------------
// Helper: Fork a Worker
//--------------------------
int fork_worker(int num1, int left_result, int calculation_program) {
    int fd_parent_to_worker[2], fd_worker_to_parent[2];
    if (pipe(fd_parent_to_worker) == -1 || pipe(fd_worker_to_parent) == -1) {
        perror("pipe");
        exit(1);
    }
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }
    if (pid == 0) { // Worker process: an instrument of the Omnissiah
        close(fd_parent_to_worker[1]);
        close(fd_worker_to_parent[0]);
        dup2(fd_parent_to_worker[0], STDIN_FILENO);
        dup2(fd_worker_to_parent[1], STDOUT_FILENO);
        close(fd_parent_to_worker[0]);
        close(fd_worker_to_parent[1]);
        if (calculation_program == 0) {
            // The left-branched ritual.
            char *args[] = {"./left", NULL};
            execvp(args[0], args);
        } else {
            // The right-branched rite.
            char *args[] = {"./right", NULL};
            execvp(args[0], args);
        }
        perror("execvp worker");
        exit(1);
    } else { // Parent: the high commander awaiting results.
        close(fd_parent_to_worker[0]);
        close(fd_worker_to_parent[1]);
        dprintf(fd_parent_to_worker[1], "%d\n", num1);
        dprintf(fd_parent_to_worker[1], "%d\n", left_result);
        close(fd_parent_to_worker[1]);
        wait(NULL); // Stand vigil until the worker returns.
        char buffer[11]; // Use a buffer of size 11 as per guidelines.
        int n = read(fd_worker_to_parent[0], buffer, 10);
        if (n < 0) { perror("read worker"); exit(1); }
        buffer[n] = '\0';
        int result;
        sscanf(buffer, "%d", &result);
        close(fd_worker_to_parent[0]);
        return result;
    }
}

//**************************
// Main
//**************************
int main (int argc, char *argv[])
{
    int current_depth, max_depth, calculation_program;
    validate_args(argc, argv, &current_depth, &max_depth, &calculation_program);

    char max_depth_str[12];
    sprintf(max_depth_str, "%d", max_depth);

    int num1;                // The number offered at this node.
    int left_result;         // The result from the left subtree.
    int intermediate_result; // The worker's computed result.
    int final_result;        // The ultimate result.
    int default_value = 1;   // Default if no subtree is summoned.

    // Print header for the current node.
    for (int i = 0; i < current_depth; i++)
        fprintf(stderr, "---");
    fprintf(stderr, "> current depth: %d, lr: %d\n", current_depth, calculation_program);

    // Step 1: Read num1.
    if (current_depth == 0) {
        printf("Please enter num1 for the root: ");
        scanf("%d", &num1);
    } else {
        scanf("%d", &num1);
    }
    for (int i = 0; i < current_depth; i++)
        fprintf(stderr, "---");
    fprintf(stderr, "> my num1 is: %d\n", num1);

    if (current_depth == 0) {
        // For the root, first dispatch the left flank.
        if (current_depth < max_depth) {
            left_result = fork_subtree(current_depth, max_depth_str, "0", num1);
        } else {
            left_result = default_value;
        }
        // Call the worker.
        intermediate_result = fork_worker(num1, left_result, calculation_program);
        // Only print the combined debug if a subtree was created.
        if (max_depth > current_depth) {
            for (int i = 0; i < current_depth; i++)
                fprintf(stderr, "---");
            fprintf(stderr, "> current depth: %d, lr: %d, my num1: %d, my num2: %d\n", current_depth, calculation_program, num1, left_result);
        }
        for (int i = 0; i < current_depth; i++)
            fprintf(stderr, "---");
        fprintf(stderr, "> my result is: %d\n", intermediate_result);
        // Now, command the right flank.
        if (current_depth < max_depth) {
            final_result = fork_subtree(current_depth, max_depth_str, "1", intermediate_result);
        } else {
            final_result = intermediate_result;
        }
        printf("The final result is: %d\n", final_result);
    } else {
        // For non-root nodes.
        if (current_depth < max_depth) {
            left_result = fork_subtree(current_depth, max_depth_str, "0", num1);
            for (int i = 0; i < current_depth; i++)
                fprintf(stderr, "---");
            fprintf(stderr, "> current depth: %d, lr: %d, my num1: %d, my num2: %d\n", current_depth, calculation_program, num1, left_result);
        } else {
            left_result = default_value;
        }
        intermediate_result = fork_worker(num1, left_result, calculation_program);
        for (int i = 0; i < current_depth; i++)
            fprintf(stderr, "---");
        fprintf(stderr, "> my result is: %d\n", intermediate_result);
        if (current_depth < max_depth) {
            final_result = fork_subtree(current_depth, max_depth_str, "1", intermediate_result);
        } else {
            final_result = intermediate_result;
        }
        // Report the result back to the commanding officer.
        printf("%d\n", final_result);
    }
    fflush(stdout);
    return 0;
}
