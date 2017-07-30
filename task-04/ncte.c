#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <panel.h>
#include <menu.h>
#include <form.h>

/* Declarations. Macros */

#define HEADER 0
#define DIALOG 1
#define EDITOR 2
#define FOOTER 3

#define MAXMEN 2
#define MAXFOR 2
#define MAXWIN 4

#define EFIELD 0
#define MAXEFI 1

#define DFIELD 1
#define MAXDFI 2

#define MEMO   0
#define PATH   1

#define INPUT  0
#define TITLE  1

#define KEY_TAB       0x09
#define KEY_ESCAPE    0x1b

/* Declarations. Types */

typedef struct tagMETA {
    MENU *menu;
    FORM *edit;
    PANEL *next;
} META;

typedef struct tagFOJA { // 11x4 bytes (44 bytes)
    int height;
    int width;
    int top;
    int left;
    int offscreen;
    int buffers;
    int options;
    unsigned justification;
    unsigned attrib_fore;
    unsigned attrib_back;
    unsigned attrib_pad;
} FOJA; // Field Parameters, Options, Justification, Attributes

int init_curses();

ITEM **create_items(const char *from_names[]);
int free_items(ITEM **from_items);

FIELD **create_fields(FOJA *from_options[]);
int free_fields(FIELD **from_fields);

int create_windows();
int setup_menus();
int setup_forms();
int setup_panels();

int free_forms();
int free_menus();
int free_windows();

void signal_handler(int signal);

/* Definitions. Variables */

WINDOW *windows[MAXWIN]; // header menu, dialog, text editor, status windows
WINDOW *menu_windows[MAXMEN]; // header menu and dialog menu sub-windows
WINDOW *form_windows[MAXFOR]; // editor form and dialog form sub-windows

PANEL *panels[MAXWIN]; // top, middle, bottom, and center panels
PANEL *focus_panel = NULL;

MENU *menus[MAXMEN]; // header menu and dialog menu
FORM *forms[MAXFOR]; // editor and dialog forms

META panels_meta[EDITOR + 1] = { // HEADER, DIALOG, EDITOR metadata structures
    [HEADER] = {NULL, NULL, NULL},
    [DIALOG] = {NULL, NULL, NULL},
    [EDITOR] = {NULL, NULL, NULL}
};

struct tagState { // program state to survive recreations
    bool dialog_visible;
    bool dialog_do_open;
    int dialog_selected;
    int header_selected;
    const char *title_open;
    const char *title_save;
    char *path_to_file;
    char *edit_buffer;
} menu_state = {
    true, true, 0, 0, "Open a file", "Save to file", NULL, NULL
};

/* Items for the header and the dialog menus */
const char *menu_header[] = {
    "F1: New", "F2: Open", "F3: Save", "F4: Quit", NULL
};
const char *menu_dialog[] = {
    "Ok", "Cancel", NULL
};

/* Fields for the editor and the dialog windows */
FOJA field_options[MAXEFI + MAXDFI] = { // maximum EFIELDs + DFIELDs
    /* Editor form fields */
    [EFIELD]     = {1, 0, 0, 0, 0, 0, // MEMO[INPUT]
                    -O_AUTOSKIP, JUSTIFY_LEFT,
                    0, 0, 0},
    /* Dialog form fields */
    [DFIELD]     = {1, 0, 1, 1, 0, 0, // PATH[INPUT]
                    -(O_AUTOSKIP | O_BLANK), 0,
                    0, A_UNDERLINE, 0},
    [DFIELD + 1] = {1, 0, 0, 1, 0, 0, // PATH[TITLE]
                    -O_EDIT, 0,
                    0, 0, 0}
};

FOJA *fields_editor[] = {
    &field_options[EFIELD], NULL
};
FOJA *fields_dialog[] = {
    &field_options[DFIELD], &field_options[DFIELD + 1], NULL
};

/* Terminal size information (TODO: remove if not used) */
struct winsize terminal_size;

/* Definitions. Functions */

int main(int argc, char *argv[])
{
    int input = 1; // go inside while loop to get the first input
    WINDOW *focus_window = NULL;
    MENU *focus_menu = NULL;
    FORM *focus_form = NULL;
    FILE *file;
    int position = 0, length = 0;
    char *path;

    /* Init curses */
    init_curses();

    /* Create windows */
    create_windows();

    /* Add menus */
    setup_menus();

    /* Create forms */
    setup_forms();

    /* Attach metadata to panels */
    setup_panels();

    /* Update the screen */
    update_panels();
    doupdate();

    /* Do not process input if setting signal failed */
    if (signal(SIGINT, signal_handler) == SIG_ERR) input = 0;

    /* Process input */
    while (input) { // initially input = 1
        input = wgetch(stdscr); // blocking call
        focus_window = panel_window(focus_panel);
        focus_menu = ((META *) panel_userptr(focus_panel))->menu;
        focus_form = ((META *) panel_userptr(focus_panel))->edit;
        /* Main (keypress processing) multiway branch */
        switch (input) {
            case 0: break; // end loop (if received via wgetch)
            case KEY_RESIZE: // intercept SIGWINCH with internal handler
                free_forms();
                free_windows();
                endwin();
                refresh();
                clear();
                create_windows();
                setup_menus();
                setup_forms();
                setup_panels();
                break;
            case KEY_ESCAPE: // switch panels
                if (focus_panel == panels[DIALOG])
                    input = KEY_MAX + 6; // dialog -> cancel
                /* Reset highlight */
                if (focus_window == windows[HEADER]) {
                    wattron(focus_window, COLOR_PAIR(1));
                    box(focus_window, 0, 0);
                    wattroff(focus_window, COLOR_PAIR(1));
                }
                /* Switch to the next panel or hide */
                focus_panel = ((META *) panel_userptr(focus_panel))->next;
                if (focus_panel == NULL)
                    focus_panel = panels[EDITOR]; // return to editor window
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
                if (focus_form == forms[0]) {
                    form_driver(focus_form, REQ_PREV_LINE);
                }
                break;
            case KEY_DOWN:
                if (focus_form == forms[0]) { // TODO: limit movement
                    form_driver(focus_form, REQ_NEXT_LINE);
                }
                break;
            case KEY_LEFT: // select next menu item or field character
                if (focus_menu == menus[HEADER]) {
                    menu_driver(focus_menu, REQ_LEFT_ITEM);
                    menu_state.header_selected =
                        item_index(current_item(focus_menu));
                } else if (focus_form != NULL) {
                    form_driver(focus_form, REQ_PREV_CHAR);
                }
                break;
            case KEY_RIGHT: // select next menu item or field character
                if (focus_menu == menus[HEADER]) {
                    menu_driver(focus_menu, REQ_RIGHT_ITEM);
                    menu_state.header_selected =
                        item_index(current_item(focus_menu));
                } else if (focus_form != NULL) { // TODO: limit movement
                    form_driver(focus_form, REQ_NEXT_CHAR);
                }
                break;
            case KEY_TAB:
                if (focus_menu == menus[HEADER]) {
                    menu_state.header_selected ++; // next item
                    menu_state.header_selected &= 3; // limit up to 4
                    set_current_item(menus[HEADER],
                                     (menu_items(menus[HEADER]))
                                     [menu_state.header_selected]);
                } else if (focus_menu == menus[DIALOG]) {
                    menu_state.dialog_selected ++; // next item
                    menu_state.dialog_selected &= 1; // limit up to 2
                    set_current_item(menus[DIALOG],
                                     (menu_items(menus[DIALOG]))
                                     [menu_state.dialog_selected]);
                }
                break;
            case KEY_BACKSPACE:
            case 0x08: // backspace character (ASCII BS)
            case 0x7f: // backspace character (ASCII DEL)
                if (focus_form != NULL)
                    form_driver(focus_form, REQ_DEL_PREV);
                break;
            case KEY_DC: // delete character (0x014a)
                if (focus_form != NULL)
                    form_driver(focus_form, REQ_DEL_CHAR);
                break;
            case KEY_ENTER:
            case 0x0a: // Line feed key (Enter)
                if (focus_form == forms[0]) {
                    form_driver(focus_form, REQ_NEW_LINE);
                } else if (focus_menu == menus[HEADER]) {
                    switch (item_index(current_item(focus_menu))) {
                        case 0: // header -> new
                            input = KEY_MAX + 1;
                            break;
                        case 1: // header -> open
                            input = KEY_MAX + 2;
                            break;
                        case 2: // header -> save
                            input = KEY_MAX + 3;
                            break;
                        case 3: // header -> quit
                            input = KEY_MAX + 4;
                        default: break;
                    }
                } else if (focus_menu == menus[DIALOG]) {
                    switch (item_index(current_item(focus_menu))) {
                        case 0: // dialog -> ok
                            input = KEY_MAX + 5; // process open/save
                            ungetch(KEY_ESCAPE); // simulate ESC keypress
                            break;
                        case 1: // dialog -> cancel
                            ungetch(KEY_ESCAPE); // simulate ESC keypress
                            //input = KEY_MAX + 6;
                        default: break;
                    }
                }
                break;
            case KEY_F(1): // F1: new file
                /* Select respective menu item (visual effect only) */
                if (focus_menu != menus[DIALOG]) {
                    set_current_item(menus[HEADER],
                                     (menu_items(menus[HEADER]))[0]);
                    input = KEY_MAX + 1;
                }
                break;
            case KEY_F(2): // F2: open file
                /* Select respective menu item (visual effect only) */
                if (focus_menu != menus[DIALOG]) {
                    set_current_item(menus[HEADER],
                                     (menu_items(menus[HEADER]))[1]);
                    input = KEY_MAX + 2;
                }
                break;
            case KEY_F(3): // F3: save file
                /* Select respective menu item (visual effect only) */
                if (focus_menu != menus[DIALOG]) {
                    set_current_item(menus[HEADER],
                                     (menu_items(menus[HEADER]))[2]);
                    input = KEY_MAX + 3;
                }
                break;
            case KEY_F(4): // F4: exit program
                /* Select respective menu item (visual effect only) */
                set_current_item(menus[HEADER],
                                 (menu_items(menus[HEADER]))[3]);
                input = KEY_MAX + 4;
                break;
            case 0x20 ... 0x7e: // printable ASCII characters
                if (focus_form != NULL) {
                    form_driver(focus_form, input);
                }
            default: break;
        }

        /* Additional (menu processing) multiway branch */
        switch (input) {
            case KEY_MAX + 1: // header menu -> new (F1)
                form_driver(forms[MEMO], REQ_CLR_FIELD);
                break;
            case KEY_MAX + 2: // header menu -> open (F2)
                /* Remember state */
                menu_state.dialog_visible = true;
                menu_state.dialog_do_open = true;
                menu_state.header_selected = 1;
                /* Set dialog title */
                set_field_buffer((form_fields(forms[PATH]))[TITLE],
                                 0, menu_state.title_open);
                /* Restore path to open a file */
                if (menu_state.path_to_file != NULL)
                    set_field_buffer((form_fields(forms[PATH]))[INPUT],
                                     0, menu_state.path_to_file);
                /* Store last panel to dialog's metadata */
                ((META *) panel_userptr(panels[DIALOG]))->next = focus_panel;
                /* Popup dialog */
                show_panel(panels[DIALOG]);
                focus_panel = panels[DIALOG];
                break;
            case KEY_MAX + 3: // header menu -> save (F3)
                /* Remember state */
                menu_state.dialog_visible = true;
                menu_state.dialog_do_open = false;
                menu_state.header_selected = 2;
                /* Set dialog title */
                set_field_buffer((form_fields(forms[PATH]))[TITLE],
                                 0, menu_state.title_save);
                /* Restore path to save a file */
                if (menu_state.path_to_file != NULL)
                    set_field_buffer((form_fields(forms[PATH]))[INPUT],
                                     0, menu_state.path_to_file);
                /* Store last panel to dialog's metadata */
                ((META *) panel_userptr(panels[DIALOG]))->next = focus_panel;
                /* Popup dialog */
                show_panel(panels[DIALOG]);
                focus_panel = panels[DIALOG];
                break;
            case KEY_MAX + 4: // header menu -> quit (F4)
                input = 0; // prevent next cycle iteration
                break;
            case KEY_MAX + 5: // dialog menu -> ok
                /* Request validation of the main editor field */
                form_driver(forms[PATH], REQ_VALIDATION);
                path = field_buffer((form_fields(forms[PATH]))[INPUT], 0);
                /* Find first space */
                for (position = 0; path[position] != ' '; position ++);
                path[position] = '\0'; // temporarily terminate buffer
                if (menu_state.dialog_do_open) { // read from a file
                    file = fopen(path, "r");
                    if (file) {
                        fseek(file, 0, SEEK_END);
                        length = ftell(file);
                        fseek(file, 0, SEEK_SET);
                        menu_state.edit_buffer = malloc(length);
                        if (menu_state.edit_buffer) {
                            fread(menu_state.edit_buffer, 1, length, file);
                        }
                        fclose(file);
                    }
                    /* Display read data in editor window (memo form) */
                    if (menu_state.edit_buffer)
                        set_field_buffer((form_fields(forms[MEMO]))[INPUT],
                                0, menu_state.edit_buffer);
                } else { // write to a file
                    menu_state.edit_buffer =
                        field_buffer((form_fields(forms[MEMO]))[INPUT], 0);
                    file = fopen(path, "w+");
                    if (file) {
                        fprintf(file, "%s", menu_state.edit_buffer);
                        fclose(file);
                    }
                }
                path[position] = ' '; // restore terminated space in buffer
                ungetch(KEY_ESCAPE);
                break;
            case KEY_MAX + 6: // dialog menu -> cancel
                /* Request validation of the main editor field */
                form_driver(forms[PATH], REQ_VALIDATION);

                hide_panel(panels[DIALOG]);
                menu_state.dialog_visible = false;

                /* Remember field buffer */
                menu_state.path_to_file =
                    field_buffer((form_fields(forms[PATH]))[INPUT], 0);
                /* Detach field buffer */
                set_field_buffer((form_fields(forms[PATH]))[INPUT], 0, NULL);
            default: break;
        }
        /* Always update on changes (TODO: optimize) */
        update_panels();
        doupdate();
        if (!input) sleep (1); // small delay before exit
    }

    /* Release curses resources */
    free_forms(); // release forms resources
    free_menus(); // detach and free  menus
    free_windows(); // release windows and panels resources
    endwin();

    /* Release dynamically allocated pointers */
    if (menu_state.path_to_file != NULL) free(menu_state.path_to_file);
    if (menu_state.edit_buffer != NULL) free(menu_state.edit_buffer);

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

    /* Create menu sub-windows (first N menu windows) */
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

    /* Create form sub-windows (may not match with menu sub-windows) */
    getmaxyx(windows[EDITOR], max_y, max_x);
    form_windows[0] = derwin(windows[EDITOR], max_y, max_x, 0, 0);
    getmaxyx(windows[DIALOG], max_y, max_x);
    form_windows[1] = derwin(windows[DIALOG], 3, max_x - 2, 1, 1);

    /* Create panels (a panel for every window) */
    for (i = 0; i < MAXWIN; i ++) {
        panels[i] = new_panel(windows[i]);
    }

    /* Draw frames (for some (sub) windows) */
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

    /* Create menus (0 - HEADER, 1 - DIALOG) if none exist */
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

/* This function MUST be called after create_windows function,
 * setup_menus, and setup_forms functions */
int setup_panels()
{
    /* Set user pointer (to the next focus panel and current menu if any) */
    if (panels[HEADER] == NULL) return 1;
    panels_meta[HEADER].menu = menus[HEADER]; // may be NULL
    panels_meta[HEADER].edit = NULL;
    panels_meta[HEADER].next = panels[EDITOR]; // may be NULL
    set_panel_userptr(panels[HEADER], &panels_meta[HEADER]);

    if (panels[DIALOG] == NULL) return 2;
    panels_meta[DIALOG].menu = menus[DIALOG]; // may be NULL
    panels_meta[DIALOG].edit = forms[PATH];
    panels_meta[DIALOG].next = NULL;
    set_panel_userptr(panels[DIALOG], &panels_meta[DIALOG]);

    if (panels[EDITOR] == NULL) return 3;
    panels_meta[EDITOR].menu = NULL;
    panels_meta[EDITOR].edit = forms[MEMO];
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

int setup_forms()
{
    int i = 0, max_x = 0, max_y = 0;

    /* Create forms (0 > MEMO > EDITOR, 1 > PATH > DIALOG) if none exist */
    if (forms[MEMO] == NULL) {
        getmaxyx(form_windows[MEMO], max_y, max_x);
        field_options[EFIELD].height = max_y;
        field_options[EFIELD].width = max_x;
        forms[MEMO] = new_form(create_fields(fields_editor));
        /* Restore editor buffer (if it was previously saved) */
        if (menu_state.edit_buffer != NULL)
            set_field_buffer((form_fields(forms[MEMO]))[INPUT],
                             0, menu_state.edit_buffer);
    }
    if (forms[PATH] == NULL) {
        getmaxyx(form_windows[PATH], max_y, max_x);
        field_options[DFIELD].width = max_x - 2; // INPUT
        field_options[DFIELD + 1].width = max_x - 2; // TITLE
        forms[PATH] = new_form(create_fields(fields_dialog));
        /* Restore dialog state if it is visible */
        if (menu_state.dialog_visible) {
        /* Dialog title */
            if (menu_state.dialog_do_open)
                set_field_buffer((form_fields(forms[PATH]))[TITLE],
                                 0, menu_state.title_open);
            else
                set_field_buffer((form_fields(forms[PATH]))[TITLE],
                                 0, menu_state.title_save);
        /* Dialog buffer if it does exist */
            if (menu_state.path_to_file != NULL)
                set_field_buffer((form_fields(forms[PATH]))[INPUT],
                                 0, menu_state.path_to_file);
        }
    }

    /* Set form windows and sub-windows */
    if (windows[EDITOR] != NULL) {
        set_form_win(forms[MEMO], windows[EDITOR]);
        /* Set form_windows[0] for forms[0] (if not NULL) */
        if (form_windows[MEMO] != NULL)
            set_form_sub(forms[MEMO], form_windows[MEMO]);
        /* Post editor form */
        post_form(forms[MEMO]);
        i ++; // successfully posted form counter
    }

    if (windows[DIALOG] != NULL) {
        set_form_win(forms[PATH], windows[DIALOG]);
        /* Set form_windows[1] for forms[1] (if not NULL) */
        if (form_windows[PATH] != NULL)
            set_form_sub(forms[PATH], form_windows[PATH]);
        /* Post dialog form */
        post_form(forms[PATH]);
        i ++; // successfully posted form counter
    }

    return i; // number of successfully posted forms
} // int setup_forms

int free_forms()
{
    int i = 0;
    FIELD *editor = (form_fields(forms[MEMO]))[INPUT];
    FIELD *dialog = (form_fields(forms[PATH]))[INPUT];

    /* Save editor buffer prior destruction */
    form_driver(forms[MEMO], REQ_VALIDATION); // validate unsaved changes
    menu_state.edit_buffer = field_buffer(editor, 0);
    set_field_buffer(editor, 0, NULL); // fake buffer to be deallocated

    /* Save dialog buffer prior destruction */
    form_driver(forms[PATH], REQ_VALIDATION); // validate unsaved changes
    menu_state.path_to_file = field_buffer(dialog, 0);
    set_field_buffer(dialog, 0, NULL); // fake buffer to be deallocated

    /* Unpost, detach and free menus and items */
    for (i = 0; i < MAXFOR; i ++) {
        unpost_form(forms[i]);
        free_fields(form_fields(forms[i]));
        free_form(forms[i]);
        forms[i] = NULL; // clear invalid pointer
    }

    return i;
} // int free_forms

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
        /* Free the item (item name points to global array variable
         * description is already NULL, item_userptr is unset) */
        free_item(from_items[num_free]);
        num_free ++;
    }
    return num_free;
} // int free_items

FIELD **create_fields(FOJA *from_options[])
{
    FIELD **field_list;
    FOJA *foja = NULL;
    int num_fields = 0;

    /* Scan array of strings and stop if NULL-element is found */
    for (; from_options[num_fields] != NULL; num_fields ++);

    /* Allocate memory for new items */
    field_list = (FIELD **) calloc(num_fields + 1, sizeof (FIELD *));
    if (field_list == NULL) { // error allocating memory
        return (FIELD **) NULL;
    }

    /* Create items or return if it's not possible to */
    for (int i = 0; i < num_fields; i ++) {
        foja = from_options[i];
        field_list[i] = new_field(foja->height, foja->width,
                foja->top, foja->left,
                foja->offscreen, foja->buffers);
        if (field_list[i] == NULL) {
            return field_list; // error creating element (return NULL)
        } else { // configure created field
            if (foja->options)
                if (foja->options > 0) // set positive, unset negative
                    field_opts_on(field_list[i], foja->options);
                else
                    field_opts_off(field_list[i], foja->options);
            if (foja->justification)
                set_field_just(field_list[i], foja->justification);
            if (foja->attrib_fore)
                set_field_fore(field_list[i], foja->attrib_fore);
            if (foja->attrib_back)
                set_field_back(field_list[i], foja->attrib_back);
            if (foja->attrib_pad)
                set_field_fore(field_list[i], foja->attrib_pad);
        }
    }

    field_list[num_fields] = (FIELD *) NULL;
    return field_list;
} // FIELD **create_fields

int free_fields(FIELD **from_fields)
{
    int num_free = 0;
    while (from_fields[num_free] != NULL) {
        /* Free the field */
        free_field(from_fields[num_free]);
        num_free ++;
    }
    return num_free;
} // int free_fields

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
