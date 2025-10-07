/***************************************************************
 * manager.c
 * Contains functionality for managing the simulation.
 * TODO: Add a `manager_thread()` function which runs the manager_run() function
 *       in a loop.
 ***************************************************************/

#include "defs.h"

/**
 * Initializes a `Manager` structure.
 *
 * Sets up a `Manager` with the initial state for the simulation.
 *
 * @param[out] manager     Pointer to the `Manager` to initialize.
 */
void manager_init(Manager *manager) {
    manager->simulation_running = 1;
    system_array_init(&manager->system_array);
    storage_init(&manager->resources);
    event_queue_init(&manager->event_queue);
}

/**
 * Cleans up the `Manager` structure.
 *
 * Frees all resources associated with the `Manager`.
 *
 * @param[in,out] manager  Pointer to the `Manager` to clean.
 */
void manager_clean(Manager *manager) {
    system_array_clean(&manager->system_array);
    storage_clean(&manager->resources);
    event_queue_clean(&manager->event_queue);
}

/**
 * Main execution loop for the manager. 
 *
 * Runs through all currently queued events until either all events are popped 
 * or the simulation is no longer running.
 *
 * @param[in,out] manager  Pointer to the `Manager` to run.

 */
void manager_run(Manager *manager) {
    Event event;
    int i, mode;
    int no_oxygen_flag = 0, distance_reached_flag = 0, need_more_flag = 0, need_less_flag = 0;
    
    System *sys = NULL;
        
    // Update the display of the current state of things
    display_simulation_state(manager);

    // Process events if one is popped
    while (manager->simulation_running && event_queue_pop(&manager->event_queue, &event)) {
        printf("Manager: Event popped %s\n", event.system->name); // Debug output
        if (event.priority == PRIORITY_IGN) continue;

        display_event(&event);

        // Default to swapping systems back into standard mode unless a check tells us otherwise.
        mode = MODE_STANDARD;

        // Set some flags based on the event that we can react to below
        no_oxygen_flag        = (event.status == EVENT_INSUFFICIENT && strcmp(event.resource->name, "Oxygen") == 0);
        distance_reached_flag = (event.status == EVENT_CAPACITY && strcmp(event.resource->name, "Distance") == 0);
        need_more_flag        = (event.status == EVENT_LOW || event.status == EVENT_INSUFFICIENT);
        need_less_flag        = (event.status == EVENT_CAPACITY) || (event.status == EVENT_HIGH);

        if (no_oxygen_flag) {
            display_finish_sim();
            printf("Oxygen depleted. Terminating all systems.\n");
            mode = MODE_TERMINATE;
            manager->simulation_running = 0;
        }
        else if (distance_reached_flag) {
            display_finish_sim();
            printf("Destination reached. Terminating all systems.\n");
            mode = MODE_TERMINATE;
            manager->simulation_running = 0;
        }
        else if (need_more_flag) {
            mode = MODE_FAST;
        }
        else if (need_less_flag) {
            mode = MODE_SLOW;
        }

        // Update all of the systems to speed up or slow down production, or terminate
        for (i = 0; i < manager->system_array.size; i++) {
            sys = manager->system_array.systems[i];
            if (system_get_mode(sys) == MODE_TERMINATE) continue;

            if (mode == MODE_TERMINATE || sys->recipe.output == event.resource) {
                system_set_mode(sys, mode);
            }
        }

        usleep(PARAM_MANAGER_WAIT * 1000 / PARAM_SPEED_MODIFIER);
    }
}

/**
 * Thread function for running the manager.
 * This is the entry point for the manager thread that will be created by pthread_create().
 * 
 * @param[in] arg Pointer to the Manager structure (cast from void*)
 * @return NULL (required for pthread function signature)
 */
 void* manager_thread(void *arg) {
    Manager *manager = (Manager*)arg;

    printf("Manager thread started\n"); // Debug output
    
    // Run the manager in a loop until simulation stops
    while (manager->simulation_running) {
        manager_run(manager);
        
        // Small delay to prevent busy waiting
        usleep(PARAM_MANAGER_WAIT * 1000 / PARAM_SPEED_MODIFIER);
    }

    printf("Manager thread ended\n"); // Debug output
    
    return NULL;
}