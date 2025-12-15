#ifndef PLANE_H
#define PLANE_H

#include <pthread.h>
#include <semaphore.h>
#include <time.h>

// Plane operation types
typedef enum
{
    LANDING,
    TAKEOFF
} OperationType;

// Plane priority levels
typedef enum
{
    NORMAL,
    EMERGENCY
} PriorityLevel;

// Plane state
typedef enum
{
    WAITING,
    APPROACHING,
    USING_RUNWAY,
    INTERRUPTED,
    COMPLETED
} PlaneState;

// Plane structure
typedef struct Plane
{
    int id;
    OperationType operation;
    PriorityLevel priority;
    PlaneState state;
    int checkpoint_progress; // 0-100%
    time_t arrival_time;
    time_t start_time;
    time_t completion_time;
    pthread_t thread;
    sem_t resume_sem; // Semaphore to signal when plane can resume
} Plane;

// Plane functions
void plane_init(Plane *plane, int id, OperationType op, PriorityLevel priority);
void *plane_thread_function(void *arg);
void plane_destroy(Plane *plane);
const char *operation_to_string(OperationType op);
const char *priority_to_string(PriorityLevel priority);
const char *state_to_string(PlaneState state);

#endif // PLANE_H
