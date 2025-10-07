/*
    This is not a file you will need to touch. 
    It handles all of the display logic needed to print the state of the simulation.
    You can disable the fancy TUI mode by commenting out the TUI_MODE definition in defs.h
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include "defs.h"

#ifdef TUI_MODE
#define CLEAR_SCREEN() printf("\033[2J")
#define CLEAR_EVENT()  printf("%-128s", "")
#define MOVE_CURSOR(row, col) printf("\033[%d;%dH", (row), (col))
#define CLEAR_LINE() printf("\033[K")
#define MOVE_DOWN_ONE() printf("\033[1B")
#define MOVE_NEXT_LINE() printf("\033[1E")
#define SAVE_CURSOR() printf("\033[s")
#define RESTORE_CURSOR() printf("\033[u")
#define VBAR(col, nrows) for (int i = 0; i < nrows; i++) {  MOVE_CURSOR(i + 1, col);  printf("|"); }
#define HIDE_CURSOR() printf("\033[?25l")
#define SHOW_CURSOR() printf("\033[?25h")
#endif
#ifndef TUI_MODE
#define CLEAR_SCREEN() printf("\n")
#define CLEAR_EVENT() {}
#define MOVE_CURSOR(row, col) {}
#define CLEAR_LINE() printf("\n")
#define MOVE_DOWN_ONE() printf("\n")
#define MOVE_NEXT_LINE() printf("\n")
#define SAVE_CURSOR() {}
#define RESTORE_CURSOR() {}
#define VBAR(col, nrows) {}
#define HIDE_CURSOR() {}
#define SHOW_CURSOR() {}

#endif

// Add macros for colorized strings
#define STR_RED(text) "\033[31m" text "\033[0m"
#define STR_GREEN(text) "\033[32m" text "\033[0m"
#define STR_YELLOW(text) "\033[33m" text "\033[0m"
#define STR_BLUE(text) "\033[34m" text "\033[0m"
#define STR_MAGENTA(text) "\033[35m" text "\033[0m"
#define STR_CYAN(text) "\033[36m" text "\033[0m"

#define MAX_EVENTS_DISPLAYED 15
#define STATUS_WIDTH 36

static int N_DISPLAYED_EVENTS = 0;

static void display_resources(const Manager *manager) __attribute__((no_sanitize("thread")));
static void display_with_header(const Manager* manager) __attribute__((no_sanitize("thread")));
static void display_modes(const Manager *manager) __attribute__((no_sanitize("thread")));
static const char* display_get_event_str(const Event *event) __attribute__((no_sanitize("thread")));
static const char* display_get_mode_str(const System *system) __attribute__((no_sanitize("thread")));

void display_simulation_state(Manager *manager) {
    // Static values are allocated to the data segment, so they persist between function calls
    static const float display_interval = 0.1;
    static struct timeval prev = {0};
    struct timeval now;

    // If this is the first time we're displaying, clear the screen and print our headers
    if (prev.tv_sec == 0) {
        CLEAR_SCREEN();
    }
    
    // If it has not been long enough since our previous display refresh, keep waiting.
    gettimeofday(&now, NULL);
    float diff = (now.tv_sec - prev.tv_sec) + (now.tv_usec - prev.tv_usec) / 1000000.0;
    if (diff < display_interval) {
        return;
    }
    
    HIDE_CURSOR();
    display_with_header(manager);
    prev = now;

    fflush(stdout);
    SHOW_CURSOR();
}

void display_finish_sim() {
    // Move the cursor to the next line and print the result
    MOVE_CURSOR(MAX_EVENTS_DISPLAYED + 4, 1);
    printf("===================================\n");
    printf("Simulation Completed.              \n");
    printf("===================================\n");
    // SAVE_CURSOR();
    // for (int i = 0; i < 10; i++) {
    //     CLEAR_LINE();
    // }
    // RESTORE_CURSOR();
    fflush(stdout);
}

static void display_with_header(const Manager* manager) {
    MOVE_CURSOR(1, 1);
    puts("----------------------------------------------------------------------------------------");
    puts("Current Resource Amounts:                            Event Log" );
    puts("----------------------------------------------------------------------------------------");
    display_resources(manager);
    puts("");
    puts("-----------------------------------");
    puts("System Modes:");
    puts("-----------------------------------");
    display_modes(manager);

    VBAR(STATUS_WIDTH, MAX_EVENTS_DISPLAYED + 4);
    MOVE_CURSOR(1, STATUS_WIDTH);
}

static void display_modes(const Manager *manager) {
    for (int i = 0; i < manager->system_array.size; i++) {
        System *system = manager->system_array.systems[i];
        const char *mode_str = display_get_mode_str(system);
        printf("%-20s: %-s\n", system->name, mode_str);
    }
}

static void display_resources(const Manager *manager) {
    for (int i = 0; i < manager->resources.size; i++) {
        Resource *resource = manager->resources.resources[i];
        int current_amount;

        // Acquire the semaphore to read the resource amount safely
        sem_wait(&resource->mutex);
        current_amount = resource->amount;
        sem_post(&resource->mutex);

        MOVE_CURSOR(i + 4, 1);
        printf("%-20s: %4d / %4d\n", resource->name, current_amount, resource->max_capacity);
    }
}

void display_event(const Event *event) {
    HIDE_CURSOR();
    const char *status_str = display_get_event_str(event);

    MOVE_CURSOR(N_DISPLAYED_EVENTS % 15 + 4, STATUS_WIDTH + 2);
    CLEAR_EVENT();
    MOVE_CURSOR(N_DISPLAYED_EVENTS % 15 + 5, STATUS_WIDTH + 2);
    CLEAR_EVENT();
    MOVE_CURSOR(N_DISPLAYED_EVENTS % 15 + 4, STATUS_WIDTH + 2);

    N_DISPLAYED_EVENTS++;

    printf("Event [%04d]: [%s] Reported Resource [%s] Status [%s]\n",
        N_DISPLAYED_EVENTS,
        event->system->name,
        event->resource->name,
        status_str);

    SHOW_CURSOR();
}

static const char* display_get_event_str(const Event *event) {
    switch (event->status) {
        case EVENT_LOW:
            return STR_YELLOW("LOW");
        case EVENT_INSUFFICIENT:
            return STR_RED("INSUFFICIENT");
        case EVENT_CAPACITY:
            return STR_BLUE("CAPACITY");
        case EVENT_HIGH:
            return STR_GREEN("HIGH");
        case EVENT_PRODUCED:
            return "PRODUCED";
        default:
            return "UNKNOWN";
    }
}

static const char* display_get_mode_str(const System *system) {
    switch (system->mode) {
        case MODE_STANDARD:
            return "STANDARD";
        case MODE_SLOW:
            return "SLOW";
        case MODE_FAST:
            return "FAST";
        case MODE_TERMINATE:
            return "TERMINATE";
        default:
            return "UNKNOWN";
    }
}