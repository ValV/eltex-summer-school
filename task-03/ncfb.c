#include <stdlib.h>
#include <limits.h>
#include <dirent.h>
#include <panel.h>
#include <menu.h>

#define LEFT 0
#define RIGHT 1
#define BOTTOM 2

typedef struct tagMETA {
    MENU *menu;
    PANEL *next;
} META;

typedef struct dirent DIRENT;

ITEM **create_items(char *from_path);
int free_items(ITEM **from_items);

int main(int argc, char *argv[]) {
    /* Variables */
    WINDOW *windows[3];
    WINDOW *focus_window;
    PANEL *panels[3];
    PANEL *focus_panel;
    WINDOW *menu_windows[2];
    MENU *menus[2];
    MENU *temp_menu;
    ITEM *temp_item;
    META *panels_meta[2] = {
        (META *) malloc(sizeof (struct tagMETA)),
        (META *) malloc(sizeof (struct tagMETA))
    };
    int input = 0, num_items = 0, max_x = 0, max_y = 0, i = 0;
    char path_left[PATH_MAX], path_right[PATH_MAX];
    char real_path[PATH_MAX], *temp_path;
    char *menu_selection = ">";

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
    realpath(".", path_left);
    menus[LEFT] = new_menu(create_items(path_left));
    set_menu_userptr(menus[LEFT], path_left);
    set_menu_mark(menus[LEFT], menu_selection);

    realpath(".", path_right);
    menus[RIGHT] = new_menu(create_items(path_right));
    set_menu_userptr(menus[RIGHT], path_right);
    set_menu_mark(menus[RIGHT], menu_selection);

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
    wattron(focus_window, COLOR_PAIR(2));
    box(focus_window, 0, 0);
    wattroff(focus_window, COLOR_PAIR(2));

    /* Post menus */
    for (i = 0; i < 2; i ++)
        post_menu(menus[i]);

    /* Update the screen */
    update_panels();
    doupdate();

    while ((input = getch()) != 'q') {
        switch (input) {
            case 'a':
            case 'd':
            case '\t':
            case KEY_LEFT:
            case KEY_RIGHT:
                /* Reset highlight */
                wattron(focus_window, COLOR_PAIR(1));
                box(focus_window, 0, 0);
                wattroff(focus_window, COLOR_PAIR(1));
                /* Switch to the next panel */
                focus_panel = ((META *) panel_userptr(focus_panel))->next;
                focus_window = panel_window(focus_panel);
                top_panel(focus_panel);
                /* Highlight */
                wattron(focus_window, COLOR_PAIR(2));
                box(focus_window, 0, 0);
                wattroff(focus_window, COLOR_PAIR(2));
                break;
            case 'w':
            case KEY_UP:
                menu_driver(((META *) panel_userptr(focus_panel))->menu,
                        REQ_UP_ITEM);
                break;
            case 's':
            case KEY_DOWN:
                menu_driver(((META *) panel_userptr(focus_panel))->menu,
                        REQ_DOWN_ITEM);
                break;
            case 'e':
            case KEY_ENTER:
                /* Get temporary facilities */
                temp_menu = ((META *) panel_userptr(focus_panel))->menu;
                temp_item = current_item(temp_menu);
                if (((DIRENT *) item_userptr(temp_item))->d_type == DT_DIR) {
                    /* Get current menu path and canonicalize it */
                    temp_path = (char *) menu_userptr(temp_menu);
                    realpath(temp_path, real_path);
                    /* Append file (directory) name to the path */
                    sprintf(temp_path, "%s/%s",
                            real_path, item_name(temp_item));
                    /* Hide menu */
                    unpost_menu(temp_menu);
                    /* Destroy old menu items */
                    free_items(menu_items(temp_menu));
                    /* Set new menu items */
                    set_menu_items(temp_menu, create_items(temp_path));
                    /* Show menu */
                    post_menu(temp_menu);
                }
                break;
            default:
                /* Forward input to the panel window */
                break;
        }
        //wrefresh(focus_window);
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

ITEM **create_items(char *from_path) {
    ITEM **item_list;
    int num_items = 0;
    struct dirent **name_list;

    /* Read directory from_path (memory for name_list will be allocated) */
    num_items = scandir(from_path, &name_list, NULL, alphasort);
    if (num_items == -1) return (ITEM **) NULL; // error scanning directory

    /* Allocate memory for new items */
    item_list = (ITEM **) calloc(num_items + 1, sizeof (ITEM *));
    if (item_list == NULL) { // error allocating memory
        free(name_list);
        return (ITEM **) NULL;
    }

    /* Create items or return if it's not possible to */
    for (int i = 0; i < num_items; i ++) {
        item_list[i] = new_item(name_list[i]->d_name, NULL);
        if (item_list[i] == NULL) return item_list; // error creating element

        /* Set user pointer to the pointer of the dirent structure */
        set_item_userptr(item_list[i], name_list[i]);
    }
    free(name_list);

    item_list[num_items] = (ITEM *) NULL;
    return item_list;
}

int free_items(ITEM **from_items) {
    int num_free = 0;
    while (from_items[num_free] != (ITEM *) NULL) {
        /* Free every dirent structure of the item */
        free((struct dirent *) item_userptr(from_items[num_free]));
        /* Free the item (item name is already NULL, description points
         * to menu_userptr, which is global array variable) */
        free_item(from_items[num_free]);
        num_free ++;
    }
    return num_free;
}

/* vim: set et sw=4: */
