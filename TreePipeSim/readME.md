# First programming Assignment

#==============================================================================
 #                           In the Name of the Emperor

 Welcome to treePipe: an Imperial demonstration of hierarchical forking and
 process communication using pipes in C. In this code, each node of the
 Imperial battle formation (a process) relays data to its subordinate (child)
 processes for further analysis, as commanded by the Emperor.

 “We march for Macragge!”
 
#==============================================================================

#------------------------------------------------------------------------------
# Overview
#------------------------------------------------------------------------------

 treePipe is a C program that:
  1) Creates a tree of processes, each representing a node in the grand army.
  2) Uses pipes for inter-process communication (parent <-> child).
  3) Optionally spawns worker processes (left/right) to handle specific
     computation rituals.
  4) Passes computed results back up through the chain of command.

 Think of it as a demonstration of organized warfare in the name of the Emperor:
 each node handles its piece of the puzzle and delegates tasks to its
 subordinates. The program terminates once the final result (the grand outcome)
 is calculated and reported.

#------------------------------------------------------------------------------
# Requirements
#------------------------------------------------------------------------------

 - A Linux/Unix environment (because of fork, pipe, execvp, and wait).
 - A C compiler (e.g. gcc).
 - The "left" and "right" worker programs (binaries) to be placed in the same
   directory if you wish to have the full ritual calculations for left and right.
   These should be compiled separately from their respective .c files (not
   provided in this sample, but you can create your own to experiment).

#------------------------------------------------------------------------------
# Compilation
#------------------------------------------------------------------------------

 1) Save the main file as treePipe.c (or use whatever name you desire).
 2) Compile with:

     gcc -o treePipe treePipe.c

 3) Make sure the worker programs "left" and "right" are compiled and present:

     gcc -o left left.c
     gcc -o right right.c

 4) You should now have treePipe, left, and right as executables in the same folder.

#------------------------------------------------------------------------------
# Usage
#------------------------------------------------------------------------------

    ./treePipe <current depth> <max depth> <left-right>

 Example:
    ./treePipe 0 2 0

 This starts the tree at depth 0, instructs that the maximum depth we can go is 2,
 and that at this node we will use the "left" ritual in the worker program. (Though
 for the root, the <left-right> flag primarily affects which worker is used, but the
 code will also spawn subtrees accordingly.)

 The program will prompt you for an integer if it’s the root (current depth=0).
 That integer is then used to spawn further processes if depth < max_depth,
 and the data flows down the tree.

#------------------------------------------------------------------------------
# Explanation of Arguments
#------------------------------------------------------------------------------

 1) current depth:
    - The “level” of this node in the grand hierarchy (0 is the root).
    - Must be non-negative. For the root, use 0.

 2) max depth:
    - The maximum depth we are allowed to descend.
    - Must be >= current depth. If current depth < max depth, we spawn more child
      processes to the left and right (if indicated).

 3) left-right:
    - A flag (0 or 1) indicating which worker binary to use:
        * 0 => use ./left
        * 1 => use ./right
    - For the root, this determines how the worker node processes the user’s input.
    - For subtrees, the parent passes 0 or 1 as a command-line arg to indicate
      which worker the node should execute.

#------------------------------------------------------------------------------
# Workflow (Brief)
#------------------------------------------------------------------------------

 1) The root node (current depth = 0) asks the user for an integer (num1).
 2) It may fork a child subtree (left flank) if current_depth < max_depth.
    - That subtree receives num1 as input.
    - It then returns a result.
 3) The root then spawns a worker process (either ./left or ./right, depending on
    the left-right flag).
    - The worker receives two integers: (num1, left_result).
    - It returns the computed result (intermediate_result).
 4) The root may then fork another subtree (right flank), passing intermediate_result
    to it, if current_depth < max_depth.
 5) The final result is either the intermediate_result (if no right subtree) or
    whatever the right subtree computes.
 6) The process prints the final result and ends.
 7) Any non-root node follows a similar pattern, but it reads its num1 from stdin
    (provided by its parent), then spawns left subtree, spawns a worker, spawns right
    subtree, and sends the final result back to its own parent.

 Essentially, it’s a carefully orchestrated series of:
    fork -> pipe -> exec -> wait -> read -> pass result onward

 For the Emperor!

#------------------------------------------------------------------------------
# Additional Notes
#------------------------------------------------------------------------------

 - The program uses multiple pipes for each fork:
     1) A pipe to send data from the parent to the child.
     2) A pipe to read results back from the child.
 - Error handling is included to check for invalid arguments and for pipe/fork
   failures.
 - The code prints debug information to stderr so you can observe the process
   hierarchy in action. It also prints user prompts and final results to stdout.

#------------------------------------------------------------------------------
# Contribute
#------------------------------------------------------------------------------

 If you want to modify or extend this code:
  - Add your own custom worker programs (e.g., left.c or right.c) that read two
    integers from stdin and write a single integer result to stdout.
  - Adjust the computations in the parent if you’d like.
  - Experiment with deeper or shallower trees, or even different forms of
    inter-process communication.

#------------------------------------------------------------------------------
# For the Emperor!
#------------------------------------------------------------------------------

 That’s it. May your forking and piping be ever in the Emperor’s favor.

 In the grim darkness of the far future, there is only pipe-based IPC…

# End of README
