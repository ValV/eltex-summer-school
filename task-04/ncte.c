#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <panel.h>
#include <menu.h>
#include <form.h>

/* Declaration. Macros */

#define KEY_TAB       0x09
#define KEY_ESCAPE    0x1b


/* Declaration. Types */

/* Panel meta data structure to hold links */
typedef struct tagMETA {
    MENU *menu;
    FORM *edit;
    PANEL *next;
} META;

/* Field options structure (arguments, options, justification, attributes) */
typedef struct tagFOJA {
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
} FOJA; // 11x4 bytes (44 bytes)

/* Structure to hold new lines for edit buffer (TODO) */
typedef struct tagLines {
    char *line_start;
    char *line_end;
    struct tagLines *line_pred;
    struct tagLines *line_post;
} LINES;

/* Program state structure to survive recreations */
typedef struct tagState {
    bool dialog_visible;
    bool dialog_do_open;
    int dialog_selected;
    int header_selected;
    const char *title_open;
    const char *title_save;
    char *path_to_file;
    char *edit_buffer;
} STATE;


/* Declaration. Functions */

/* Auxiliary functions */

ITEM **create_items(const char *from_names[]);
int free_items(ITEM **from_items);
FIELD **create_fields(FOJA *from_options[]);
int free_fields(FIELD **from_fields);


/* Ncurses setup functions */

int init_curses();
int create_windows();
int setup_menus();
int setup_forms();
int setup_panels();
int free_forms();
int free_menus();
int free_windows();


/* Definition. Variables */

/*
 * WINDOWS (windows and panels main set)
 */

/* Main windows enumeration set */
enum {
    window_header,
    window_dialog,
    window_editor,
    window_footer,
    window_count
} window_index;

/* Main window list (array of pointers) */
WINDOW *windows[window_count];

/* Main panel list (a panel for each window) */
PANEL *panels[window_count];

/* Panel metadata list (header, dialog, editor) */
META panels_meta[window_editor + 1] = {
    [window_header] = {NULL, NULL, NULL},
    [window_dialog] = {NULL, NULL, NULL},
    [window_editor] = {NULL, NULL, NULL}
};

/*
 * MENUS (menus, auxiliary subwindows, and menu items)
 *
 * Names menu_attributes, menu_back, menu_cursor, menu_driver, menu_fore,
 * menu_format, menu_grey, menu_hook, menu_init, menu_items, menu_mark,
 * menu_new, menu_opts*, menu_pad, menu_pattern, menu_post, menu_request*,
 * menu_spacing, menu_sub, menu_term, menu_userptr, and menu_win must not
 * be used since they are part of ncurses menu library.
 */

/* Menus enumeration set */
enum {
    menu_header,
    menu_dialog,
    menu_count
} menu_index;

/* Menus list (array of pointers) */
MENU *menus[menu_count];

/* Menu subwindows list (one subwindow for each menu) */
WINDOW *menu_windows[menu_count];

/* Items (build from array of strings by create_items function) */
const char *items_header[] = {
    "F1: New", "F2: Open", "F3: Save", "F4: Quit", NULL
};

const char *items_dialog[] = {
    "Ok", "Cancel", NULL
};

/*
 * FORMS (forms, auxiliary subwindows, and fields)
 *
 * Names form_cursor, form_data, form_driver*, form_field*, form_hook,
 * form_init, form_new*, form_opts*, form_page, form_post, form_request*,
 * form_sub, form_term, form_userptr, form_variables, and form_win must not
 * be used since they are part of ncurses form library.
 *
 * Names field_arg, field_back, field_buffer, field_count, field_fore,
 * field_index, field_info, field_init, field_just, field_opts*, field_pad,
 * field_status, field_term, field_type, and field_userptr must not be used
 * since they are part of ncurses form library.
 */

/* Forms enumeration set */
enum {
    form_editor,
    form_dialog,
    form_count
} form_index;

/* Forms list (array of pointers) */
FORM *forms[form_count];

/* Form subwindows list (a subwindow for each form) */
WINDOW *form_windows[form_count];

/* Field enumeration sets (omni field indexing and relative to a form) */
enum {
    field_editor_input,
    field_dialog_input,
    field_dialog_title,
    field_total_count
};

enum {
    field_input,
    field_title
};

/* Field options (array of initialized structures) */
FOJA field_options[field_total_count] = {
    [field_editor_input] = {
        1, 0, 0, 0, 0, 0, -O_AUTOSKIP, JUSTIFY_LEFT, 0, 0, 0
    },
    [field_dialog_input] = {
        1, 0, 1, 1, 0, 0, -(O_AUTOSKIP | O_BLANK), 0, 0, A_UNDERLINE, 0
    },
    [field_dialog_title] = {
        1, 0, 0, 1, 0, 0, -O_EDIT, 0, 0, 0, 0
    }
};

/* Fields (arrays of links for each form) */
FOJA *fields_editor[] = {
    [field_input] = &field_options[field_editor_input],
    NULL
};

FOJA *fields_dialog[] = {
    [field_input] = &field_options[field_dialog_input],
    [field_title] = &field_options[field_dialog_title],
    NULL
};

/*
 * STATE variables (variables to save current state for entire program)
 */

/* Focus panel (link to currently selected panel) */
PANEL *focus_panel = NULL;

/* Menu state (state of menus and buffers) */
STATE menu_state = {
    true, true, 0, 0, "Open a file", "Save to file", NULL, NULL
};

/* Terminal size information (TODO: remove if not used) */
struct winsize terminal_size;


/* Definition. Functions */

/*
 * Main function
 *
 * This function returns zero if the program does exit correctly.
 */
int main(int argc, char *argv[])
{
    /* Variables. Selected frames */
    WINDOW *focus_window = NULL;
    MENU *focus_menu = NULL;
    FORM *focus_form = NULL;
    /* Variables. External file */
    FILE *file;
    int position = 0, length = 0;
    char *path;
    /* Variables. Loop control */
    int input = 1;

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

    /* Process input (initially input == 1) */
    while (input) {
        /* Blocking call for user input */
        input = wgetch(stdscr);
        /* Get currently selected frames (windows, menus, forms) */
        focus_window = panel_window(focus_panel);
        focus_menu = ((META *) panel_userptr(focus_panel))->menu;
        focus_form = ((META *) panel_userptr(focus_panel))->edit;

        /* Main (keypress processing) multiway branch */
        switch (input) {
            /* End loop if 0 has been received via wgetch */
            case 0: break;
            case KEY_RESIZE:
                /* Ncurses internal SIGWINCH handler */
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
            case KEY_ESCAPE:
                /* Switch panels routine */
                if (focus_panel == panels[window_dialog]) {
                    /* Forward to "dialog -> cancel" routine */
                    input = KEY_MAX + 6;
                }
                /* Clear highlight for header menu */
                if (focus_window == windows[window_header]) {
                    wattron(focus_window, COLOR_PAIR(1));
                    box(focus_window, 0, 0);
                    wattroff(focus_window, COLOR_PAIR(1));
                }
                /* Switch to the next panel or hide */
                focus_panel = ((META *) panel_userptr(focus_panel))->next;
                /* Return to editor window if no next panel */
                if (focus_panel == NULL) {
                    focus_panel = panels[window_editor];
                }
                focus_window = panel_window(focus_panel);
                /* Set highlight if new window is the header */
                if (focus_window == windows[window_header]) {
                    wattron(focus_window, COLOR_PAIR(2));
                    box(focus_window, 0, 0);
                    wattroff(focus_window, COLOR_PAIR(2));
                }
                top_panel(focus_panel);
                break;
            case KEY_UP:
                /* Navigate previous line in editor form's active field */
                if (focus_form == forms[form_editor]) {
                    form_driver(focus_form, REQ_PREV_LINE);
                }
                break;
            case KEY_DOWN:
                /* Navigate next line in editor form's active field */
                if (focus_form == forms[form_editor]) {
                    /* TODO: limit movement */
                    form_driver(focus_form, REQ_NEXT_LINE);
                }
                break;
            case KEY_LEFT:
                /* Select previous menu item or field character */
                if (focus_menu == menus[menu_header]) {
                    menu_driver(focus_menu, REQ_LEFT_ITEM);
                    menu_state.header_selected =
                        item_index(current_item(focus_menu));
                } else if (focus_form != NULL) {
                    form_driver(focus_form, REQ_PREV_CHAR);
                }
                break;
            case KEY_RIGHT:
                /* Select next menu item or field character */
                if (focus_menu == menus[menu_header]) {
                    menu_driver(focus_menu, REQ_RIGHT_ITEM);
                    menu_state.header_selected =
                        item_index(current_item(focus_menu));
                } else if (focus_form != NULL) {
                    /* TODO: limit movement */
                    form_driver(focus_form, REQ_NEXT_CHAR);
                }
                break;
            case KEY_TAB:
                /* Menu item cycling */
                if (focus_menu == menus[menu_header]) {
                    menu_state.header_selected ++; // next item
                    menu_state.header_selected &= 3; // limit up to 4
                    set_current_item(menus[menu_header],
                                     (menu_items(menus[menu_header]))
                                     [menu_state.header_selected]);
                } else if (focus_menu == menus[menu_dialog]) {
                    menu_state.dialog_selected ++; // next item
                    menu_state.dialog_selected &= 1; // limit up to 2
                    set_current_item(menus[menu_dialog],
                                     (menu_items(menus[menu_dialog]))
                                     [menu_state.dialog_selected]);
                }
                break;
            case KEY_BACKSPACE:
            case 0x08: // ASCII BS
            case 0x7f: // ASCII DEL
                /* Backspace (delete previous characterin a field) */
                if (focus_form != NULL) {
                    form_driver(focus_form, REQ_DEL_PREV);
                }
                break;
            case KEY_DC: // 0x014a
                /* Delete (delete current character in a field) */
                if (focus_form != NULL) {
                    form_driver(focus_form, REQ_DEL_CHAR);
                }
                break;
            case KEY_ENTER:
            case 0x0a: // ASCII LF
                if (focus_form == forms[form_editor]) {
                    form_driver(focus_form, REQ_NEW_LINE);
                } else if (focus_menu == menus[menu_header]) {
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
                } else if (focus_menu == menus[menu_dialog]) {
                    switch (item_index(current_item(focus_menu))) {
                        case 0:
                            /* Forward to "dialog -> ok" routine and close
                             * via ESC keypress simulation */
                            input = KEY_MAX + 5;
                            ungetch(KEY_ESCAPE);
                            break;
                        case 1:
                            /* Forward to "dialog -> cancel" routine 
                             * via ESC keypress simulation */
                            ungetch(KEY_ESCAPE);
                        default: break;
                    }
                }
                break;
            case KEY_F(1):
                /* F1: new file - select menu item (visual effect only) */
                if (focus_menu != menus[menu_dialog]) {
                    /* Set from any window except dialog */
                    set_current_item(menus[menu_header],
                                     (menu_items(menus[menu_header]))[0]);
                    input = KEY_MAX + 1;
                }
                break;
            case KEY_F(2):
                /* F2: open file - select menu item (visual effect only) */
                if (focus_menu != menus[menu_dialog]) {
                    /* Set from any window except dialog */
                    set_current_item(menus[menu_header],
                                     (menu_items(menus[menu_header]))[1]);
                    input = KEY_MAX + 2;
                }
                break;
            case KEY_F(3):
                /* F3: save file - select menu item (visual effect only) */
                if (focus_menu != menus[menu_dialog]) {
                    /* Set from any window except dialog */
                    set_current_item(menus[menu_header],
                                     (menu_items(menus[menu_header]))[2]);
                    input = KEY_MAX + 3;
                }
                break;
            case KEY_F(4): // F4: exit program
                /* F4: exit program - select menu item from any window */
                set_current_item(menus[menu_header],
                                 (menu_items(menus[menu_header]))[3]);
                input = KEY_MAX + 4;
                break;
            case 0x20 ... 0x7e:
                /* Pass printable ASCII characters to a form's active field */
                if (focus_form != NULL) {
                    form_driver(focus_form, input);
                }
            default: break;
        }

        /* Additional (menu processing) multiway branch */
        switch (input) {
            case KEY_MAX + 1: // header menu -> new (F1)
                form_driver(forms[form_editor], REQ_CLR_FIELD);
                break;
            case KEY_MAX + 2: // header menu -> open (F2)
                /* Remember state */
                menu_state.dialog_visible = true;
                menu_state.dialog_do_open = true;
                menu_state.header_selected = 1;
                /* Set dialog title */
                set_field_buffer((
                                     form_fields(forms[form_dialog])
                                 )[field_title],
                                 0, menu_state.title_open);
                /* Restore path to open a file */
                if (menu_state.path_to_file != NULL) {
                    set_field_buffer((
                                         form_fields(forms[form_dialog])
                                     )[field_input],
                                     0, menu_state.path_to_file);
                }
                /* Store last panel to dialog's metadata */
                (
                    (META *) panel_userptr(panels[window_dialog])
                )->next = focus_panel;
                /* Popup dialog */
                show_panel(panels[window_dialog]);
                focus_panel = panels[window_dialog];
                break;
            case KEY_MAX + 3: // header menu -> save (F3)
                /* Remember state */
                menu_state.dialog_visible = true;
                menu_state.dialog_do_open = false;
                menu_state.header_selected = 2;
                /* Set dialog title */
                set_field_buffer((
                                     form_fields(forms[form_dialog])
                                 )[field_title],
                                 0, menu_state.title_save);
                /* Restore path to save a file */
                if (menu_state.path_to_file != NULL) {
                    set_field_buffer((
                                         form_fields(forms[form_dialog])
                                     )[field_input],
                                     0, menu_state.path_to_file);
                }
                /* Store last panel to dialog's metadata */
                (
                    (META *) panel_userptr(panels[window_dialog])
                )->next = focus_panel;
                /* Popup dialog */
                show_panel(panels[window_dialog]);
                focus_panel = panels[window_dialog];
                break;
            case KEY_MAX + 4: // header menu -> quit (F4)
                /* Prevent next cycle iteration */
                input = 0;
                break;
            case KEY_MAX + 5: // dialog menu -> ok
                /* Request validation of the main editor field */
                form_driver(forms[form_dialog], REQ_VALIDATION);
                path = field_buffer((
                                        form_fields(forms[form_dialog])
                                    )[field_input], 0);
                /* Find first space and temporarily treminate it */
                for (position = 0; path[position] != ' '; position ++);
                path[position] = '\0';
                /* Read from a file */
                if (menu_state.dialog_do_open) {
                    file = fopen(path, "r");
                    if (file) {
                        fseek(file, 0, SEEK_END);
                        length = ftell(file);
                        fseek(file, 0, SEEK_SET);
                        free (menu_state.edit_buffer);
                        menu_state.edit_buffer = malloc(length);
                        if (menu_state.edit_buffer) {
                            fread(menu_state.edit_buffer, 1, length, file);
                        }
                        fclose(file);
                    }
                    /* Display read data in editor window (memo form) */
                    if (menu_state.edit_buffer) {
                        set_field_buffer((
                                             form_fields(forms[form_editor])
                                         )[field_input],
                                         0, menu_state.edit_buffer);
                    }
                } else {
                    /* Write to a file */
                    free(menu_state.edit_buffer);
                    menu_state.edit_buffer =
                        field_buffer((
                                         form_fields(forms[form_editor])
                                     )[field_input], 0);
                    file = fopen(path, "w+");
                    if (file) {
                        fprintf(file, "%s", menu_state.edit_buffer);
                        fclose(file);
                    }
                }
                /* Restore terminated space in buffer and exit via ESC */
                path[position] = ' ';
                ungetch(KEY_ESCAPE);
                break;
            case KEY_MAX + 6: // dialog menu -> cancel
                /* Request validation of the main editor field */
                form_driver(forms[form_dialog], REQ_VALIDATION);

                hide_panel(panels[window_dialog]);
                menu_state.dialog_visible = false;

                /* Remember field buffer */
                menu_state.path_to_file =
                    field_buffer((
                                     form_fields(forms[form_dialog])
                                 )[field_input], 0);
                /* Detach field buffer */
                set_field_buffer((
                                     form_fields(forms[form_dialog])
                                 )[field_input], 0, NULL);
            default: break;
        }
        /* Always update on changes (TODO: optimize) */
        update_panels();
        doupdate();
        if (!input) {
            /* Small delay before exit */
            sleep (1);
        }
    }

    /* Release curses resources */
    free_forms();
    free_menus();
    free_windows();
    endwin();

    /* Release dynamically allocated pointers */
    free(menu_state.path_to_file);
    free(menu_state.edit_buffer);

    return 0;
} /* int main */

/*
 * Initialize curses function
 *
 * This function initializes ncurses graphical (pseudo) context, set color,
 * char-by-char getting, silent, and extended keys modes. Initializes color
 * pairs.
 * This function always returns zero.
 */
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
} /* int init_curses */

/* 
 * Create windows function
 *
 * This function produces basic UI construction - windows, sub-windows,
 * and panels. This MUST precede setup_* and free_* functions.
 * This function always returns zero.
 */
int create_windows()
{
    int i = 0, max_x = 0, max_y = 0;

    /* Create windows (height, width, top, left) */
    windows[window_header] = newwin(3, COLS, 0, 0); // 0-Top
    windows[window_dialog] = newwin(6, COLS / 2, LINES / 2 - 3, COLS / 4); // 1-Center
    windows[window_editor] = newwin(LINES - 5, COLS, 3, 0); // 2-Middle
    windows[window_footer] = newwin(2, COLS, LINES - 2, 0); // 3-Bottom

    /* Create menu sub-windows (first N menu windows) */
    for (i = 0; i < menu_count; i ++) {
        getmaxyx(windows[i], max_y, max_x);
        switch (i) {
            case menu_dialog:
                menu_windows[i] = derwin(windows[i],
                        1, 16, max_y - 2, max_x / 2 - 8);
                break;
            default:
                menu_windows[i] = derwin(windows[i],
                        max_y - 2, max_x - 2, 1, 1);
        }
    }

    /* Create form sub-windows (may not match with menu sub-windows) */
    getmaxyx(windows[window_editor], max_y, max_x);
    form_windows[form_editor] =
        derwin(windows[window_editor], max_y, max_x, 0, 0);
    getmaxyx(windows[window_dialog], max_y, max_x);
    form_windows[form_dialog] =
        derwin(windows[window_dialog], 3, max_x - 2, 1, 1);

    /* Create panels (a panel for every window) */
    for (i = 0; i < window_count; i ++) {
        panels[i] = new_panel(windows[i]);
    }

    /* Draw frames (for some (sub) windows) */
    for (i = 0; i < window_count; i ++) {
        switch (i) {
            case window_header:
            case window_footer:
            case window_dialog:
                wattron(windows[i], COLOR_PAIR(1));
                box(windows[i], 0, 0); // draw blue frame by default
                wattroff(windows[i], COLOR_PAIR(1));
            default: break;
        }
    }

    return 0;
} /* int create_windows */

/*
 * Setup menus function
 *
 * This function creates and/or configures ncurses menus for windows. This
 * function MUST be called after create_windows function (as it depends
 * on windows and sub-windows).
 * This function returns number of created and posted menus.
 */
int setup_menus()
{
    int i = 0;

    /* Create menus (0 - HEADER, 1 - DIALOG) if none exist */
    if (menus[menu_header] == NULL) {
        menus[menu_header] = new_menu(create_items(items_header));
    }
    if (menus[menu_dialog] == NULL) {
        menus[menu_dialog] = new_menu(create_items(items_dialog));
    }

    /* Set menu windows and sub-windows */
    for (i = 0; i < menu_count; i ++) {
        /* Extra checks if called before create_windows */
        if (windows[i] != NULL) {
            set_menu_win(menus[i], windows[i]);
        }
        if (menu_windows[i] != NULL) {
            set_menu_sub(menus[i], menu_windows[i]);
        }
        menu_opts_off(menus[i], O_SHOWDESC); // disable descriptions
        set_menu_mark(menus[i], " "); // set empty mark
    }

    /* Set menu formats (multi-columnar one-line menus) */
    for (i = 0; items_header[i] != NULL; i ++);
    set_menu_format(menus[menu_header], 1, i);
    for (i = 0; items_dialog[i] != NULL; i ++);
    set_menu_format(menus[menu_dialog], 1, i);

    /* TODO: restore selected item according to the menu_status */

    /* Post menus (with extra check for attached window) */
    for (i = 0; i < menu_count; i ++)
        if (windows[i] != NULL) {
            post_menu(menus[i]);
        }

    return 0;
} /* int setup_menus */

/*
 * Setup panels function
 *
 * This function configures panel metadata within tagMETA structures and binds
 * it to panels, sets up panel order. This function MUST be called after
 * create_windows function, setup_menus, and setup_forms functions.
 * This function returns on error number of panel that does not exist or zero
 * on success.
 */
int setup_panels()
{
    /* Set user pointer (to the next focus panel and current menu if any) */
    if (panels[window_header] == NULL) {
        return window_header;
    }
    panels_meta[window_header].menu = menus[menu_header];
    panels_meta[window_header].edit = NULL;
    panels_meta[window_header].next = panels[window_editor];
    set_panel_userptr(panels[window_header], &panels_meta[window_header]);

    if (panels[window_dialog] == NULL) {
        return window_dialog;
    }
    panels_meta[window_dialog].menu = menus[menu_dialog];
    panels_meta[window_dialog].edit = forms[form_dialog];
    panels_meta[window_dialog].next = NULL;
    set_panel_userptr(panels[window_dialog], &panels_meta[window_dialog]);

    if (panels[window_editor] == NULL) {
        return window_editor;
    }
    panels_meta[window_editor].menu = NULL;
    panels_meta[window_editor].edit = forms[form_editor];
    panels_meta[window_editor].next = panels[window_header];
    set_panel_userptr(panels[window_editor], &panels_meta[window_editor]);

    /* Update focus */
    if (menu_state.dialog_visible) {
        show_panel(panels[window_dialog]);
        focus_panel = panels[window_dialog];
    } else {
        focus_panel = panels[window_editor];
        hide_panel(panels[window_dialog]);
    }
    top_panel(focus_panel);

    return 0;
} /* int setup_panels */

/*
 * Setup forms function
 *
 * This function creates forms in existing windows and associates them with
 * subwindows (if any). This function calls create_fields auxiliary function.
 * This function MUST be called after create_windows function.
 * This function returns number of created and posted forms.
 */
int setup_forms()
{
    int i = 0, max_x = 0, max_y = 0;

    /* Create forms (0 > MEMO > EDITOR, 1 > PATH > DIALOG) if none exist */
    if (forms[form_editor] == NULL) {
        getmaxyx(form_windows[form_editor], max_y, max_x);
        field_options[field_input].height = max_y;
        field_options[field_input].width = max_x;
        forms[form_editor] = new_form(create_fields(fields_editor));
        /* Restore editor buffer (if it was previously saved) */
        if (menu_state.edit_buffer != NULL) {
            set_field_buffer((form_fields(forms[form_editor]))[field_input],
                             0, menu_state.edit_buffer);
        }
    }
    if (forms[form_dialog] == NULL) {
        getmaxyx(form_windows[form_dialog], max_y, max_x);
        field_options[field_input].width = max_x - 2;
        field_options[field_title].width = max_x - 2;
        forms[form_dialog] = new_form(create_fields(fields_dialog));
        /* Restore dialog state if it is visible */
        if (menu_state.dialog_visible) {
        /* Dialog title */
            if (menu_state.dialog_do_open) {
                set_field_buffer((form_fields(forms[form_dialog]))[field_title],
                                 0, menu_state.title_open);
            } else {
                set_field_buffer((form_fields(forms[form_dialog]))[field_title],
                                 0, menu_state.title_save);
            }
        /* Dialog buffer if it does exist */
            if (menu_state.path_to_file != NULL) {
                set_field_buffer((form_fields(forms[form_dialog]))[field_input],
                                 0, menu_state.path_to_file);
            }
        }
    }

    /* Set form windows and sub-windows */
    if (windows[window_editor] != NULL) {
        set_form_win(forms[form_editor], windows[window_editor]);
        /* Set form_windows[0] for forms[0] (if not NULL) */
        if (form_windows[form_editor] != NULL) {
            set_form_sub(forms[form_editor], form_windows[form_editor]);
        }
        /* Post editor form */
        post_form(forms[form_editor]);
        i ++; // successfully posted form counter
    }

    if (windows[window_dialog] != NULL) {
        set_form_win(forms[form_dialog], windows[window_dialog]);
        /* Set form_windows[1] for forms[1] (if not NULL) */
        if (form_windows[form_dialog] != NULL) {
            set_form_sub(forms[form_dialog], form_windows[form_dialog]);
        }
        /* Post dialog form */
        post_form(forms[form_dialog]);
        i ++; // successfully posted form counter
    }

    return i; // number of successfully posted forms
} /* int setup_forms */

/*
 * Free forms function
 *
 * This function sequentially releases resources of the forms, but saves
 * buffers into menu_state variable. Buffers MUST be released later manually.
 * This function MUST be called after create_windows function and before
 * free_windows function.
 * This function returns number of relesed forms.
 */
int free_forms()
{
    int i = 0;
    FIELD *editor = (form_fields(forms[form_editor]))[field_input];
    FIELD *dialog = (form_fields(forms[form_dialog]))[field_input];

    /* Save editor buffer prior destruction */
    form_driver(forms[form_editor], REQ_VALIDATION);
    free(menu_state.edit_buffer);
    menu_state.edit_buffer = field_buffer(editor, 0);
    set_field_buffer(editor, 0, NULL);

    /* Save dialog buffer prior destruction */
    form_driver(forms[form_dialog], REQ_VALIDATION);
    menu_state.path_to_file = field_buffer(dialog, 0);
    set_field_buffer(dialog, 0, NULL);

    /* Unpost, detach and free menus and items */
    for (i = 0; i < form_count; i ++) {
        unpost_form(forms[i]);
        free_fields(form_fields(forms[i]));
        free_form(forms[i]);
        forms[i] = NULL; // clear invalid pointer
    }

    return i;
} /* int free_forms */

/*
 * Free menus function
 *
 * This function releases resources of ncurses menus (previously created by
 * setup_menus function). This function MUST be called after setup_menus
 * function and SHOULD be called before free_windows function on exit.
 * This function returns number of released menus.
 */
int free_menus()
{
    int i = 0;

    /* Unpost, detach and free menus and items */
    for (i = 0; i < menu_count; i ++) {
        unpost_menu(menus[i]);
        free_items(menu_items(menus[i]));
        free_menu(menus[i]);
    }

    return i;
} /* int free_menus */

/*
 * Free windows function
 *
 * This function releases resources of ncurses windows (previously created by
 * create_windows function). This function MUST be called after create_windows
 * function and MAY be called without calling free_menus function.
 * This function returns number of released windows.
 */
int free_windows()
{
    int i = 0;

    /* Detach menus (if any) before desroying windows */
    for (i = 0; i < menu_count; i ++)
        if (menus[i] != NULL) {
            unpost_menu(menus[i]);
        }
    /* Destroy windows */
    for (i = 0; i < window_count; i ++) {
        /* Erase borders */
        wborder(windows[i], ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        /* Destroy panel, then window */
        del_panel(panels[i]);
        delwin(windows[i]);
    }

    return i;
} /* int free_windows */

/*
 * Create items function (auxiliary)
 *
 * This function sequentially creates menu items.
 *
 * from_names - a null-terminated array of pointers to strings (null-
 *              terminated arrays of char).
 *
 * return - a null-terminated array of pointers to ITEM * to pass to
 *          <new_menu> function, or just NULL if error occurs, or item_list
 *          has no elements.
 *
 * NOTE: if error occurs on the first ITEM creation (<new_item>), then this
 *       function will return a pointer to the array with the only one NULL
 *       element.
 */
ITEM **create_items(const char *from_names[])
{
    ITEM **item_list;
    int num_items = 0;

    /* Scan array of strings and stop if NULL-element is found */
    for (; from_names[num_items] != NULL; num_items ++);

    /* Allocate memory for new items (if any) */
    if (num_items) {
        item_list = (ITEM **) calloc(num_items + 1, sizeof (ITEM *));
        if (item_list == NULL) {
            /* Error allocating memory */
            return NULL;
        }
    } else {
        /* No items */
        return NULL;
    }

    /* Create items or return if it's not possible to */
    for (int i = 0; i < num_items; i ++) {
        item_list[i] = new_item(from_names[i], NULL);
        if (item_list[i] == NULL) {
            /* Error creating element (return that were created) */
            item_list = realloc(item_list, sizeof (ITEM *) * (i + 1));
            return item_list;
        }
    }

    item_list[num_items] = NULL;
    return item_list;
} /* ITEM **create_items */

/*
 * Free items function (auxiliary)
 *
 * This function sequentially releases resources hold by menu items.
 *
 * from_items - must be a pointer to a null-terminated array of pointers.
 *
 * return - number of released elements.
 *
 * WARNING: passing a variable other than null-terminated list of pointers,
 *          from <create_item> function will cause severe errors.
 */
int free_items(ITEM **from_items)
{
    int num_free = 0;

    if (from_items != NULL) {
        while (from_items[num_free] != NULL) {
            /* Free the item (item name points to global array variable
             * description is already NULL, item_userptr is unset) */
            free_item(from_items[num_free]);
            num_free ++;
        }
    }

    /* Release the item list (may be NULL) */
    free(from_items);

    return num_free;
} /* int free_items */

/*
 * Create fields function (auxiliary)
 *
 * This function creates a list of fields.
 *
 * from_options - an array of pointers to FOJA that hold options for the
 *                relevant fields.
 *
 * return - a null-terminated array of pointers to FIELD * to pass to
 *          <new_form> function.
 *
 * NOTE: if error occurs on the first ITEM creation (<new_field>), then this
 *       function will return a pointer to the array with the only one NULL
 *       element.
 */
FIELD **create_fields(FOJA *from_options[])
{
    FIELD **field_list;
    FOJA *foja = NULL;
    int num_fields = 0;

    /* Scan array of strings and stop if NULL-element is found */
    for (; from_options[num_fields] != NULL; num_fields ++);

    /* Allocate memory for new items (if any) */
    if (num_fields) {
        field_list = (FIELD **) calloc(num_fields + 1, sizeof (FIELD *));
        if (field_list == NULL) {
            /* Error allocating memory */
            return NULL;
        }
    } else {
        /* No fields */
        return NULL;
    }

    /* Create items or return if it's not possible to */
    for (int i = 0; i < num_fields; i ++) {
        foja = from_options[i];
        field_list[i] = new_field(foja->height, foja->width,
                foja->top, foja->left,
                foja->offscreen, foja->buffers);
        if (field_list[i] == NULL) {
            /* Error creating element (break) */
            field_list = realloc(field_list, sizeof (FIELD *) * (i + 1));
            return field_list;
        } else {
            /* Configure created field */
            if (foja->options) {
                /* Set positive, unset negative options */
                if (foja->options > 0) {
                    field_opts_on(field_list[i], foja->options);
                } else {
                    field_opts_off(field_list[i], foja->options);
                }
            }
            /* Set justification */
            if (foja->justification) {
                set_field_just(field_list[i], foja->justification);
            }
            /* Set attributes */
            if (foja->attrib_fore) {
                set_field_fore(field_list[i], foja->attrib_fore);
            }
            if (foja->attrib_back) {
                set_field_back(field_list[i], foja->attrib_back);
            }
            if (foja->attrib_pad) {
                set_field_fore(field_list[i], foja->attrib_pad);
            }
        }
    }

    field_list[num_fields] = NULL;
    return field_list;
} /* FIELD **create_fields */

/*
 * Free fields function (auxiliary)
 *
 * This function releases resources hold by form fields.
 *
 * from_fields - must be a pointer to a null-terminated array of pointers.
 *
 * return - number of released elements.
 *
 * WARNING: passing a variable other than null-terminated list of pointers,
 *          from <create_fields> function will cause severe errors.
 */
int free_fields(FIELD **from_fields)
{
    int num_free = 0;

    if (from_fields != NULL) {
        while (from_fields[num_free] != NULL) {
            /* Free the field */
            free_field(from_fields[num_free]);
            num_free ++;
        }
    }

    /* Release the field list (may be NULL) */
    free(from_fields);

    return num_free;
} /* int free_fields */

/* vim: set et sw=4: */
