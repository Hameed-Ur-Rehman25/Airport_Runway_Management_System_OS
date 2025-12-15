#ifndef QUEUE_H
#define QUEUE_H

#include <semaphore.h>

// Forward declaration
struct Plane;

// Queue node structure
typedef struct QueueNode
{
    struct Plane *plane;
    struct QueueNode *next;
} QueueNode;

// Queue structure
typedef struct Queue
{
    QueueNode *head;
    QueueNode *tail;
    int count;
    sem_t sem_access; // Binary semaphore to protect queue operations
} Queue;

// Queue operations
void queue_init(Queue *queue);
void queue_enqueue(Queue *queue, struct Plane *plane);
struct Plane *queue_dequeue(Queue *queue);
struct Plane *queue_peek(Queue *queue);
int queue_is_empty(Queue *queue);
int queue_get_count(Queue *queue);
void queue_destroy(Queue *queue);

#endif // QUEUE_H
