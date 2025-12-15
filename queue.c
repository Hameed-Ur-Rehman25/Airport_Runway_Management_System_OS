#include "queue.h"
#include "plane.h"
#include <stdlib.h>
#include <stdio.h>

// Initialize queue
void queue_init(Queue *queue)
{
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    sem_init(&queue->sem_access, 0, 1); // Binary semaphore initialized to 1
}

// Enqueue a plane (add to tail)
void queue_enqueue(Queue *queue, Plane *plane)
{
    sem_wait(&queue->sem_access);

    QueueNode *new_node = (QueueNode *)malloc(sizeof(QueueNode));
    if (new_node == NULL)
    {
        perror("Failed to allocate queue node");
        sem_post(&queue->sem_access);
        return;
    }

    new_node->plane = plane;
    new_node->next = NULL;

    if (queue->tail == NULL)
    {
        // Queue is empty
        queue->head = new_node;
        queue->tail = new_node;
    }
    else
    {
        // Add to tail
        queue->tail->next = new_node;
        queue->tail = new_node;
    }

    queue->count++;

    sem_post(&queue->sem_access);
}

// Dequeue a plane (remove from head)
Plane *queue_dequeue(Queue *queue)
{
    sem_wait(&queue->sem_access);

    if (queue->head == NULL)
    {
        sem_post(&queue->sem_access);
        return NULL;
    }

    QueueNode *node = queue->head;
    Plane *plane = node->plane;

    queue->head = node->next;
    if (queue->head == NULL)
    {
        queue->tail = NULL;
    }

    queue->count--;
    free(node);

    sem_post(&queue->sem_access);
    return plane;
}

// Peek at the front plane without removing
Plane *queue_peek(Queue *queue)
{
    sem_wait(&queue->sem_access);

    Plane *plane = NULL;
    if (queue->head != NULL)
    {
        plane = queue->head->plane;
    }

    sem_post(&queue->sem_access);
    return plane;
}

// Check if queue is empty
int queue_is_empty(Queue *queue)
{
    sem_wait(&queue->sem_access);
    int empty = (queue->head == NULL);
    sem_post(&queue->sem_access);
    return empty;
}

// Get queue count
int queue_get_count(Queue *queue)
{
    sem_wait(&queue->sem_access);
    int count = queue->count;
    sem_post(&queue->sem_access);
    return count;
}

// Destroy queue and free all nodes
void queue_destroy(Queue *queue)
{
    sem_wait(&queue->sem_access);

    QueueNode *current = queue->head;
    while (current != NULL)
    {
        QueueNode *next = current->next;
        free(current);
        current = next;
    }

    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;

    sem_post(&queue->sem_access);
    sem_destroy(&queue->sem_access);
}
