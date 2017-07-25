#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <panel.h>
#include <menu.h>
#include <form.h>

/* Declarations */

int init_curses();

ITEM **create_items(const char *from_names[]);
int free_items(ITEM **from_items);

int create_windows();
int setup_menus();
int setup_panels();
int free_menus();
int free_windows();

void signal_handler(int signal);

/* Definitions */

#define HEADER 0
#define DIALOG 1
#define MAXMEN 2
#define EDITOR 2
#define FOOTER 3
#define MAXWIN 4

#define KEY_ESCAPE 0x1b

/* Types */
typedef struct tagMETA {
    MENU *menu;
    PANEL *next;
} META;

/* Variables */
WINDOW *windows[MAXWIN]; // header menu, dialog, text editor, status windows
PANEL *panels[MAXWIN]; // top, middle, bottom, and center panels
PANEL *focus_panel = NULL;
WINDOW *menu_windows[MAXMEN]; // header menu and dialog menu sub-windows
MENU *menus[MAXMEN]; // header menu and dialog menu

META panels_meta[3] = { // HEADER, DIALOG, EDITOR metadata structures
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL}
};

struct tagState {
    bool dialog_visible;
    bool dialog_do_open;
    int dialog_selected;
    int header_selected;
    const char *title_open;
    const char *title_save;
    char *path_open;
    char *path_save;
} menu_state = {false, true, 0, 0, "Open a file", "Save to file", NULL, NULL};

/* Items for the header and the dialog menus */
const char *menu_header[] = {"New", "Open", "Save", "Quit", NULL};
const char *menu_dialog[] = {"Ok", "Cancel", NULL};

/* Terminal size information */
struct winsize terminal_size;

/* Functions */
int main(int argc, char *argv[])
{
    int input = 1; // go inside while loop to get the first input
    PANEL *temp_panel = NULL; // for switching between dialogs and static
    WINDOW *focus_window = NULL;
    MENU *focus_menu = NULL;

    /* Init curses */
    init_curses();

    /* Create windows */
    create_windows();

    /* Add menus */
    setup_menus();

    /* Attach metadata to panels */
    setup_panels();

    /* Update the screen */
    update_panels();
    doupdate();

    /* Do not process input if setting signal failed */
    if (signal(SIGINT, signal_handler) == SIG_ERR) input = 0;

    /* Process input */
    while (input) { // initially input=1
        input = wgetch(stdscr); // blocking call
        focus_window = panel_window(focus_panel);
        focus_menu = ((META *) panel_userptr(focus_panel))->menu;
        switch (input) {
            case 0: break; // end loop (if received via signal)
            case KEY_RESIZE: // intercept SIGWINCH with internal handler
                    free_windows();
                    endwin();
                    refresh();
                    clear();
                    create_windows();
                    setup_menus();
                    setup_panels();
                    break;
            case '\t': // switch panels
                /* Reset highlight */
                if (focus_window == windows[HEADER]) {
                    wattron(focus_window, COLOR_PAIR(1));
                    box(focus_window, 0, 0);
                    wattroff(focus_window, COLOR_PAIR(1));
                }
                /* Switch to the next panel or hide */
                temp_panel = ((META *) panel_userptr(focus_panel))->next;
                if (temp_panel != NULL) {
                    focus_panel = temp_panel; // next panel is ok
                } else {
                    /* Hide panel with no next panel (dialog) */
                    hide_panel(focus_panel);
                    focus_panel = panels[EDITOR]; // return to editor window
                }
                focus_window = panel_window(focus_panel);
                /* Highlight */
                if (focus_window == windows[HEADER]) {
                    wattron(focus_window, COLOR_PAIR(2));
                    box(focus_window, 0, 0);
                    wattroff(focus_window, COLOR_PAIR(2));
                }
                top_panel(focus_panel);
                break;
            case KEY_UP:
                break; // TODO: implement menu actions and frame input
            case KEY_DOWN:
                break; // TODO: implement menu actions and frame input
            case KEY_LEFT:
                if (focus_menu != NULL) {
                    menu_driver(focus_menu, REQ_LEFT_ITEM);
                    break;
                }
            case KEY_RIGHT:
                if (focus_menu != NULL) {
                    menu_driver(focus_menu, REQ_RIGHT_ITEM);
                    break;
                }
            case KEY_ENTER:
                break; // TODO: implement menu actions and frame input
            case KEY_ESCAPE:
                if (focus_menu == menus[HEADER]) {
                    /* Quit only if header menu is focused */
                    input = 0;
                    break;
                }
            case KEY_F(1): // F1: new file
                set_current_item(focus_menu, (menu_items(focus_menu))[0]);
                break;
            case KEY_F(2): // F2: open file
                set_current_item(focus_menu, (menu_items(focus_menu))[1]);
                break;
            case KEY_F(3): // F3: save file
                set_current_item(focus_menu, (menu_items(focus_menu))[2]);
                break;
            case KEY_F(4): // F4: exit program
                set_current_item(focus_menu, (menu_items(focus_menu))[3]);
                input = 0;
                break;
            default:
                /* TODO: forward input to the menus and forms */
                if (focus_menu != NULL) {
                    menu_driver(focus_menu, input);
                }
        }
        /* Always update on changes (TODO: optimize) */
        update_panels();
        doupdate();
        if (!input) sleep (1); // small delay before exit
    }

    /* Release curses resources */
    free_menus(); // detach menus
    free_windows(); // release windows resources
    endwin(); // free_windows works only upon windows and panels

    return 0;
} // int main

int init_curses()
{
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

    return 0;
} // int init_curses

/* This function produces basic UI construction - windows, sub-windows,
 * and panels. This MUST precede setup_* and free_* functions */
int create_windows()
{
    int i = 0, max_x = 0, max_y = 0;

    /* Create windows (height, width, top, left) */
    windows[HEADER] = newwin(3, COLS, 0, 0); // 0-Top
    windows[DIALOG] = newwin(6, COLS / 2, LINES / 2 - 3, COLS / 4); // 1-Center
    windows[EDITOR] = newwin(LINES - 5, COLS, 3, 0); // 2-Middle
    windows[FOOTER] = newwin(2, COLS, LINES - 2, 0); // 3-Bottom

    /* Create sub-windows (first N menu windows) */
    for (i = 0; i < MAXMEN; i ++) {
        getmaxyx(windows[i], max_y, max_x);
        switch (i) {
            case DIALOG:
                menu_windows[i] = derwin(windows[i],
                        1, 16, max_y - 2, max_x / 2 - 8);
                break;
            default:
                menu_windows[i] = derwin(windows[i],
                        max_y - 2, max_x - 2, 1, 1);
        }
    }

    /* Create panels (a panel for every window) */
    for (i = 0; i < MAXWIN; i ++) {
        panels[i] = new_panel(windows[i]);
    }

    /* Draw frames (for some windows) */
    for (i = 0; i < MAXWIN; i ++) {
        switch (i) {
            case HEADER:
            case FOOTER:
            case DIALOG:
                wattron(windows[i], COLOR_PAIR(1));
                box(windows[i], 0, 0); // draw blue frame by default
                wattroff(windows[i], COLOR_PAIR(1));
            default: break;
        }
    }

    return 0;
} // int create_windows

/* This function MUST be called after create_windows function
 * (as it depends on windows and sub-windows) */
int setup_menus()
{
    int i = 0;

    /* Create menus (0 - HEADER, 1 - DIALOG) if none exist*/
    if (menus[HEADER] == NULL)
        menus[HEADER] = new_menu(create_items(menu_header));
    if (menus[DIALOG] == NULL)
        menus[DIALOG] = new_menu(create_items(menu_dialog));

    /* Set menu windows and sub-windows */
    for (i = 0; i < MAXMEN; i ++) {
        /* Extra checks if called before create_windows */
        if (windows[i] != NULL)
            set_menu_win(menus[i], windows[i]);
        if (menu_windows[i] != NULL)
            set_menu_sub(menus[i], menu_windows[i]);
        menu_opts_off(menus[i], O_SHOWDESC); // disable descriptions
        set_menu_mark(menus[i], " "); // set empty mark
    }

    /* Set menu formats (multi-columnar one-line menus) */
    for (i = 0; menu_header[i] != NULL; i ++);
    set_menu_format(menus[HEADER], 1, i);
    for (i = 0; menu_dialog[i] != NULL; i ++);
    set_menu_format(menus[DIALOG], 1, i);

    /* TODO: restore selected item according to the menu_status */

    /* Post menus (with extra check for attached window) */
    for (i = 0; i < MAXMEN; i ++)
        if (windows[i] != NULL)
            post_menu(menus[i]);

    return 0;
} // int setup_menus

/* This function MUST be called after create_windows function
 * and setup_menus function */
int setup_panels()
{
    /* Set user pointer (to the next focus panel and current menu if any) */
    if (panels[HEADER] == NULL) return 1;
    panels_meta[HEADER].menu = menus[HEADER]; // may be NULL
    panels_meta[HEADER].next = panels[EDITOR]; // may be NULL
    set_panel_userptr(panels[HEADER], &panels_meta[HEADER]);

    if (panels[DIALOG] == NULL) return 2;
    panels_meta[DIALOG].menu = menus[DIALOG]; // may be NULL
    panels_meta[DIALOG].next = NULL;
    set_panel_userptr(panels[DIALOG], &panels_meta[DIALOG]);

    if (panels[EDITOR] == NULL) return 3;
    panels_meta[EDITOR].menu = NULL;
    panels_meta[EDITOR].next = panels[HEADER]; // may be NULL
    set_panel_userptr(panels[EDITOR], &panels_meta[EDITOR]);

    /* Update focus */
    if (menu_state.dialog_visible) {
        show_panel(panels[DIALOG]);
        focus_panel = panels[DIALOG];
    } else {
        focus_panel = panels[EDITOR];
        hide_panel(panels[DIALOG]);
    }
    top_panel(focus_panel);

    return 0;
} // int setup_panels

/* This function HAVE TO be called after setup_menus function and SHOULD be
 * called before free_windows function (as it depends on menus) */
int free_menus()
{
    int i = 0;

    /* Unpost, detach and free menus and items */
    for (i = 0; i < MAXMEN; i ++) {
        unpost_menu(menus[i]);
        free_items(menu_items(menus[i]));
        free_menu(menus[i]);
    }

    return i;
} // int free_menus

/* This function HAVE TO be called after create_windows function */
int free_windows()
{
    int i = 0;

    /* Detach menus (if any) before desroying windows */
    for (i = 0; i < MAXMEN; i ++)
        if (menus[i] != NULL)
            unpost_menu(menus[i]);
    /* Destroy windows */
    for (i = 0; i < MAXWIN; i ++) {
        /* Erase borders */
        wborder(windows[i], ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        /* Destroy panel, then window */
        del_panel(panels[i]);
        delwin(windows[i]);
    }

    return i;
} // int free_windows

ITEM **create_items(const char *from_names[])
{
    ITEM **item_list;
    int num_items = 0;

    /* Scan array of strings and stop if NULL-element is found */
    for (; from_names[num_items] != NULL; num_items ++);

    /* Allocate memory for new items */
    item_list = (ITEM **) calloc(num_items + 1, sizeof (ITEM *));
    if (item_list == NULL) { // error allocating memory
        return (ITEM **) NULL;
    }

    /* Create items or return if it's not possible to */
    for (int i = 0; i < num_items; i ++) {
        item_list[i] = new_item(from_names[i], NULL);
        if (item_list[i] == NULL) return item_list; // error creating element
    }

    item_list[num_items] = (ITEM *) NULL;
    return item_list;
} // ITEM **create_items

int free_items(ITEM **from_items)
{
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
} // int free_items

void signal_handler(int signal)
{
    /* Ctrl+C (SIGINT) signal handler */
    switch (signal) {
        case SIGINT:
            ungetch(0);
            break;
        /* TODO: remove this condition (or remove entire handler) */
        case SIGWINCH:
            /* Recreate windows for the new terminal size */
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal_size);
            resizeterm(terminal_size.ws_row, terminal_size.ws_col);
            break;
        default:
            break;
    }
} // void signal_handler

/* vim: set et sw=4: */
