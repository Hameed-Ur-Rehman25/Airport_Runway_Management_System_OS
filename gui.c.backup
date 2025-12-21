#include "gui.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

GUISystem gui_system;
int gui_enabled = 0;

// Initialize ncurses GUI
void gui_init()
{
    // Initialize ncurses
    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0); // Hide cursor

    // Initialize color pairs
    init_pair(1, COLOR_GREEN, COLOR_BLACK);  // Normal
    init_pair(2, COLOR_RED, COLOR_BLACK);    // Emergency
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); // Warning/Active
    init_pair(4, COLOR_CYAN, COLOR_BLACK);   // Info
    init_pair(5, COLOR_WHITE, COLOR_BLUE);   // Header
    init_pair(6, COLOR_BLACK, COLOR_WHITE);  // Runway

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // Create windows
    gui_system.header_win = newwin(3, max_x, 0, 0);
    gui_system.runway_win = newwin(12, max_x / 2, 3, 0);
    gui_system.emergency_queue_win = newwin(12, max_x / 2, 3, max_x / 2);
    gui_system.normal_queue_win = newwin(8, max_x / 2, 15, 0);
    gui_system.stats_win = newwin(8, max_x / 2, 15, max_x / 2);
    gui_system.log_win = newwin(max_y - 23, max_x, 23, 0);

    gui_system.log_line = 0;
    gui_system.max_log_lines = max_y - 25;

    // Initialize GUI semaphore
    sem_init(&gui_system.gui_sem, 0, 1);

    // Enable scrolling for log window
    scrollok(gui_system.log_win, TRUE);

    gui_draw_header();
    gui_refresh_all();

    gui_enabled = 1;
}

// Draw header
void gui_draw_header()
{
    sem_wait(&gui_system.gui_sem);

    wbkgd(gui_system.header_win, COLOR_PAIR(5));
    box(gui_system.header_win, 0, 0);

    mvwprintw(gui_system.header_win, 1, 2, "AIRPORT RUNWAY MANAGEMENT SYSTEM - Real-Time Visualization");

    wrefresh(gui_system.header_win);

    sem_post(&gui_system.gui_sem);
}

// Draw runway visual
void gui_draw_runway_visual(Plane *plane)
{
    sem_wait(&gui_system.gui_sem);

    WINDOW *win = gui_system.runway_win;
    werase(win);
    box(win, 0, 0);

    wattron(win, COLOR_PAIR(4) | A_BOLD);
    mvwprintw(win, 1, 2, "RUNWAY STATUS");
    wattroff(win, COLOR_PAIR(4) | A_BOLD);

    // Draw runway
    mvwprintw(win, 3, 2, "=============================================");
    mvwprintw(win, 4, 2, "||                                         ||");
    mvwprintw(win, 5, 2, "||             R U N W A Y                 ||");
    mvwprintw(win, 6, 2, "||                                         ||");
    mvwprintw(win, 7, 2, "=============================================");

    if (plane != NULL)
    {
        int color = (plane->priority == EMERGENCY) ? 2 : 1;
        wattron(win, COLOR_PAIR(color) | A_BOLD);

        if (plane->operation == LANDING)
        {
            mvwprintw(win, 5, 8, "✈ Plane %d LANDING (%d%%)", plane->id, plane->checkpoint_progress);
        }
        else
        {
            mvwprintw(win, 5, 8, "Plane %d TAKEOFF (%d%%) ✈", plane->id, plane->checkpoint_progress);
        }

        wattroff(win, COLOR_PAIR(color) | A_BOLD);

        // Progress bar
        int bar_width = 39;
        int filled = (bar_width * plane->checkpoint_progress) / 100;

        mvwprintw(win, 9, 2, "Progress: [");
        wattron(win, COLOR_PAIR(3));
        for (int i = 0; i < filled; i++)
        {
            wprintw(win, "=");
        }
        wattroff(win, COLOR_PAIR(3));
        for (int i = filled; i < bar_width; i++)
        {
            wprintw(win, " ");
        }
        wprintw(win, "] %d%%", plane->checkpoint_progress);
    }
    else
    {
        wattron(win, COLOR_PAIR(1));
        mvwprintw(win, 5, 15, "RUNWAY AVAILABLE");
        wattroff(win, COLOR_PAIR(1));

        mvwprintw(win, 9, 2, "Status: IDLE - Waiting for planes...");
    }

    wrefresh(win);
    sem_post(&gui_system.gui_sem);
}

// Update runway display
void gui_update_runway(Plane *active_plane)
{
    if (!gui_enabled)
        return;
    gui_draw_runway_visual(active_plane);
}

// Update queue displays
void gui_update_queues()
{
    if (!gui_enabled)
        return;

    sem_wait(&gui_system.gui_sem);

    // Emergency Queue Window
    WINDOW *emerg_win = gui_system.emergency_queue_win;
    werase(emerg_win);
    box(emerg_win, 0, 0);

    wattron(emerg_win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(emerg_win, 1, 2, "EMERGENCY QUEUE");
    wattroff(emerg_win, COLOR_PAIR(2) | A_BOLD);

    int emerg_count = queue_get_count(&runway_system.emergency_queue);
    mvwprintw(emerg_win, 2, 2, "Waiting: %d plane(s)", emerg_count);

    // Display queue items
    int line = 4;
    QueueNode *node = runway_system.emergency_queue.head;
    int count = 0;
    while (node != NULL && count < 6)
    {
        Plane *p = node->plane;
        wattron(emerg_win, COLOR_PAIR(2));
        mvwprintw(emerg_win, line++, 2, "⚠ Plane %d - %s",
                  p->id, operation_to_string(p->operation));
        wattroff(emerg_win, COLOR_PAIR(2));
        node = node->next;
        count++;
    }

    if (emerg_count == 0)
    {
        wattron(emerg_win, COLOR_PAIR(1));
        mvwprintw(emerg_win, 4, 2, "(Empty)");
        wattroff(emerg_win, COLOR_PAIR(1));
    }

    wrefresh(emerg_win);

    // Normal Queue Window
    WINDOW *normal_win = gui_system.normal_queue_win;
    werase(normal_win);
    box(normal_win, 0, 0);

    wattron(normal_win, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(normal_win, 1, 2, "NORMAL QUEUE");
    wattroff(normal_win, COLOR_PAIR(1) | A_BOLD);

    int normal_count = queue_get_count(&runway_system.normal_queue);
    mvwprintw(normal_win, 2, 2, "Waiting: %d plane(s)", normal_count);

    // Display queue items
    line = 4;
    node = runway_system.normal_queue.head;
    count = 0;
    while (node != NULL && count < 3)
    {
        Plane *p = node->plane;
        wattron(normal_win, COLOR_PAIR(1));
        mvwprintw(normal_win, line++, 2, "✈ Plane %d - %s",
                  p->id, operation_to_string(p->operation));
        wattroff(normal_win, COLOR_PAIR(1));
        node = node->next;
        count++;
    }

    if (normal_count == 0)
    {
        wattron(normal_win, COLOR_PAIR(1));
        mvwprintw(normal_win, 4, 2, "(Empty)");
        wattroff(normal_win, COLOR_PAIR(1));
    }

    wrefresh(normal_win);

    sem_post(&gui_system.gui_sem);
}

// Update statistics
void gui_update_stats()
{
    if (!gui_enabled)
        return;

    sem_wait(&gui_system.gui_sem);

    WINDOW *stats_win = gui_system.stats_win;
    werase(stats_win);
    box(stats_win, 0, 0);

    wattron(stats_win, COLOR_PAIR(4) | A_BOLD);
    mvwprintw(stats_win, 1, 2, "STATISTICS");
    wattroff(stats_win, COLOR_PAIR(4) | A_BOLD);

    mvwprintw(stats_win, 3, 2, "Total Planes: %d", runway_system.total_planes);
    mvwprintw(stats_win, 4, 2, "Completed: %d", runway_system.planes_completed);
    mvwprintw(stats_win, 5, 2, "Preemptions: %d", runway_system.preemptions_count);
    mvwprintw(stats_win, 6, 2, "In Progress: %d",
              runway_system.total_planes - runway_system.planes_completed);

    wrefresh(stats_win);

    sem_post(&gui_system.gui_sem);
}

// Log event to GUI
void gui_log_event(const char *format, ...)
{
    if (!gui_enabled)
        return;

    sem_wait(&gui_system.gui_sem);

    WINDOW *log_win = gui_system.log_win;

    // First clear and redraw box on first line
    if (gui_system.log_line == 0)
    {
        werase(log_win);
        box(log_win, 0, 0);
        wattron(log_win, COLOR_PAIR(4) | A_BOLD);
        mvwprintw(log_win, 0, 2, " EVENT LOG ");
        wattroff(log_win, COLOR_PAIR(4) | A_BOLD);
    }

    va_list args;
    va_start(args, format);

    // Move to next line (starting from line 1, leaving room for box)
    int current_line = (gui_system.log_line % gui_system.max_log_lines) + 1;
    wmove(log_win, current_line, 2);
    wclrtoeol(log_win); // Clear the line

    vw_printw(log_win, format, args);
    va_end(args);

    gui_system.log_line++;

    box(log_win, 0, 0); // Redraw box
    wattron(log_win, COLOR_PAIR(4) | A_BOLD);
    mvwprintw(log_win, 0, 2, " EVENT LOG ");
    wattroff(log_win, COLOR_PAIR(4) | A_BOLD);

    wrefresh(log_win);

    sem_post(&gui_system.gui_sem);
}

// Refresh all windows
void gui_refresh_all()
{
    if (!gui_enabled)
        return;

    gui_update_runway(runway_system.active_plane);
    gui_update_queues();
    gui_update_stats();
}

// Destroy GUI
void gui_destroy()
{
    if (!gui_enabled)
        return;

    sem_wait(&gui_system.gui_sem);

    // Delete windows
    delwin(gui_system.header_win);
    delwin(gui_system.runway_win);
    delwin(gui_system.emergency_queue_win);
    delwin(gui_system.normal_queue_win);
    delwin(gui_system.stats_win);
    delwin(gui_system.log_win);

    // End ncurses
    endwin();

    sem_post(&gui_system.gui_sem);
    sem_destroy(&gui_system.gui_sem);

    gui_enabled = 0;
}
