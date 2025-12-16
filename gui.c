#include "gui.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

GUISystem gui_system;
int gui_enabled = 0;

// Initialize ncurses GUI
void gui_init()
{
    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0);

    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(5, COLOR_WHITE, COLOR_BLUE);
    init_pair(6, COLOR_BLACK, COLOR_GREEN);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
    init_pair(8, COLOR_MAGENTA, COLOR_BLACK);

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    gui_system.header_win = newwin(3, max_x, 0, 0);
    gui_system.runway_win = newwin(17, max_x / 2 - 1, 3, 1);
    gui_system.stats_win = newwin(17, max_x / 2 - 1, 3, max_x / 2 + 1);
    gui_system.emergency_queue_win = newwin(11, max_x / 2 - 1, 20, 1);
    gui_system.normal_queue_win = newwin(11, max_x / 2 - 1, 20, max_x / 2 + 1);
    gui_system.log_win = newwin(max_y - 31, max_x, 31, 0);

    gui_system.log_line = 0;
    gui_system.max_log_lines = max_y - 33;

    sem_init(&gui_system.gui_sem, 0, 1);
    scrollok(gui_system.log_win, TRUE);

    gui_draw_header();
    gui_refresh_all();
    gui_enabled = 1;
}

void gui_draw_header()
{
    sem_wait(&gui_system.gui_sem);
    WINDOW *win = gui_system.header_win;
    wbkgd(win, COLOR_PAIR(5));
    werase(win);
    box(win, 0, 0);
    wattron(win, A_BOLD);
    mvwprintw(win, 1, (COLS - 60) / 2, "  AIRPORT RUNWAY MANAGEMENT SYSTEM - Real-Time Monitor  ");
    wattroff(win, A_BOLD);
    wrefresh(win);
    sem_post(&gui_system.gui_sem);
}

void gui_draw_runway_visual(Plane *plane)
{
    sem_wait(&gui_system.gui_sem);
    WINDOW *win = gui_system.runway_win;
    werase(win);
    box(win, 0, 0);

    wattron(win, COLOR_PAIR(4) | A_BOLD);
    mvwprintw(win, 0, 2, " RUNWAY STATUS ");
    wattroff(win, COLOR_PAIR(4) | A_BOLD);

    if (plane != NULL)
    {
        int color = (plane->priority == EMERGENCY) ? 2 : 1;
        
        mvwhline(win, 2, 2, ACS_HLINE, 40);
        wattron(win, A_BOLD);
        mvwprintw(win, 3, 3, "ACTIVE OPERATION");
        wattroff(win, A_BOLD);
        mvwhline(win, 4, 2, ACS_HLINE, 40);

        mvwprintw(win, 6, 4, "Plane ID:");
        wattron(win, color | A_BOLD);
        mvwprintw(win, 6, 22, "#%d", plane->id);
        wattroff(win, color | A_BOLD);

        mvwprintw(win, 7, 4, "Priority:");
        wattron(win, color | A_BOLD);
        mvwprintw(win, 7, 22, "%s", priority_to_string(plane->priority));
        wattroff(win, color | A_BOLD);

        mvwprintw(win, 8, 4, "Operation:");
        wattron(win, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(win, 8, 22, "%s", operation_to_string(plane->operation));
        wattroff(win, COLOR_PAIR(3) | A_BOLD);

        mvwhline(win, 10, 2, ACS_HLINE, 40);
        int runway_width = 36;
        int plane_pos = (runway_width * plane->checkpoint_progress) / 100;

        mvwaddch(win, 11, 4, ACS_ULCORNER);
        for (int i = 0; i < runway_width; i++)
            mvwaddch(win, 11, 5 + i, ACS_HLINE);
        mvwaddch(win, 11, 5 + runway_width, ACS_URCORNER);

        mvwaddch(win, 12, 4, ACS_VLINE);
        for (int i = 0; i < runway_width; i++)
        {
            if (i == plane_pos)
            {
                wattron(win, COLOR_PAIR(3) | A_BOLD);
                mvwaddstr(win, 12, 5 + i, "*");
                wattroff(win, COLOR_PAIR(3) | A_BOLD);
            }
            else
                mvwaddch(win, 12, 5 + i, ' ');
        }
        mvwaddch(win, 12, 5 + runway_width, ACS_VLINE);

        mvwaddch(win, 13, 4, ACS_LLCORNER);
        for (int i = 0; i < runway_width; i++)
            mvwaddch(win, 13, 5 + i, ACS_HLINE);
        mvwaddch(win, 13, 5 + runway_width, ACS_LRCORNER);

        mvwprintw(win, 15, 3, "Progress:");
        int bar_width = 28;
        int filled = (bar_width * plane->checkpoint_progress) / 100;

        mvwaddch(win, 15, 13, '[');
        wattron(win, COLOR_PAIR(8) | A_BOLD);
        for (int i = 0; i < filled; i++)
            mvwaddch(win, 15, 14 + i, ACS_CKBOARD);
        wattroff(win, COLOR_PAIR(8) | A_BOLD);
        for (int i = filled; i < bar_width; i++)
            mvwaddch(win, 15, 14 + i, ' ');
        mvwaddch(win, 15, 14 + bar_width, ']');
        wattron(win, A_BOLD);
        mvwprintw(win, 15, 16 + bar_width, "%3d%%", plane->checkpoint_progress);
        wattroff(win, A_BOLD);
    }
    else
    {
        wattron(win, COLOR_PAIR(1) | A_BOLD);
        mvwprintw(win, 7, 10, "====================");
        mvwprintw(win, 8, 10, "||  RUNWAY IDLE   ||");
        mvwprintw(win, 9, 10, "====================");
        wattroff(win, COLOR_PAIR(1) | A_BOLD);
        mvwprintw(win, 11, 7, "No active operations");
        mvwprintw(win, 12, 5, "Waiting for next plane...");
    }

    wrefresh(win);
    sem_post(&gui_system.gui_sem);
}

void gui_update_runway(Plane *active_plane)
{
    if (!gui_enabled) return;
    gui_draw_runway_visual(active_plane);
}

void gui_update_queues()
{
    if (!gui_enabled) return;
    sem_wait(&gui_system.gui_sem);

    WINDOW *emerg_win = gui_system.emergency_queue_win;
    werase(emerg_win);
    box(emerg_win, 0, 0);
    wattron(emerg_win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(emerg_win, 0, 2, " EMERGENCY QUEUE ");
    wattroff(emerg_win, COLOR_PAIR(2) | A_BOLD);

    int emerg_count = queue_get_count(&runway_system.emergency_queue);
    wattron(emerg_win, A_BOLD);
    mvwprintw(emerg_win, 1, 3, "Priority: HIGH");
    wattroff(emerg_win, A_BOLD);
    mvwprintw(emerg_win, 1, 22, "| Waiting: ");
    wattron(emerg_win, COLOR_PAIR(2) | A_BOLD);
    wprintw(emerg_win, "%d", emerg_count);
    wattroff(emerg_win, COLOR_PAIR(2) | A_BOLD);
    mvwhline(emerg_win, 2, 1, ACS_HLINE, 42);

    int line = 3;
    QueueNode *node = runway_system.emergency_queue.head;
    int count = 0;
    while (node != NULL && count < 6)
    {
        Plane *p = node->plane;
        wattron(emerg_win, COLOR_PAIR(2) | A_BOLD);
        mvwprintw(emerg_win, line, 3, "%d.", count + 1);
        wattroff(emerg_win, COLOR_PAIR(2) | A_BOLD);
        mvwprintw(emerg_win, line, 6, "Plane #%-2d", p->id);
        wattron(emerg_win, COLOR_PAIR(2));
        mvwprintw(emerg_win, line, 19, "%-7s", operation_to_string(p->operation));
        wattroff(emerg_win, COLOR_PAIR(2));
        if (p->checkpoint_progress > 0)
            mvwprintw(emerg_win, line, 28, "(%d%%)", p->checkpoint_progress);
        line++;
        node = node->next;
        count++;
    }

    if (emerg_count == 0)
    {
        wattron(emerg_win, COLOR_PAIR(1));
        mvwprintw(emerg_win, 5, 10, "-- Queue Empty --");
        wattroff(emerg_win, COLOR_PAIR(1));
    }
    else if (emerg_count > 6)
        mvwprintw(emerg_win, line, 3, "... +%d more", emerg_count - 6);

    wrefresh(emerg_win);

    WINDOW *normal_win = gui_system.normal_queue_win;
    werase(normal_win);
    box(normal_win, 0, 0);
    wattron(normal_win, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(normal_win, 0, 2, " NORMAL QUEUE ");
    wattroff(normal_win, COLOR_PAIR(1) | A_BOLD);

    int normal_count = queue_get_count(&runway_system.normal_queue);
    wattron(normal_win, A_BOLD);
    mvwprintw(normal_win, 1, 3, "Priority: NORMAL");
    wattroff(normal_win, A_BOLD);
    mvwprintw(normal_win, 1, 22, "| Waiting: ");
    wattron(normal_win, COLOR_PAIR(1) | A_BOLD);
    wprintw(normal_win, "%d", normal_count);
    wattroff(normal_win, COLOR_PAIR(1) | A_BOLD);
    mvwhline(normal_win, 2, 1, ACS_HLINE, 42);

    line = 3;
    node = runway_system.normal_queue.head;
    count = 0;
    while (node != NULL && count < 6)
    {
        Plane *p = node->plane;
        wattron(normal_win, COLOR_PAIR(1) | A_BOLD);
        mvwprintw(normal_win, line, 3, "%d.", count + 1);
        wattroff(normal_win, COLOR_PAIR(1) | A_BOLD);
        mvwprintw(normal_win, line, 6, "Plane #%-2d", p->id);
        wattron(normal_win, COLOR_PAIR(1));
        mvwprintw(normal_win, line, 19, "%-7s", operation_to_string(p->operation));
        wattroff(normal_win, COLOR_PAIR(1));
        if (p->checkpoint_progress > 0)
            mvwprintw(normal_win, line, 28, "(%d%%)", p->checkpoint_progress);
        line++;
        node = node->next;
        count++;
    }

    if (normal_count == 0)
    {
        wattron(normal_win, COLOR_PAIR(1));
        mvwprintw(normal_win, 5, 10, "-- Queue Empty --");
        wattroff(normal_win, COLOR_PAIR(1));
    }
    else if (normal_count > 6)
        mvwprintw(normal_win, line, 3, "... +%d more", normal_count - 6);

    wrefresh(normal_win);
    sem_post(&gui_system.gui_sem);
}

void gui_update_stats()
{
    if (!gui_enabled) return;
    sem_wait(&gui_system.gui_sem);

    WINDOW *stats_win = gui_system.stats_win;
    werase(stats_win);
    box(stats_win, 0, 0);
    wattron(stats_win, COLOR_PAIR(4) | A_BOLD);
    mvwprintw(stats_win, 0, 2, " SIMULATION STATISTICS ");
    wattroff(stats_win, COLOR_PAIR(4) | A_BOLD);

    int in_progress = runway_system.total_planes - runway_system.planes_completed;
    mvwhline(stats_win, 2, 2, ACS_HLINE, 40);

    wattron(stats_win, A_BOLD);
    mvwprintw(stats_win, 3, 4, "Total Planes:");
    wattroff(stats_win, A_BOLD);
    wattron(stats_win, COLOR_PAIR(4) | A_BOLD);
    mvwprintw(stats_win, 3, 28, "%3d", runway_system.total_planes);
    wattroff(stats_win, COLOR_PAIR(4) | A_BOLD);

    wattron(stats_win, A_BOLD);
    mvwprintw(stats_win, 5, 4, "Completed:");
    wattroff(stats_win, A_BOLD);
    wattron(stats_win, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(stats_win, 5, 28, "%3d", runway_system.planes_completed);
    wattroff(stats_win, COLOR_PAIR(1) | A_BOLD);

    wattron(stats_win, A_BOLD);
    mvwprintw(stats_win, 6, 4, "In Progress:");
    wattroff(stats_win, A_BOLD);
    wattron(stats_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(stats_win, 6, 28, "%3d", in_progress);
    wattroff(stats_win, COLOR_PAIR(3) | A_BOLD);

    mvwhline(stats_win, 7, 2, ACS_HLINE, 40);

    wattron(stats_win, A_BOLD);
    mvwprintw(stats_win, 8, 4, "Preemptions:");
    wattroff(stats_win, A_BOLD);
    wattron(stats_win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(stats_win, 8, 28, "%3d", runway_system.preemptions_count);
    wattroff(stats_win, COLOR_PAIR(2) | A_BOLD);

    if (runway_system.total_planes > 0)
    {
        int completion_pct = (runway_system.planes_completed * 100) / runway_system.total_planes;
        mvwhline(stats_win, 10, 2, ACS_HLINE, 40);
        wattron(stats_win, A_BOLD);
        mvwprintw(stats_win, 11, 4, "Overall Progress:");
        wattroff(stats_win, A_BOLD);

        int bar_width = 26;
        int filled = (bar_width * completion_pct) / 100;

        mvwaddch(stats_win, 13, 5, '[');
        wattron(stats_win, COLOR_PAIR(1) | A_BOLD);
        for (int i = 0; i < filled; i++)
            mvwaddch(stats_win, 13, 6 + i, ACS_CKBOARD);
        wattroff(stats_win, COLOR_PAIR(1) | A_BOLD);
        for (int i = filled; i < bar_width; i++)
            mvwaddch(stats_win, 13, 6 + i, ' ');
        mvwaddch(stats_win, 13, 6 + bar_width, ']');
        wattron(stats_win, A_BOLD);
        mvwprintw(stats_win, 13, 9 + bar_width, "%3d%%", completion_pct);
        wattroff(stats_win, A_BOLD);
    }

    wrefresh(stats_win);
    sem_post(&gui_system.gui_sem);
}

void gui_log_event(const char *format, ...)
{
    if (!gui_enabled) return;
    sem_wait(&gui_system.gui_sem);

    WINDOW *log_win = gui_system.log_win;
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
    int current_line = (gui_system.log_line % gui_system.max_log_lines) + 1;
    wmove(log_win, current_line, 2);
    wclrtoeol(log_win);
    vw_printw(log_win, format, args);
    va_end(args);

    gui_system.log_line++;
    box(log_win, 0, 0);
    wattron(log_win, COLOR_PAIR(4) | A_BOLD);
    mvwprintw(log_win, 0, 2, " EVENT LOG ");
    wattroff(log_win, COLOR_PAIR(4) | A_BOLD);
    wrefresh(log_win);
    sem_post(&gui_system.gui_sem);
}

void gui_refresh_all()
{
    if (!gui_enabled) return;
    gui_update_runway(runway_system.active_plane);
    gui_update_queues();
    gui_update_stats();
}

void gui_destroy()
{
    if (!gui_enabled) return;
    sem_wait(&gui_system.gui_sem);
    delwin(gui_system.header_win);
    delwin(gui_system.runway_win);
    delwin(gui_system.emergency_queue_win);
    delwin(gui_system.normal_queue_win);
    delwin(gui_system.stats_win);
    delwin(gui_system.log_win);
    endwin();
    sem_post(&gui_system.gui_sem);
    sem_destroy(&gui_system.gui_sem);
    gui_enabled = 0;
}
