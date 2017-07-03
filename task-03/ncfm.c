#include <panel.h>

int main(int argc, char *argv[]) {
    /* Variables */
    WINDOW *windows[3];
    WINDOW *focus_window;
    PANEL *panels[3];
    PANEL *focus_panel;
    int input = 0;

    /* Init curses */
    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    /* Init colors */
    init_pair(1, COLOR_BLUE, 0);
    init_pair(2, COLOR_WHITE, 0);
    init_pair(3, COLOR_CYAN, 0);

    /* Create windows */
    windows[0] = newwin(LINES - 3, COLS / 2, 0, 0);
    windows[1] = newwin(LINES - 3, COLS / 2, 0, COLS / 2);
    windows[2] = newwin(3, COLS, LINES - 3, 0);

    /* Draw frames */
    wattron(windows[0], COLOR_PAIR(1));
    wattron(windows[1], COLOR_PAIR(1));
    wattron(windows[2], COLOR_PAIR(1));
    box(windows[0], 0, 0);
    box(windows[1], 0, 0);
    box(windows[2], 0, 0);
    wattroff(windows[0], COLOR_PAIR(1));
    wattroff(windows[1], COLOR_PAIR(1));
    wattroff(windows[2], COLOR_PAIR(1));

    /* Create panels */
    panels[0] = new_panel(windows[0]);
    panels[1] = new_panel(windows[1]);
    panels[2] = new_panel(windows[2]);

    /* Set user pointer to the next focus panel */
    set_panel_userptr(panels[0], panels[1]);
    set_panel_userptr(panels[1], panels[0]);

    /* Update focus */
    focus_panel = panels[0];
    focus_window = panel_window(focus_panel);
    top_panel(focus_panel);
    wmove(focus_window, 1, 1);
    wattron(focus_window, COLOR_PAIR(2));
    box(focus_window, 0, 0);
    wattroff(focus_window, COLOR_PAIR(2));

    /* Update the screen */
    update_panels();
    doupdate();

    while ((input = getch()) != 'q') {
        switch (input) {
            case '\t':
                /* Reset highlight */
                wattron(focus_window, COLOR_PAIR(1));
                box(focus_window, 0, 0);
                wattroff(focus_window, COLOR_PAIR(1));
                /* Switch to the next panel */
                focus_panel = (PANEL *) panel_userptr(focus_panel);
                focus_window = panel_window(focus_panel);
                top_panel(focus_panel);
                wmove(focus_window, 1, 1);
                /* Highlight */
                wattron(focus_window, COLOR_PAIR(2));
                box(focus_window, 0, 0);
                wattroff(focus_window, COLOR_PAIR(2));
                break;
            default:
                /* Forward input to the panel window */
                break;
        }
        update_panels();
        doupdate();
    }
    endwin();

    return 0;
}
/* vim: set et sw=4: */
