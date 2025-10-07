#include "defs.h"

void load_data(Manager *manager);

int main(void) {
    Manager manager;
    pthread_t manager_thread_id;
    pthread_t *system_threads;

    manager_init(&manager);
    load_data(&manager);
    
    // NOTE: The code to handle the manager run and the systems
    //       will be moved to threading functions.
    /*while (manager.simulation_running) {
        manager_run(&manager);
        for (int i = 0; i < manager.system_array.size; i++) {
            system_run(manager.system_array.systems[i]);
        }
    }*/

    // Allocate array for system threads

    system_threads = malloc(manager.system_array.size * sizeof(pthread_t));
    if (system_threads == NULL) {
        printf("Failed to allocate memory for system threads\n");
        return 1;
    }

    // Create manager thread
    if (pthread_create(&manager_thread_id, NULL, manager_thread, &manager) != 0){
        printf("Failed to create manager thread\n");
        return 1;
    }

    // Create system threads
    for (int i = 0; i < manager.system_array.size; i++) {
        if (pthread_create(&system_threads[i], NULL, system_thread, manager.system_array.systems[i]) != 0){
            printf("Failed to create system thread %d\n", i);
            return 1;
        }
    }

    // Wait for manager and system threads to finish
    pthread_join(manager_thread_id, NULL);
    for (int i = 0; i < manager.system_array.size; i++) {
        pthread_join(system_threads[i], NULL);
    }

    // Free system threads
    free(system_threads);

    // Find the distance resource to print out how far we went
    for (int i = 0; i < manager.resources.size; i++) {
        if (strcmp(manager.resources.resources[i]->name, "Distance") == 0) {
            printf("=> Total Distance Travelled: %d furlongs.\n", manager.resources.resources[i]->amount);
        }
    }

    // Clean up manager
    manager_clean(&manager);
    return 0;
}

void load_data(Manager *manager) {
    // Create resources
    Resource *fuel, *oxygen, *energy, *distance;
    resource_create(&fuel, "Fuel", 1000, 1000);
    resource_create(&oxygen, "Oxygen", 20, 50);
    resource_create(&energy, "Energy", 30, 50);
    resource_create(&distance, "Distance", 0, 1000);

    storage_add(&manager->resources, fuel);
    storage_add(&manager->resources, oxygen);
    storage_add(&manager->resources, energy);
    storage_add(&manager->resources, distance);

    // Create systems
    System *propulsion_system, *life_support_system, *crew_capsule_system, *generator_system;
    Recipe propulsion_recipe, life_support_recipe, crew_capsule_recipe, generator_recipe;

    // Propulsion: consumes fuel, produces distance
    recipe_init(&propulsion_recipe, fuel, distance, 5, 25, 500);
    system_create(&propulsion_system, "Propulsion", propulsion_recipe, &manager->event_queue);

    // Life Support: consumes energy, produces oxygen
    recipe_init(&life_support_recipe, energy, oxygen, 10, 5, 100);
    system_create(&life_support_system, "Life Support", life_support_recipe, &manager->event_queue);

    // Crew Capsule: consumes oxygen, produces nothing
    recipe_init(&crew_capsule_recipe, oxygen, NULL, 5, 0, 200);
    system_create(&crew_capsule_system, "Crew", crew_capsule_recipe, &manager->event_queue);

    // Generator: consumes fuel, produces energy
    recipe_init(&generator_recipe, fuel, energy, 10, 9, 200);
    system_create(&generator_system, "Generator", generator_recipe, &manager->event_queue);

    system_array_add(&manager->system_array, propulsion_system);
    system_array_add(&manager->system_array, life_support_system);
    system_array_add(&manager->system_array, crew_capsule_system);
    system_array_add(&manager->system_array, generator_system);
}