#include "runway.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>

// Global runway system instance
RunwaySystem runway_system;

// Initialize runway system
void runway_init(RunwaySystem *sys, int landing_duration, int takeoff_duration)
{
    // Initialize semaphores
    sem_init(&sys->runway_access, 0, 1);       // Binary semaphore for runway
    sem_init(&sys->console_access, 0, 1);      // Binary semaphore for console
    sem_init(&sys->emergency_flag_sem, 0, 1);  // Binary semaphore for flag
    sem_init(&sys->emergency_queue_sem, 0, 0); // Counting semaphore (initially 0)
    sem_init(&sys->normal_queue_sem, 0, 0);    // Counting semaphore (initially 0)
    sem_init(&sys->active_plane_sem, 0, 1);    // Binary semaphore for active plane
    sem_init(&sys->completed_sem, 0, 1);       // Binary semaphore for counter
    sem_init(&sys->preemptions_sem, 0, 1);     // Binary semaphore for preemptions

    // Initialize state
    sys->emergency_flag = 0;
    sys->active_plane = NULL;
    sys->total_planes = 0;
    sys->planes_completed = 0;
    sys->preemptions_count = 0;

    // Initialize queues
    queue_init(&sys->emergency_queue);
    queue_init(&sys->normal_queue);

    // Set configuration
    sys->config.landing_duration = landing_duration;
    sys->config.takeoff_duration = takeoff_duration;

    runway_print_status("[SYSTEM] Runway system initialized (Landing: %ds, Takeoff: %ds)",
                        landing_duration, takeoff_duration);
}

// Thread-safe console output
void runway_print_status(const char *format, ...)
{
    sem_wait(&runway_system.console_access);

    // Get current time
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[9];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);

    printf("[%s] ", time_str);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
    fflush(stdout);

    sem_post(&runway_system.console_access);
}

// Request runway access with priority scheduling
void runway_request_access(Plane *plane)
{
    plane->state = APPROACHING;

    // Priority-based scheduling: always check emergency queue first
    if (plane->priority == EMERGENCY)
    {
        // Emergency plane waits on emergency queue semaphore
        sem_wait(&runway_system.emergency_queue_sem);

        // Try to acquire runway
        sem_wait(&runway_system.runway_access);

        // Remove self from queue
        Plane *dequeued = queue_dequeue(&runway_system.emergency_queue);

        runway_print_status("[GRANTED] EMERGENCY Plane %d granted runway access", plane->id);
    }
    else
    {
        // Normal plane waits on normal queue semaphore
        sem_wait(&runway_system.normal_queue_sem);

        // Try to acquire runway
        sem_wait(&runway_system.runway_access);

        // Remove self from queue
        Plane *dequeued = queue_dequeue(&runway_system.normal_queue);

        runway_print_status("[GRANTED] NORMAL Plane %d granted runway access", plane->id);
    }

    // Set as active plane
    sem_wait(&runway_system.active_plane_sem);
    runway_system.active_plane = plane;
    sem_post(&runway_system.active_plane_sem);
}

// Perform runway operation with checkpoint support
void runway_perform_operation(Plane *plane)
{
    plane->state = USING_RUNWAY;
    if (plane->start_time == 0)
    {
        plane->start_time = time(NULL);
    }

    // Determine operation duration
    int duration = (plane->operation == LANDING) ? runway_system.config.landing_duration : runway_system.config.takeoff_duration;

    // Calculate remaining duration based on checkpoint
    int elapsed_time = (duration * plane->checkpoint_progress) / 100;
    int remaining_time = duration - elapsed_time;

    if (plane->checkpoint_progress > 0)
    {
        runway_print_status("[RESUME] Plane %d resuming %s from %d%% (remaining: %ds)",
                            plane->id,
                            operation_to_string(plane->operation),
                            plane->checkpoint_progress,
                            remaining_time);
    }
    else
    {
        runway_print_status("[OPERATION] Plane %d starting %s (duration: %ds)",
                            plane->id,
                            operation_to_string(plane->operation),
                            duration);
    }

    // Perform operation with frequent checkpoint checking (every 500ms)
    int checkpoint_interval_ms = 500;
    int total_intervals = (remaining_time * 1000) / checkpoint_interval_ms;

    for (int i = 0; i < total_intervals; i++)
    {
        usleep(checkpoint_interval_ms * 1000); // Sleep for 500ms

        // Update checkpoint progress
        plane->checkpoint_progress = elapsed_time +
                                     ((i + 1) * checkpoint_interval_ms * 100) / (duration * 1000);
        if (plane->checkpoint_progress > 100)
            plane->checkpoint_progress = 100;

        // Check for emergency preemption (only for normal planes)
        if (plane->priority == NORMAL)
        {
            sem_wait(&runway_system.emergency_flag_sem);
            int emergency_pending = runway_system.emergency_flag;
            sem_post(&runway_system.emergency_flag_sem);

            if (emergency_pending)
            {
                // Save checkpoint and yield runway
                plane->state = INTERRUPTED;

                runway_print_status("[PREEMPTED] Plane %d interrupted at %d%% - yielding to emergency",
                                    plane->id, plane->checkpoint_progress);

                // Increment preemption counter
                sem_wait(&runway_system.preemptions_sem);
                runway_system.preemptions_count++;
                sem_post(&runway_system.preemptions_sem);

                // Clear active plane
                sem_wait(&runway_system.active_plane_sem);
                runway_system.active_plane = NULL;
                sem_post(&runway_system.active_plane_sem);

                // Release runway
                sem_post(&runway_system.runway_access);

                // Clear emergency flag
                sem_wait(&runway_system.emergency_flag_sem);
                runway_system.emergency_flag = 0;
                sem_post(&runway_system.emergency_flag_sem);

                // Re-enqueue to normal queue
                queue_enqueue(&runway_system.normal_queue, plane);
                sem_post(&runway_system.normal_queue_sem);

                runway_print_status("[REQUEUE] Plane %d re-queued to NORMAL queue with checkpoint at %d%%",
                                    plane->id, plane->checkpoint_progress);

                // Wait on resume semaphore (will be signaled when can retry)
                // For this implementation, we'll request access again
                runway_request_access(plane);

                // Recursive call to resume operation
                runway_perform_operation(plane);
                return;
            }
        }
    }

    // Operation completed
    plane->checkpoint_progress = 100;
    runway_print_status("[FINISHED] Plane %d completed %s operation",
                        plane->id, operation_to_string(plane->operation));
}

// Release runway
void runway_release(Plane *plane)
{
    // Clear active plane
    sem_wait(&runway_system.active_plane_sem);
    runway_system.active_plane = NULL;
    sem_post(&runway_system.active_plane_sem);

    // Release runway semaphore
    sem_post(&runway_system.runway_access);

    runway_print_status("[RELEASE] Plane %d released runway", plane->id);
}

// Display final statistics
void runway_display_stats()
{
    runway_print_status("\n========== SIMULATION STATISTICS ==========");
    runway_print_status("Total Planes Processed: %d", runway_system.planes_completed);
    runway_print_status("Emergency Preemptions: %d", runway_system.preemptions_count);
    runway_print_status("Emergency Queue Final: %d", queue_get_count(&runway_system.emergency_queue));
    runway_print_status("Normal Queue Final: %d", queue_get_count(&runway_system.normal_queue));
    runway_print_status("===========================================\n");
}

// Destroy runway system
void runway_destroy(RunwaySystem *sys)
{
    runway_print_status("[SYSTEM] Shutting down runway system...");

    // Destroy queues
    queue_destroy(&sys->emergency_queue);
    queue_destroy(&sys->normal_queue);

    // Destroy semaphores
    sem_destroy(&sys->runway_access);
    sem_destroy(&sys->console_access);
    sem_destroy(&sys->emergency_flag_sem);
    sem_destroy(&sys->emergency_queue_sem);
    sem_destroy(&sys->normal_queue_sem);
    sem_destroy(&sys->active_plane_sem);
    sem_destroy(&sys->completed_sem);
    sem_destroy(&sys->preemptions_sem);

    runway_print_status("[SYSTEM] Runway system shutdown complete");
}
