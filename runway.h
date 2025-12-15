#ifndef RUNWAY_H
#define RUNWAY_H

#include <semaphore.h>
#include "plane.h"
#include "queue.h"

// Runway configuration
typedef struct
{
    int landing_duration; // seconds
    int takeoff_duration; // seconds
} RunwayConfig;

// Global runway state
typedef struct
{
    sem_t runway_access;       // Binary semaphore for runway access
    sem_t console_access;      // Binary semaphore for console output
    sem_t emergency_flag_sem;  // Binary semaphore to protect emergency flag
    sem_t emergency_queue_sem; // Counting semaphore for emergency queue
    sem_t normal_queue_sem;    // Counting semaphore for normal queue

    int emergency_flag;     // Flag to signal active plane to yield
    Plane *active_plane;    // Currently using runway
    sem_t active_plane_sem; // Protect active_plane pointer

    Queue emergency_queue;
    Queue normal_queue;

    RunwayConfig config;

    int total_planes;
    int planes_completed;
    sem_t completed_sem; // Protect planes_completed counter
    int preemptions_count;
    sem_t preemptions_sem; // Protect preemptions_count
} RunwaySystem;

// Global runway system instance
extern RunwaySystem runway_system;

// Runway functions
void runway_init(RunwaySystem *sys, int landing_duration, int takeoff_duration);
void runway_destroy(RunwaySystem *sys);
void runway_request_access(Plane *plane);
void runway_perform_operation(Plane *plane);
void runway_release(Plane *plane);
void runway_print_status(const char *format, ...);
void runway_display_stats();

#endif // RUNWAY_H
