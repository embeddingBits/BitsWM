#include "wm.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>

Display *display;
Window root, status_bar;
Client *clients[MAX_WINDOWS];
int num_clients = 0;
int screen_width, screen_height;
int current_workspace = 0;
XftFont *status_font;
XftDraw *xft_draw;
XftColor xft_color, xft_active_color;

int main() {
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    root = DefaultRootWindow(display);
    Screen *screen = DefaultScreenOfDisplay(display);
    screen_width = WidthOfScreen(screen);
    screen_height = HeightOfScreen(screen);

    XSelectInput(display, root, SubstructureRedirectMask | 
                SubstructureNotifyMask | KeyPressMask | ButtonPressMask);

    /* Register keybinds */
    for (int i = 0; i < sizeof(keybinds) / sizeof(keybinds[0]); i++) {
        XGrabKey(display, XKeysymToKeycode(display, keybinds[i].keysym), 
                 keybinds[i].mod, root, True, GrabModeAsync, GrabModeAsync);
    }
    for (int i = 0; i < NUM_WORKSPACES; i++) {
        XGrabKey(display, XKeysymToKeycode(display, XK_1 + i), Mod4Mask,
                 root, True, GrabModeAsync, GrabModeAsync);
    }

    /* Register mouse bind */
    XGrabButton(display, MOUSE_BUTTON, MOUSE_MOD, root, True, 
                ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);

    create_status_bar();

    XEvent ev;
    while (1) {
        XNextEvent(display, &ev);
        
        switch (ev.type) {
            case MapRequest:
                add_window(ev.xmaprequest.window);
                break;
            case DestroyNotify:
                remove_window(ev.xdestroywindow.window);
                break;
            case KeyPress:
                for (int i = 0; i < sizeof(keybinds) / sizeof(keybinds[0]); i++) {
                    if (ev.xkey.keycode == XKeysymToKeycode(display, keybinds[i].keysym) &&
                        ev.xkey.state & keybinds[i].mod && keybinds[i].func) {
                        keybinds[i].func();
                    }
                }
                if (ev.xkey.state & Mod1Mask) {
                    Window focused;
                    int revert;
                    XGetInputFocus(display, &focused, &revert);
                    for (int i = 0; i < num_clients; i++) {
                        if (clients[i]->window == focused) {
                            if (ev.xkey.keycode == XKeysymToKeycode(display, XK_Up)) {
                                resize_window(focused, clients[i]->x, clients[i]->y,
                                             clients[i]->width, clients[i]->height + 10);
                            }
                            else if (ev.xkey.keycode == XKeysymToKeycode(display, XK_Down)) {
                                resize_window(focused, clients[i]->x, clients[i]->y,
                                             clients[i]->width, clients[i]->height - 10);
                            }
                            break;
                        }
                    }
                }
                else if (ev.xkey.state & Mod4Mask) {
                    for (int i = 0; i < NUM_WORKSPACES; i++) {
                        if (ev.xkey.keycode == XKeysymToKeycode(display, XK_1 + i)) {
                            switch_workspace(i);
                            update_status_bar();
                            break;
                        }
                    }
                }
                break;
            case ButtonPress:
                if (ev.xbutton.state & MOUSE_MOD) {
                    handle_mouse_resize(&ev.xbutton);
                }
                break;
            case ConfigureRequest:
                {
                    XWindowChanges wc = {
                        .x = ev.xconfigurerequest.x,
                        .y = ev.xconfigurerequest.y,
                        .width = ev.xconfigurerequest.width,
                        .height = ev.xconfigurerequest.height,
                        .border_width = ev.xconfigurerequest.border_width,
                        .sibling = ev.xconfigurerequest.above,
                        .stack_mode = ev.xconfigurerequest.detail
                    };
                    XConfigureWindow(display, ev.xconfigurerequest.window,
                                    ev.xconfigurerequest.value_mask, &wc);
                    tile_windows();
                }
                break;
            case Expose:
                if (ev.xexpose.window == status_bar) {
                    update_status_bar();
                }
                break;
        }
    }
    
    cleanup();
    return 0;
}
