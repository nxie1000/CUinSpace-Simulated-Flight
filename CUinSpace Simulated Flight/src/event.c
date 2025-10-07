/***************************************************************
 * event.c 
 * Contains functionality for events and event queues.
 * Event queues pop from the head and push into priority order, highest priority at the beginning.
 ***************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "defs.h"

/**
 * Initializes an `Event` structure.
 *
 * Sets up an `Event` with the provided system, resource, status, priority, and amount.
 *
 * @param[out] event     Pointer to the `Event` to initialize.
 * @param[in]  system    Pointer to the `System` that generated the event.
 * @param[in]  resource  Pointer to the `Resource` associated with the event.
 * @param[in]  status    Status code representing the event type.
 */
void event_init(Event *event, System *system, Resource *resource, int status) {
    event->system   = system;
    event->resource = resource;
    event->status   = status;
    event->priority = status & 0xFF00; // Extract priority bits from status
}

/**
 * Initializes an `EventQueue` structure.
 *
 * @param[out] queue     Pointer to the `EventQueue` to initialize.
 */
void event_queue_init(EventQueue *queue) {
    assert(queue != NULL);
    queue->head = NULL;

    // Initialize the semaphore
    int result = sem_init(&queue->mutex, 0, 1);
    assert(result == 0); // Check if the semaphore was initialized successfully
}

/**
 * Cleans up the `EventQueue` by freeing all nodes.
 *
 * @param[in,out] queue  Pointer to the `EventQueue` to clean.
 */
void event_queue_clean(EventQueue *queue) {
    if (queue != NULL) {
        // Destroy the semaphore
        sem_destroy(&queue->mutex);

        EventNode *current = queue->head;
        
        // Free all nodes in the linked list
        while (current != NULL) {
            EventNode *next = current->next;
            free(current);
            current = next;
        }
        
        // Reset head to NULL
        queue->head = NULL;
    }
}

/**
 * Pushes an `Event` onto the `EventQueue`.
 *
 * Adds the event to the queue in a thread-safe manner, maintaining priority order (highest first).
 *
 * @param[in,out] queue  Pointer to the `EventQueue`.
 * @param[in]     event  Pointer to the `Event` to push onto the queue.
 */
void event_queue_push(EventQueue *queue, const Event *event) {
    assert(queue != NULL);
    assert(event != NULL);
    
    // Acquire the semaphore
    sem_wait(&queue->mutex);

    // Create a new node
    EventNode *new_node = (EventNode *)malloc(sizeof(EventNode));
    assert(new_node != NULL);
    
    // Copy the event data
    new_node->event = *event;
    new_node->next = NULL;
    
    // If queue is empty, make this the head
    if (queue->head == NULL) {
        queue->head = new_node;

        // Release the semaphore
        sem_post(&queue->mutex);
        return;
    }
    
    // Find the correct position to insert based on priority
    // Higher priority values come first, same priority maintains order (FIFO)
    EventNode *current = queue->head;
    EventNode *prev = NULL;
    
    // Traverse until we find a node with lower priority
    while (current != NULL && current->event.priority >= event->priority) {
        prev = current;
        current = current->next;
    }
    
    // Insert at the beginning. Works for both empty and non-empty queues.
    if (prev == NULL) {
        new_node->next = queue->head;
        queue->head = new_node;
    }
    // Insert in the middle or at the end
    else {
        prev->next = new_node;
        new_node->next = current;
    }

    // Release the semaphore
    sem_post(&queue->mutex);
}

/**
 * Pops an `Event` from the `EventQueue`.
 *
 * Removes the highest priority event from the queue in a thread safe manner and returns it.
 *
 * @param[in,out] queue  Pointer to the `EventQueue`.
 * @param[out]    event  Pointer to the `Event` to fill with the popped data.
 * @return 1 if an event was successfully popped, 0 if the queue was empty.
 */
int event_queue_pop(EventQueue *queue, Event* event) {
    assert(queue != NULL);
    assert(event != NULL);
    
    // Acquire the semaphore
    sem_wait(&queue->mutex);

    // Check if queue is empty
    if (queue->head == NULL) {
        sem_post(&queue->mutex);
        return 0;
    }
    
    // Get the head node
    EventNode *head_node = queue->head;
    
    // Copy the event data
    *event = head_node->event;
    
    // Update head to next node
    queue->head = head_node->next;
    
    // Free the old head node
    free(head_node);
    
    // Release the semaphore
    sem_post(&queue->mutex);
    
    return 1;
}
