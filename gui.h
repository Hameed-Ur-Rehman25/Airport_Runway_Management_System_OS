#ifndef GUI_H
#define GUI_H

#include <ncurses.h>
#include <semaphore.h>
#include "plane.h"
#include "runway.h"

// GUI Windows
typedef struct
{
    WINDOW *header_win;
    WINDOW *runway_win;
    WINDOW *emergency_queue_win;
    WINDOW *normal_queue_win;
    WINDOW *status_win;
    WINDOW *stats_win;
    WINDOW *log_win;
    int log_line;
    int max_log_lines;
    sem_t gui_sem; // Protect GUI updates
} GUISystem;

// Global GUI instance
extern GUISystem gui_system;
extern int gui_enabled;

// GUI functions
void gui_init();
void gui_destroy();
void gui_update_runway(Plane *active_plane);
void gui_update_queues();
void gui_update_stats();
void gui_log_event(const char *format, ...);
void gui_draw_header();
void gui_draw_runway_visual(Plane *plane);
void gui_refresh_all();

#endif // GUI_H
