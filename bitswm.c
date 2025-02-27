#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_WINDOWS 100

typedef struct {
    Window window;
    int x, y, width, height;
} Client;

Display *display;
Window root;
Client *clients[MAX_WINDOWS];
int num_clients = 0;

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
    }
}

void remove_window(Window w) {
    for (int i = 0; i < num_clients; i++) {
        if (clients[i]->window == w) {
            free(clients[i]);
            clients[i] = clients[num_clients - 1];
            num_clients--;
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
            if (new_width > 50 && new_height > 50) {  // Minimum size
                resize_window(w, attr.x, attr.y, new_width, new_height);
            }
        }
    } while (event.type != ButtonRelease);
    
    XUngrabPointer(display, CurrentTime);
}

int main() {
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    root = DefaultRootWindow(display);
    XSelectInput(display, root, SubstructureRedirectMask | 
                SubstructureNotifyMask | KeyPressMask | ButtonPressMask);
    
    // Key bindings
    XGrabKey(display, XKeysymToKeycode(display, XK_Tab), Mod1Mask, 
            root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XK_Up), Mod1Mask, 
            root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, XKeysymToKeycode(display, XK_Down), Mod1Mask, 
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
                }
                break;
        }
    }
    
    XCloseDisplay(display);
    return 0;
}
