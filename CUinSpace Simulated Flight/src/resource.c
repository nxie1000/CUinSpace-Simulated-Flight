/***************************************************************
 * resource.c
 * Contains functionality for managing resources.
 * This includes: Resources, Storage (Resource Array), Recipes
 ***************************************************************/

#include "defs.h"
#include <assert.h>

/**
 * Creates and initializes a `Resource` structure.
 * Dynamically allocates memory for a resource and the resource name field and initializes all of the values.
 * 
 * @param[out] resource     Pointer to the `Resource` to initialize.
 * @param[in]  name         Name of the resource, copied into a dynamically allocated field.
 * @param[in]  amount       Initial amount of the resource avaialble.
 * @param[in]  max_capacity Maximum capacity this resource can hold.
 */
void resource_create(Resource **resource, const char *name, int amount, int max_capacity) {
    // Dynamically allocate memory for the Resource structure
    *resource = (Resource *)malloc(sizeof(Resource));
    assert(*resource != NULL);
    
    // Dynamically allocate and copy the name
    (*resource)->name = (char *)malloc(strlen(name) + 1);
    assert((*resource)->name != NULL);
    strcpy((*resource)->name, name);
    
    // Initialize the resource values
    (*resource)->amount = amount;
    (*resource)->max_capacity = max_capacity;

    // Initialize the semaphore
    int result = sem_init(&(*resource)->mutex, 0, 1);
    assert(result == 0); // Check if the semaphore was initialized successfully
}

/**
 * Destroys a `Resource` structure.
 * Frees all dynamic memory for a resource.
 *
 * @param[in] resource Pointer to the `Resource` to destroy.
 */
void resource_destroy(Resource *resource) {
    if (resource != NULL) {
        // Destroy the semaphore
        sem_destroy(&resource->mutex);

        // Free the dynamically allocated name
        if (resource->name != NULL) {
            free(resource->name);
        }
        
        // Free the Resource structure itself
        free(resource);
    }
}

/**
 * Thread safe function to add as much resource from the amount as possible and decreases amount by the amount that was added.
 * 
 * @param[in,out] resource Pointer to the `Resource` to add to.
 * @param[in,out] amount   Pointer to the amount of resource to add, will be decreased by the amount that was added.
 */
void resource_transfer_into(Resource *resource, int *amount) {
    // Acquire the semaphore
    sem_wait(&resource->mutex);

    int remaining_capacity = resource->max_capacity - resource->amount;
    int amount_to_transfer = (remaining_capacity >= *amount)? *amount : remaining_capacity;
    
    resource->amount += amount_to_transfer;
    *amount -= amount_to_transfer; // Decrease the amount by what was added

    // Release the semaphore
    sem_post(&resource->mutex);
}

/**
 * Thread safe function to remove as much of a resource as it can and returns the actual amount that was removed.
 * 
 * @param[in,out] resource Pointer to the `Resource` to remove from.
 * @param[in,out] amount   Pointer to the amount of resource to remove, will be decreased by the amount that was removed.
 */
void resource_transfer_from(Resource *resource, int *amount) {
    // Acquire the semaphore
    sem_wait(&resource->mutex);

    int amount_to_transfer = (resource->amount < *amount) ? resource->amount : *amount; // Remove all that's available
    
    resource->amount -= amount_to_transfer;
    *amount -= amount_to_transfer;

    // Release the semaphore
    sem_post(&resource->mutex);
}

/**
 * Initializes a `Recipe` structure.
 * Sets the input and output resources, their amounts, and the processing time.
 * 
 * @param[out] recipe          Pointer to the `Recipe` to initialize.
 * @param[in]  input           Pointer to the input `Resource` for the recipe.
 * @param[in]  output          Pointer to the output `Resource` for the recipe.
 * @param[in]  input_amount    Amount of the input resource consumed.
 * @param[in]  output_amount   Amount of the output resource produced.
 * @param[in]  processing_time Processing time in milliseconds.
*/
void recipe_init(Recipe *recipe, Resource *input, Resource *output, int input_amount, int output_amount, int processing_time) {
    recipe->input = input;
    recipe->output = output;
    recipe->input_amount = input_amount;
    recipe->output_amount = output_amount;
    recipe->processing_time = processing_time;
}

/**
 * Initializes the resource array with a given initial capacity.
 * 
 * @param[out] array Pointer to the `SharedResourceArray` to initialize.
 */
void storage_init(SharedResourceArray *array) {
    assert(array != NULL);
    
    // Initialize with a reasonable starting capacity
    array->capacity = 10;
    array->size = 0;
    
    // Dynamically allocate memory for the resources array
    array->resources = (Resource **)malloc(array->capacity * sizeof(Resource *));
    assert(array->resources != NULL);
}

/**
 * Cleans up the resource array by destroying all resources and freeing memory.
 * 
 * @param[in,out] array Pointer to the `SharedResourceArray` to clean.
 */
void storage_clean(SharedResourceArray *array) {
    if (array != NULL) {
        // Free all resources in the array
        for (int i = 0; i < array->size; i++) {
            if (array->resources[i] != NULL) {
                resource_destroy(array->resources[i]);
            }
        }
        
        // Free the resources array itself
        if (array->resources != NULL) {
            free(array->resources);
        }
        
        // Reset array fields
        array->resources = NULL;
        array->size = 0;
        array->capacity = 0;
    }
}

/**
 * Adds a resource to the resource array, resizing if necessary.
 * 
 * @param[in,out] storage Pointer to the `SharedResourceArray` to add the resource to.
 * @param[in]     resource Pointer to the `Resource` to add.
 */
void storage_add(SharedResourceArray *storage, Resource *resource) {
    assert(storage != NULL);
    assert(resource != NULL);
    
    // Check if we need to resize the array
    if (storage->size >= storage->capacity) {
        // Double the capacity
        int new_capacity = storage->capacity * 2;
        
        // Manually allocate new memory (can't use realloc)
        Resource **new_resources = (Resource **)malloc(new_capacity * sizeof(Resource *));
        assert(new_resources != NULL);
        
        // Copy existing resources to new array
        for (int i = 0; i < storage->size; i++) {
            new_resources[i] = storage->resources[i];
        }
        
        // Free old array and update to new one
        free(storage->resources);
        storage->resources = new_resources;
        storage->capacity = new_capacity;
    }
    
    // Add the new resource
    storage->resources[storage->size] = resource;
    storage->size++;
}
