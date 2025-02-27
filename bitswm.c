#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_WINDOWS 100
#define MASTER_WIDTH_FACTOR 0.55  // Master window takes 55% of screen width

typedef struct {
    Window window;
    int x, y, width, height;
} Client;

Display *display;
Window root;
Client *clients[MAX_WINDOWS];
int num_clients = 0;
int screen_width, screen_height;

void tile_windows() {
    if (num_clients == 0) return;

    int master_width = screen_width * MASTER_WIDTH_FACTOR;
    int stack_width = screen_width - master_width;
    int stack_height = (num_clients > 1) ? screen_height / (num_clients - 1) : 0;

    // Master window (first window)
    if (num_clients >= 1) {
        clients[0]->x = 0;
        clients[0]->y = 0;
        clients[0]->width = master_width;
        clients[0]->height = screen_height;
        XConfigureWindow(display, clients[0]->window,
                        CWX | CWY | CWWidth | CWHeight,
                        &(XWindowChanges){
                            .x = clients[0]->x,
                            .y = clients[0]->y,
                            .width = clients[0]->width,
                            .height = clients[0]->height
                        });
    }

    // Stack windows (remaining windows)
    for (int i = 1; i < num_clients; i++) {
        clients[i]->x = master_width;
        clients[i]->y = (i - 1) * stack_height;
        clients[i]->width = stack_width;
        clients[i]->height = stack_height;
        XConfigureWindow(display, clients[i]->window,
                        CWX | CWY | CWWidth | CWHeight,
                        &(XWindowChanges){
                            .x = clients[i]->x,
                            .y = clients[i]->y,
                            .width = clients[i]->width,
                            .height = clients[i]->height
                        });
    }
}

void add_window(Window w) {
    if (num_clients < MAX_WINDOWS) {
        XWindowAttributes attr;
        XGetWindowAttributes(display, w, &attr);
        
        clients[num_clients] = malloc(sizeof(Client));
        clients[num_clients]->window = w;
        clients[num_clients]->x = attr.x;
        clients[num_clients]->y = attr.y;
        clients[num_clients]->width = attr.width;
        clients[num_clients]->height = attr.height;
        
        XSelectInput(display, w, EnterWindowMask | FocusChangeMask | 
                    PropertyChangeMask | StructureNotifyMask);
        XMapWindow(display, w);
        num_clients++;
        tile_windows();
    }
}

void remove_window(Window w) {
    for (int i = 0; i < num_clients; i++) {
        if (clients[i]->window == w) {
            free(clients[i]);
            clients[i] = clients[num_clients - 1];
            num_clients--;
            tile_windows();
            break;
        }
    }
}

void switch_window() {
    if (num_clients > 0) {
        static int current = 0;
        current = (current + 1) % num_clients;
        XRaiseWindow(display, clients[current]->window);
        XSetInputFocus(display, clients[current]->window, 
                      RevertToParent, CurrentTime);
    }
}

void resize_window(Window w, int x, int y, int width, int height) {
    XConfigureWindow(display, w, 
                    CWX | CWY | CWWidth | CWHeight, 
                    &(XWindowChanges){
                        .x = x,
                        .y = y,
                        .width = width,
                        .height = height
                    });
}

void handle_mouse_resize(XButtonEvent *ev) {
    Window w = ev->window;
    XWindowAttributes attr;
    XGetWindowAttributes(display, w, &attr);
    
    int x_start = ev->x_root;
    int y_start = ev->y_root;
    
    XEvent event;
    XGrabPointer(display, root, False, 
                PointerMotionMask | ButtonReleaseMask,
                GrabModeAsync, GrabModeAsync, 
                None, None, CurrentTime);
    
    do {
        XMaskEvent(display, ButtonReleaseMask | PointerMotionMask, &event);
        if (event.type == MotionNotify) {
            int new_width = attr.width + (event.xmotion.x_root - x_start);
            int new_height = attr.height + (event.xmotion.y_root - y_start);
            if (new_width > 50 && new_height > 50) {
                resize_window(w, attr.x, attr.y, new_width, new_height);
            }
        }
    } while (event.type != ButtonRelease);
    
    XUngrabPointer(display, CurrentTime);
}

void launch_kitty() {
    if (fork() == 0) {
        execlp("kitty", "kitty", NULL);
        perror("Failed to launch kitty");
        exit(1);
    }
}

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
    
    // Key bindings
    XGrabKey(display, XKeysymToKeycode(display, XK_Tab), Mod1Mask, 
            root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XK_Up), Mod1Mask, 
            root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XK_Down), Mod1Mask, 
            root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XK_q), Mod4Mask, 
            root, True, GrabModeAsync, GrabModeAsync);
    
    // Mouse binding
    XGrabButton(display, 1, Mod1Mask, root, True, 
               ButtonPressMask, GrabModeAsync, GrabModeAsync, 
               None, None);

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
                if (ev.xkey.keycode == XKeysymToKeycode(display, XK_Tab) && 
                    ev.xkey.state & Mod1Mask) {
                    switch_window();
                }
                else if (ev.xkey.keycode == XKeysymToKeycode(display, XK_q) && 
                         ev.xkey.state & Mod4Mask) {
                    launch_kitty();
                }
                else if (ev.xkey.state & Mod1Mask) {
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
                break;
            case ButtonPress:
                if (ev.xbutton.state & Mod1Mask) {
                    handle_mouse_resize(&ev.xbutton);
                }
                break;
            case ConfigureRequest:
                {
                    XWindowChanges wc;
                    wc.x = ev.xconfigurerequest.x;
                    wc.y = ev.xconfigurerequest.y;
                    wc.width = ev.xconfigurerequest.width;
                    wc.height = ev.xconfigurerequest.height;
                    wc.border_width = ev.xconfigurerequest.border_width;
                    wc.sibling = ev.xconfigurerequest.above;
                    wc.stack_mode = ev.xconfigurerequest.detail;
                    XConfigureWindow(display, ev.xconfigurerequest.window,
                                   ev.xconfigurerequest.value_mask, &wc);
                    tile_windows();
                }
                break;
        }
    }
    
    XCloseDisplay(display);
    return 0;
}
