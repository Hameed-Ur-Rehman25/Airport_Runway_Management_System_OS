#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "runway.h"
#include "plane.h"
#include "gui.h"

#define DEFAULT_TOTAL_PLANES 10
#define DEFAULT_EMERGENCY_PROBABILITY 15 // 15%
#define DEFAULT_LANDING_DURATION 8       // 8 seconds for better visualization
#define DEFAULT_TAKEOFF_DURATION 6       // 6 seconds for better visualization

// Function to generate random operation type
OperationType random_operation()
{
    return (rand() % 2 == 0) ? LANDING : TAKEOFF;
}

// Function to determine if plane is emergency based on probability
PriorityLevel random_priority(int emergency_prob)
{
    return (rand() % 100 < emergency_prob) ? EMERGENCY : NORMAL;
}

// Display usage information
void print_usage(const char *program_name)
{
    printf("Airport Runway Management System Simulator\n");
    printf("==========================================\n\n");
    printf("Usage: %s [options]\n\n", program_name);
    printf("Options:\n");
    printf("  -n <number>    Total number of planes (default: %d)\n", DEFAULT_TOTAL_PLANES);
    printf("  -e <percent>   Emergency probability 0-100 (default: %d%%)\n", DEFAULT_EMERGENCY_PROBABILITY);
    printf("  -l <seconds>   Landing duration (default: %d seconds)\n", DEFAULT_LANDING_DURATION);
    printf("  -t <seconds>   Takeoff duration (default: %d seconds)\n", DEFAULT_TAKEOFF_DURATION);
    printf("  -g             Enable GUI mode (ncurses visualization)\n");
    printf("  -h             Display this help message\n\n");
    printf("Example:\n");
    printf("  %s -n 20 -e 20 -l 6 -t 4\n", program_name);
    printf("  (Simulate 20 planes with 20%% emergency, 6s landing, 4s takeoff)\n\n");
}

int main(int argc, char *argv[])
{
    // Default parameters
    int total_planes = DEFAULT_TOTAL_PLANES;
    int emergency_prob = DEFAULT_EMERGENCY_PROBABILITY;
    int landing_duration = DEFAULT_LANDING_DURATION;
    int takeoff_duration = DEFAULT_TAKEOFF_DURATION;
    int use_gui = 0;

    // Parse command-line arguments
    int opt;
    while ((opt = getopt(argc, argv, "n:e:l:t:gh")) != -1)
    {
        switch (opt)
        {
        case 'n':
            total_planes = atoi(optarg);
            if (total_planes <= 0)
            {
                fprintf(stderr, "Error: Number of planes must be positive\n");
                return 1;
            }
            break;
        case 'e':
            emergency_prob = atoi(optarg);
            if (emergency_prob < 0 || emergency_prob > 100)
            {
                fprintf(stderr, "Error: Emergency probability must be between 0 and 100\n");
                return 1;
            }
            break;
        case 'l':
            landing_duration = atoi(optarg);
            if (landing_duration <= 0)
            {
                fprintf(stderr, "Error: Landing duration must be positive\n");
                return 1;
            }
            break;
        case 't':
            takeoff_duration = atoi(optarg);
            if (takeoff_duration <= 0)
            {
                fprintf(stderr, "Error: Takeoff duration must be positive\n");
                return 1;
            }
            break;
        case 'g':
            use_gui = 1;
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    // Seed random number generator
    srand(time(NULL));

    // Initialize GUI if requested
    if (use_gui)
    {
        gui_init();
        usleep(500000); // Give GUI time to initialize
    }

    // Print simulation parameters
    if (!use_gui)
    {
        printf("\n");
        printf("╔══════════════════════════════════════════════════════════╗\n");
        printf("║   AIRPORT RUNWAY MANAGEMENT SYSTEM SIMULATION           ║\n");
        printf("╚══════════════════════════════════════════════════════════╝\n");
        printf("\n");
        printf("Simulation Parameters:\n");
        printf("  • Total Planes: %d\n", total_planes);
        printf("  • Emergency Probability: %d%%\n", emergency_prob);
        printf("  • Landing Duration: %d seconds\n", landing_duration);
        printf("  • Takeoff Duration: %d seconds\n", takeoff_duration);
        printf("  • Checkpoint Interval: 500ms (for preemption checks)\n");
        printf("\n");
        printf("═══════════════════════════════════════════════════════════\n\n");
    }

    // Initialize runway system
    runway_init(&runway_system, landing_duration, takeoff_duration);
    runway_system.total_planes = total_planes;

    // Allocate array for planes
    Plane *planes = (Plane *)malloc(total_planes * sizeof(Plane));
    if (planes == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate memory for planes\n");
        return 1;
    }

    // Create and initialize planes
    if (!use_gui)
    {
        printf("[SETUP] Creating %d planes...\n", total_planes);
    }
    else
    {
        gui_log_event("[SETUP] Creating %d planes...", total_planes);
    }
    for (int i = 0; i < total_planes; i++)
    {
        OperationType op = random_operation();
        PriorityLevel priority = random_priority(emergency_prob);
        plane_init(&planes[i], i + 1, op, priority);
    }

    if (!use_gui)
    {
        printf("[SETUP] All planes created. Starting simulation...\n\n");
        sleep(1);
    }
    else
    {
        gui_log_event("[SETUP] All planes created. Starting simulation...");
        gui_refresh_all();
        sleep(1);
    }

    // Spawn plane threads with staggered arrival
    for (int i = 0; i < total_planes; i++)
    {
        if (pthread_create(&planes[i].thread, NULL, plane_thread_function, &planes[i]) != 0)
        {
            fprintf(stderr, "Error: Failed to create thread for plane %d\n", i + 1);
            return 1;
        }

        // Stagger arrivals for better visualization (1-3 seconds between planes)
        usleep((rand() % 2000 + 1000) * 1000); // 1s to 3s
    }

    // Wait for all planes to complete
    if (!use_gui)
    {
        printf("\n[SYSTEM] Waiting for all planes to complete...\n\n");
    }
    else
    {
        gui_log_event("[SYSTEM] Waiting for all planes to complete...");
    }
    for (int i = 0; i < total_planes; i++)
    {
        pthread_join(planes[i].thread, NULL);
        plane_destroy(&planes[i]);
    }

    // Display final statistics
    if (!use_gui)
    {
        printf("\n");
        runway_display_stats();
    }
    else
    {
        gui_log_event("");
        gui_log_event("========== SIMULATION COMPLETE ==========");
        gui_log_event("Press any key to exit...");
        gui_refresh_all();

        // Wait for keypress
        timeout(-1);
        getch();

        // Cleanup GUI
        gui_destroy();

        // Show stats in console after GUI closes
        printf("\n");
        runway_display_stats();
    }

    // Cleanup
    runway_destroy(&runway_system);
    free(planes);

    if (!use_gui)
    {
        printf("Simulation completed successfully!\n\n");
    }
    else
    {
        printf("Simulation completed successfully!\n\n");
    }

    return 0;
}
