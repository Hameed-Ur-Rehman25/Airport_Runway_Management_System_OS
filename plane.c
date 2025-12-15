#include "plane.h"
#include "runway.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Initialize a plane
void plane_init(Plane *plane, int id, OperationType op, PriorityLevel priority)
{
    plane->id = id;
    plane->operation = op;
    plane->priority = priority;
    plane->state = WAITING;
    plane->checkpoint_progress = 0;
    plane->arrival_time = time(NULL);
    plane->start_time = 0;
    plane->completion_time = 0;
    sem_init(&plane->resume_sem, 0, 0); // Initialize to 0, will be posted when can resume
}

// Convert enum to string for display
const char *operation_to_string(OperationType op)
{
    return (op == LANDING) ? "LANDING" : "TAKEOFF";
}

const char *priority_to_string(PriorityLevel priority)
{
    return (priority == EMERGENCY) ? "EMERGENCY" : "NORMAL";
}

const char *state_to_string(PlaneState state)
{
    switch (state)
    {
    case WAITING:
        return "WAITING";
    case APPROACHING:
        return "APPROACHING";
    case USING_RUNWAY:
        return "USING_RUNWAY";
    case INTERRUPTED:
        return "INTERRUPTED";
    case COMPLETED:
        return "COMPLETED";
    default:
        return "UNKNOWN";
    }
}

// Main plane thread function
void *plane_thread_function(void *arg)
{
    Plane *plane = (Plane *)arg;

    // Log arrival
    runway_print_status("[ARRIVAL] Plane %d (%s, %s) requesting runway access",
                        plane->id,
                        priority_to_string(plane->priority),
                        operation_to_string(plane->operation));

    // Add to appropriate queue
    plane->state = WAITING;
    if (plane->priority == EMERGENCY)
    {
        queue_enqueue(&runway_system.emergency_queue, plane);
        sem_post(&runway_system.emergency_queue_sem); // Signal emergency queue has item

        // Set emergency flag to alert active plane
        sem_wait(&runway_system.emergency_flag_sem);
        runway_system.emergency_flag = 1;
        sem_post(&runway_system.emergency_flag_sem);

        runway_print_status("[EMERGENCY] Plane %d added to EMERGENCY queue (Queue size: %d)",
                            plane->id,
                            queue_get_count(&runway_system.emergency_queue));
    }
    else
    {
        queue_enqueue(&runway_system.normal_queue, plane);
        sem_post(&runway_system.normal_queue_sem); // Signal normal queue has item

        runway_print_status("[QUEUE] Plane %d added to NORMAL queue (Queue size: %d)",
                            plane->id,
                            queue_get_count(&runway_system.normal_queue));
    }

    // Request runway access (priority-based scheduling)
    runway_request_access(plane);

    // Perform runway operation (with checkpoint/resume)
    runway_perform_operation(plane);

    // Release runway
    runway_release(plane);

    // Mark as completed
    plane->state = COMPLETED;
    plane->completion_time = time(NULL);

    runway_print_status("[COMPLETED] Plane %d finished %s (Total time: %ld seconds)",
                        plane->id,
                        operation_to_string(plane->operation),
                        plane->completion_time - plane->arrival_time);

    // Increment completed counter
    sem_wait(&runway_system.completed_sem);
    runway_system.planes_completed++;
    sem_post(&runway_system.completed_sem);

    return NULL;
}

// Destroy plane resources
void plane_destroy(Plane *plane)
{
    sem_destroy(&plane->resume_sem);
}
