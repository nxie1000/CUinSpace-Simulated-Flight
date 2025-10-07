CUinSpace Simulated Flight

## Program Description

This project is a simulation of a fictional space system involving dynamic resource management and multithreading in C. The goal is to simulate systems that consume and produce resources while being managed by a central manager thread. The simulation architecture supports both single-threaded and multi-threaded execution using POSIX threads (`pthread`) and ensures thread safety with semaphores (`sem_t`).

**Features:**
- **Dynamic Memory Allocation:** Manual allocation and deallocation of complex structures and arrays.
- **Linked lists:** Event queue implemented as a priority-based linked list.
- **Multithreading:** Separate threads for each system and the manager.
- **Synchronization:** Shared data (resources and events) protected with binary semaphores to avoid race conditions.
- **Makefile:** A proper `Makefile` is included to build all components modularly and support incremental compilation.

## Building and Running
1. Ensure all source files, Makefile, and README are in the same directory.
2. Compile program using make:
    ```
    make
    ```

3. Execute the program:
    ```
    ./p2
    ```

4. Clean up all compiled files:
    ```
    make clean
    ```

### Prerequisites:
- A Unix-like environment (Linux, macOS, or WSL)
- `gcc` with pthread support
- `make`

To compile and run the program, follow these steps in a Linux terminal:

## Notes

\- Can modify #define PARAM_SPEED-MODIFIER to 1 to make the code run faster
\- Make sure to swap #define SINGLE_THREAD_MODE out when done with the single threading part
