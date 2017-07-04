#include <stdlib.h>
#include <panel.h>
#include <menu.h>

#define LEFT 0
#define RIGHT 1
#define BOTTOM 2
#define ELNUM 5

typedef struct tagMETA {
    MENU *menu;
    PANEL *next;
} META;

char *data_a[] = {
    "Element A",
    "Element B",
    "Element C",
    "Element D",
    "Element E",
};

char *data_b[] = {
    "Element 1",
    "Element 2",
    "Element 3",
    "Element 4",
    "Element 5",
};

ITEM **create_items(char **from_array);
int free_items(ITEM **from_items);

int main(int argc, char *argv[]) {
    /* Variables */
    WINDOW *windows[3];
    WINDOW *focus_window;
    PANEL *panels[3];
    PANEL *focus_panel;
    WINDOW *menu_windows[2];
    MENU *menus[2];
    META *panels_meta[2] = {
        (META *) malloc(sizeof (struct tagMETA)),
        (META *) malloc(sizeof (struct tagMETA))
    };
    int input = 0, num_items = 0, max_x = 0, max_y = 0, i = 0;

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
    windows[0] = newwin(LINES - 3, COLS / 2, 0, 0); // LEFT
    windows[1] = newwin(LINES - 3, COLS / 2, 0, COLS / 2); // RIGHT
    windows[2] = newwin(3, COLS, LINES - 3, 0); // BOTTOM

    /* Draw frames */
    for (i = 0; i < 3; i ++) {
        wattron(windows[i], COLOR_PAIR(1));
        box(windows[i], 0, 0);
        wattroff(windows[i], COLOR_PAIR(1));
    }

    /* Create menus */
    menus[LEFT] = new_menu(create_items(data_a));
    menus[RIGHT] = new_menu(create_items(data_b));

    /* Create menu sub-windows */
    for (i = 0; i < 2; i ++) {
        getmaxyx(windows[i], max_y, max_x);
        menu_windows[i] = derwin(windows[i], max_y - 2, max_x - 2, 1, 1);
    }

    /* Set menu windows and sub-windows */
    for (i = 0; i < 2; i ++) {
        set_menu_win(menus[i], windows[i]);
        set_menu_sub(menus[i], menu_windows[i]);
    }

    /* Create panels */
    for (i = 0; i < 3; i ++) {
        panels[i] = new_panel(windows[i]);
    }

    /* Set user pointer to the next focus panel */
    panels_meta[LEFT]->menu = menus[LEFT];
    panels_meta[LEFT]->next = panels[RIGHT];
    set_panel_userptr(panels[LEFT], panels_meta[LEFT]);
    panels_meta[RIGHT]->menu = menus[RIGHT];
    panels_meta[RIGHT]->next = panels[LEFT];
    set_panel_userptr(panels[RIGHT], panels_meta[RIGHT]);

    /* Update focus */
    focus_panel = panels[LEFT];
    focus_window = panel_window(focus_panel);
    top_panel(focus_panel);
    //wmove(focus_window, 1, 1);
    wattron(focus_window, COLOR_PAIR(2));
    box(focus_window, 0, 0);
    wattroff(focus_window, COLOR_PAIR(2));

    /* Update the screen */
    update_panels();
    doupdate();

    /* Post menus */
    for (i = 0; i < 2; i ++) {
        post_menu(menus[i]);
        wrefresh(windows[i]);
    }

    while ((input = getch()) != 'q') {
        switch (input) {
            case '\t':
                /* Reset highlight */
                wattron(focus_window, COLOR_PAIR(1));
                box(focus_window, 0, 0);
                wattroff(focus_window, COLOR_PAIR(1));
                /* Switch to the next panel */
                focus_panel = ((META *) panel_userptr(focus_panel))->next;
                focus_window = panel_window(focus_panel);
                top_panel(focus_panel);
                //wmove(focus_window, 1, 1);
                /* Highlight */
                wattron(focus_window, COLOR_PAIR(2));
                box(focus_window, 0, 0);
                wattroff(focus_window, COLOR_PAIR(2));
                break;
            case KEY_UP:
                menu_driver(((META *) panel_userptr(focus_panel))->menu,
                        REQ_UP_ITEM);
                break;
            case KEY_DOWN:
                menu_driver(((META *) panel_userptr(focus_panel))->menu,
                        REQ_DOWN_ITEM);
                break;
            default:
                /* Forward input to the panel window */
                break;
        }
        wrefresh(focus_window);
        update_panels();
        doupdate();
    }
    /* Unpost, detach and free menus and items */
    for (i = 0; i < 2; i ++) {
        unpost_menu(menus[i]);
        free_items(menu_items(menus[i]));
        free_menu(menus[i]);
    }
    endwin();

    return 0;
}

ITEM **create_items(char **from_array) {
    ITEM **item_list;
    int num_items = 0;
    /* Allocate memory for new items */
    num_items = ELNUM;
    item_list = (ITEM **) calloc(num_items + 1, sizeof (ITEM *));
    if (item_list == NULL) {
        /* Error allocating memory */
        return (ITEM **) NULL;
    }
    /* Create items */
    for (int i = 0; i < num_items; i ++) {
        /* Create item or return if can't do more */
        if ((item_list[i] = new_item(from_array[i], from_array[i])) == NULL) {
            /* Error creating element - return that is done */
            return item_list;
        }
        /* Set user pointer if any */
        set_item_userptr(item_list[i], (void *) NULL);
    }
    item_list[num_items] = (ITEM *) NULL;
    return item_list;
}

int free_items(ITEM **from_items) {
    int num_free = 0;
    while (from_items[num_free] != (ITEM *) NULL) {
        free_item(from_items[num_free]);
        num_free ++;
    }
    return num_free;
}
/* vim: set et sw=4: */
