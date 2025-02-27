#ifndef WM_H
#define WM_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>

/* Structs */
typedef struct {
    Window window;
    int x, y, width, height;
    int workspace;
} Client;

/* Globals */
extern Display *display;
extern Window root, status_bar;
extern Client *clients[];
extern int num_clients;
extern int screen_width, screen_height;
extern int current_workspace;
extern XftFont *status_font;
extern XftDraw *xft_draw;
extern XftColor xft_color, xft_active_color;

/* Client Functions (client.c) */
void tile_windows(void);
void add_window(Window w);
void remove_window(Window w);
void switch_window(void);
void resize_window(Window w, int x, int y, int width, int height);
void handle_mouse_resize(XButtonEvent *ev);

/* Utility Functions (utils.c) */
void create_status_bar(void);
void update_status_bar(void);
void launch_kitty(void);
void launch_xterm(void);
void launch_firefox(void);
void launch_rofi(void);
void kill_active_window(void);
void switch_workspace(int new_workspace);
void cleanup(void);

#endif
