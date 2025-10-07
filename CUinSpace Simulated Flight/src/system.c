/***************************************************************
 * system.c
 * Contains functionality for running a single system.
 * TODO: Add a `system_thread()` function which runs the system_run() function
 *       in a loop.
 ***************************************************************/

#include "defs.h"
#include <assert.h>

// Helper functions just used by this C file to clean up our code
// Using static means they can't get linked into other files
static void system_simulate_process_time(System *);
static void report_recipe_thresholds(System *system);

/**
 * Creates and initializes a `System` structure.
 *
 * Dynamically allocates memory for a system and sets its name, recipe, mode, and global event queue.
 * NOTE: Global queue is not actually a data segment global queue, it's just shared between systems.
 *
 * @param[out] system      Pointer to the `System` to initialize.
 * @param[in]  name        Name of the system, copied into a dynamically allocate field.
 * @param[in]  recipe      Recipe containing input/output resources and processing time.
 * @param[in]  event_queue Pointer to the shared event queue for the system, the global_queue field of the system.
 */
void system_create(System **system, const char *name, Recipe recipe, EventQueue *event_queue) {
    // Dynamically allocate memory for the System structure
    *system = (System *)malloc(sizeof(System));
    assert(*system != NULL);
    
    // Dynamically allocate and copy the name
    (*system)->name = (char *)malloc(strlen(name) + 1);
    assert((*system)->name != NULL);
    strcpy((*system)->name, name);
    
    // Initialize the recipe (copy by value)
    (*system)->recipe = recipe;
    
    // Set the global event queue
    (*system)->global_queue = event_queue;
    
    // Initialize mode to STANDARD as default
    (*system)->mode = MODE_STANDARD;
}

/**
 * Destroys a `System` structure.
 *
 * Frees all dynamic memory for a system.
 *
 * @param[in] system Pointer to the `System` to destroy.
 */
void system_destroy(System *system) {
    if (system != NULL) {
        // Free the dynamically allocated name
        if (system->name != NULL) {
            free(system->name);
        }
        
        // Free the System structure itself
        free(system);
    }
}

/**
 * Gets the current mode of the system.
 *
 * @param[in] system Pointer to the `System` to get the mode from.
 * @return The current mode of the system.
 */
int system_get_mode(const System *system) {
    return system->mode;
}

/**
 * Sets the mode of the system.
 *
 * @param[in,out] system Pointer to the `System` to set the mode for.
 * @param[in]     mode   The new mode to set for the system.
 */
void system_set_mode(System *system, int mode) {
    system->mode = mode;
}

/**
 * Main execution function for a system.
 *
 * Attempts to run the system's recipe, pulling input resources, processing them, and pushing output resources.
 * If SINGLE_THREAD_MODE is defined as a non-zero value, it will not wait for resources to accumulate before continuing.
 *
 * @param[in,out] system Pointer to the `System` to run.
 */
void system_run(System *system) {
    int local_output_amount = 0;

    // Pull input resources until we have enough to convert
    int amount_to_pull = system->recipe.input_amount;
    while (amount_to_pull > 0 && system_get_mode(system) != MODE_TERMINATE) {
        resource_transfer_from(system->recipe.input, &amount_to_pull);
        if (amount_to_pull > 0) {
            // If we don't have enough input resources, report the low status
            Event *event = malloc(sizeof(Event));  // Allocate new event
            event_init(event, system, system->recipe.input, EVENT_INSUFFICIENT);
            event_queue_push(system->global_queue, event);
            free(event);
            usleep(PARAM_SYSTEM_WAIT * 1000 / PARAM_SPEED_MODIFIER);

            if (SINGLE_THREAD_MODE) return;
        }
    }

    // If we have enough input resources, process them
    if (amount_to_pull == 0) {
        system_simulate_process_time(system);
        local_output_amount = system->recipe.output_amount;
        Event *event = malloc(sizeof(Event));  // Allocate new event
        event_init(event, system, system->recipe.input, EVENT_PRODUCED);
        event_queue_push(system->global_queue, event);
        free(event);
    }

    // Push the resource to the centralized storage, IF there is even an output in the recipe
    while (system->recipe.output && local_output_amount > 0 && system_get_mode(system) != MODE_TERMINATE) {
        resource_transfer_into(system->recipe.output, &local_output_amount);
        if (local_output_amount > 0) {
            // If we didn't load everything in, report that we're still at capacity
            Event *event = malloc(sizeof(Event));  // Allocate new event
            event_init(event, system, system->recipe.output, EVENT_CAPACITY);
            event_queue_push(system->global_queue, event);
            free(event);
            usleep(PARAM_SYSTEM_WAIT * 1000 / PARAM_SPEED_MODIFIER);

            if (SINGLE_THREAD_MODE) return;
        }
    }

    report_recipe_thresholds(system);
}

/**
 * Thread safe local helper function that reports the current thresholds for a system's recipe.
 *
 * @param[in] system Pointer to the `System` to report thresholds for.
 */
static void report_recipe_thresholds(System *system) {
    // Check if input resource exists
    if (system->recipe.input == NULL) {
        return;  // Skip if no input resource
    }
    
    int low_threshold = system->recipe.input_amount * PARAM_RESOURCE_LOW;
    int high_threshold = system->recipe.input_amount * PARAM_RESOURCE_HIGH;
    int current_amount;

    // Acquire the semaphore
    sem_wait(&system->recipe.input->mutex);
    current_amount = system->recipe.input->amount;
    sem_post(&system->recipe.input->mutex);

    if (current_amount <= low_threshold) {
        Event *event = malloc(sizeof(Event));  // Allocate new event
        event_init(event, system, system->recipe.input, EVENT_LOW);
        event_queue_push(system->global_queue, event);
        free(event);
    } else if (current_amount > high_threshold) {
        Event *event = malloc(sizeof(Event));  // Allocate new event
        event_init(event, system, system->recipe.input, EVENT_HIGH);
        event_queue_push(system->global_queue, event);
        free(event);
    }
}

/**
 * Local helper function that simulates the processing time of a system.
 * 
 * @param[in] system Pointer to the `System` to simulate processing time for.
 */
static void system_simulate_process_time(System *system) {
    int adjusted_processing_time;
    switch (system->mode) {
        case MODE_SLOW:
            adjusted_processing_time = system->recipe.processing_time * 4;
            break;
        case MODE_FAST:
            adjusted_processing_time = system->recipe.processing_time / 4;
            break;
        default:
            adjusted_processing_time = system->recipe.processing_time;
    }
    usleep(adjusted_processing_time * 1000 / PARAM_SPEED_MODIFIER);
}

/**
 * Initializes a `SystemArray` structure.
 *
 * Allocates memory for the system array and initializes its size and capacity.
 *
 * @param[out] array Pointer to the `SystemArray` to initialize.
 */
void system_array_init(SystemArray *array) {
    assert(array != NULL);
    
    // Initialize with a reasonable starting capacity
    array->capacity = 10;
    array->size = 0;
    
    // Dynamically allocate memory for the systems array
    array->systems = (System **)malloc(array->capacity * sizeof(System *));
    assert(array->systems != NULL);
}

/**
 * Cleans up a `SystemArray` structure.
 *
 * Frees all memory associated with the array and its systems.
 *
 * @param[in,out] array Pointer to the `SystemArray` to clean.
 */
void system_array_clean(SystemArray *array) {
    if (array != NULL) {
        // Free all systems in the array
        for (int i = 0; i < array->size; i++) {
            if (array->systems[i] != NULL) {
                system_destroy(array->systems[i]);
            }
        }
        
        // Free the systems array itself
        if (array->systems != NULL) {
            free(array->systems);
        }
        
        // Reset array fields
        array->systems = NULL;
        array->size = 0;
        array->capacity = 0;
    }
}

/**
 * Adds a `System` to a `SystemArray`.
 *
 * Resizes the array if necessary and adds the system to the end of the array.
 *
 * @param[in,out] array Pointer to the `SystemArray` to add the system to.
 * @param[in]     system Pointer to the `System` to add.
 */
void system_array_add(SystemArray *array, System *system) {
    assert(array != NULL);
    assert(system != NULL);
    
    // Check if we need to resize the array
    if (array->size >= array->capacity) {
        // Double the capacity
        int new_capacity = array->capacity * 2;
        
        // Manually allocate new memory (can't use realloc)
        System **new_systems = (System **)malloc(new_capacity * sizeof(System *));
        assert(new_systems != NULL);
        
        // Copy existing systems to new array
        for (int i = 0; i < array->size; i++) {
            new_systems[i] = array->systems[i];
        }
        
        // Free old array and update to new one
        free(array->systems);
        array->systems = new_systems;
        array->capacity = new_capacity;
    }
    
    // Add the new system
    array->systems[array->size] = system;
    array->size++;
}

/**
 * Thread function for running a system.
 * This is the entry point for system threads that will be created by pthread_create().
 * 
 * @param[in] arg Pointer to the System structure (cast from void*)
 * @return NULL (required for pthread function signature)
 */

void* system_thread(void *arg) {
    System *system = (System *)arg;

    // Check if system is valid
    if (system == NULL) {
        printf("Error: NULL system passed to system_thread\n");
        return NULL;
    }
    
    // Run the system in a loop until the system is terminated
    while (system_get_mode(system) != MODE_TERMINATE) {
        system_run(system);

        // Small delay to prevent spamming the event queue
        usleep(PARAM_SYSTEM_WAIT * 1000 / PARAM_SPEED_MODIFIER);
    }

    return NULL;
}