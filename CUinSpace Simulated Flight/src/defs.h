#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
// Multi-threading headers
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>

#define MODE_TERMINATE    0
#define MODE_DISABLED     1
#define MODE_SLOW         2
#define MODE_STANDARD     3
#define MODE_FAST         4

#define PRIORITY_HIGH   0xF000
#define PRIORITY_MED    0xA000
#define PRIORITY_LOW    0x8000
#define PRIORITY_IGN    0x0000 // Ignored priority level

#define EVENT_OK           (PRIORITY_IGN  | 0x0000)
#define EVENT_LOW          (PRIORITY_MED  | 0x0001)
#define EVENT_INSUFFICIENT (PRIORITY_HIGH | 0x0002)
#define EVENT_CAPACITY     (PRIORITY_MED  | 0x0003)
#define EVENT_HIGH         (PRIORITY_MED  | 0x0004)
#define EVENT_PRODUCED     (PRIORITY_IGN  | 0x0010)

#define PARAM_MANAGER_WAIT    10   // Milliseconds for the manager to wait between popping the queue
#define PARAM_SYSTEM_WAIT    500   // Milliseconds between loops of the system to prevent spamming with events
#define PARAM_RESOURCE_LOW   2     // Multiplier for whether a recipe has low resources (e.g., 2 * input amount)
#define PARAM_RESOURCE_HIGH  5     // Multiplier for whether a recipe has enough resources (e.g., 5 * input amount)
#define PARAM_SPEED_MODIFIER 1    // Usleep times are divided by this to speed up the simulation, faster for single-threaded mode recommended

#define SINGLE_THREAD_MODE 0       // Set this to zero to run the simulation in multi-threaded mode
#define TUI_MODE                   // Text UI Mode, comment this line out if you want it to print without fancy formatting.

// Represents the resource amounts for the entire rocket
typedef struct Resource {
    char *name;         // Dynamically allocated string
    int amount;         // Current amount of the resource in storage
    int max_capacity;   // Maximum capacity of the resource
    sem_t mutex;        // Binary semaphore to protect the resource from race conditions
} Resource;

// Represents the amount of a resource consumed/produced for a single system
typedef struct Recipe {
    Resource *input;    // Resource that is consumed, from central storage
    Resource *output;   // Resource that is produced, from central storage
    int input_amount;   // Amount of the input resource consumed
    int output_amount;  // Amount of the output resource produced
    int processing_time; // Processing time in milliseconds
} Recipe;

// A system which consumes resources, waits for `processing_time` milliseconds, then produced the produced resource
typedef struct System {
    char *name;         // Dynamically allocated string
    struct EventQueue *global_queue;  // Pointer to event queue shared by all systems and manager
    Recipe recipe;      // Stores information about what resources are produced / consumed
    int mode;           // Current mode of the system (e.g., STANDARD, SLOW, FAST, DISABLED, MODE_TERMINATE)
} System;

// Used to send notifications to the manager about an issue / state of the system
typedef struct Event {
    System *system;
    Resource *resource;
    int status;     
    int priority;   // Higher values indicate higher priority
} Event;

// Linked List Node for the Event queue
typedef struct EventNode {
    Event event;
    struct EventNode *next;
} EventNode;

// Linked List structure, single instance shared by all systems
typedef struct EventQueue {
    EventNode *head;
    sem_t mutex;        // Binary semaphore to protect the event queue from race conditions
} EventQueue;

// A basic dynamic array to store all of the systems in the simulation
typedef struct SystemArray {
    System **systems;
    int size;
    int capacity;
} SystemArray;

// A basic resource array to store the centralized resource stores of the rocket
typedef struct SharedResourceArray {
    Resource **resources;
    int size;
    int capacity;
} SharedResourceArray;

// Container structure which contains all of the core data for our simulation
typedef struct Manager {
    int simulation_running;
    SystemArray system_array;
    SharedResourceArray resources;
    EventQueue event_queue;
} Manager;

// Manager functions
void manager_init(Manager *manager);
void manager_clean(Manager *manager);
void manager_run(Manager *manager);

// System functions
void system_create(System **system, const char *name, Recipe recipe, EventQueue *event_queue);
void system_destroy(System *system);
void system_run(System *system);

// These getters help us tell the compiler, with this attribute tag, not to consider these functions for race conditions
int system_get_mode(const System *system) __attribute__((no_sanitize("thread")));
void system_set_mode(System *system, int mode) __attribute__((no_sanitize("thread")));

// Resource functions
void resource_create(Resource **resource, const char *name, int amount, int max_capacity);
void resource_destroy(Resource *resource);
void resource_transfer_into(Resource *resource, int *amount);
void resource_transfer_from(Resource *resource, int *amount);

// ResourceAmount functions
void recipe_init(Recipe *recipe, Resource *input, Resource *output, int input_amount, int output_amount, int processing_time);

// Event functions
void event_init(Event *event, System *system, Resource *resource, int status);

// EventQueue functions
void event_queue_init(EventQueue *queue);
void event_queue_clean(EventQueue *queue);
void event_queue_push(EventQueue *queue, const Event *event); 
int  event_queue_pop(EventQueue *queue, Event* event);

// Dynamic array functions for systems and resources
void system_array_init(SystemArray *array);
void system_array_clean(SystemArray *array);
void system_array_add(SystemArray *array, System *system);

void storage_init(SharedResourceArray *array);
void storage_clean(SharedResourceArray *array);
void storage_add(SharedResourceArray *array, Resource *resource);

// Simulation display functionality
void display_simulation_state(Manager *manager) __attribute__((no_sanitize("thread")));
void display_event(const Event *event) __attribute__((no_sanitize("thread")));
void display_finish_sim();

//Thread funciton declarations
void* system_thread(void *arg);
void* manager_thread(void *arg);