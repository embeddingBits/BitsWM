#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define MAX_WINDOWS 100
#define GAP 15
#define BAR_HEIGHT 20
#define NUM_WORKSPACES 4

typedef struct {
    Window window;
    int x, y, width, height;
    int workspace;  // Which workspace this window belongs to
} Client;

Display *display;
Window root, status_bar;
Client *clients[MAX_WINDOWS];
int num_clients = 0;
int screen_width, screen_height;
int current_workspace = 0;  // Active workspace (0-3)

// Function prototypes
void update_status_bar();

void create_status_bar() {
    status_bar = XCreateSimpleWindow(display, root, 0, 0, screen_width, BAR_HEIGHT,
                                    0, BlackPixel(display, DefaultScreen(display)),
                                    WhitePixel(display, DefaultScreen(display)));
    XSelectInput(display, status_bar, ExposureMask);
    XMapWindow(display, status_bar);
}

void tile_windows() {
    if (num_clients == 0) return;

    int usable_width = screen_width - 2 * GAP;
    int usable_height = screen_height - 2 * GAP - BAR_HEIGHT;  // Account for status bar

    int visible_clients = 0;
    for (int i = 0; i < num_clients; i++) {
        if (clients[i]->workspace == current_workspace) visible_clients++;
    }

    if (visible_clients == 0) return;

    if (visible_clients == 1) {
        for (int i = 0; i < num_clients; i++) {
            if (clients[i]->workspace != current_workspace) {
                XUnmapWindow(display, clients[i]->window);
                continue;
            }
            clients[i]->x = GAP;
            clients[i]->y = GAP + BAR_HEIGHT;
            clients[i]->width = usable_width;
            clients[i]->height = usable_height;
            XConfigureWindow(display, clients[i]->window,
                            CWX | CWY | CWWidth | CWHeight,
                            &(XWindowChanges){
                                .x = clients[i]->x,
                                .y = clients[i]->y,
                                .width = clients[i]->width,
                                .height = clients[i]->height
                            });
            XMapWindow(display, clients[i]->window);
        }
    } else if (visible_clients == 2) {
        int master_width = (usable_width - GAP) / 2;
        int mapped = 0;
        for (int i = 0; i < num_clients; i++) {
            if (clients[i]->workspace != current_workspace) {
                XUnmapWindow(display, clients[i]->window);
                continue;
            }
            if (mapped == 0) {
                clients[i]->x = GAP;
                clients[i]->y = GAP + BAR_HEIGHT;
                clients[i]->width = master_width;
                clients[i]->height = usable_height;
            } else {
                clients[i]->x = GAP + master_width + GAP;
                clients[i]->y = GAP + BAR_HEIGHT;
                clients[i]->width = master_width;
                clients[i]->height = usable_height;
            }
            XConfigureWindow(display, clients[i]->window,
                            CWX | CWY | CWWidth | CWHeight,
                            &(XWindowChanges){
                                .x = clients[i]->x,
                                .y = clients[i]->y,
                                .width = clients[i]->width,
                                .height = clients[i]->height
                            });
            XMapWindow(display, clients[i]->window);
            mapped++;
        }
    } else {
        int master_width = (usable_width - GAP) / 2;
        int stack_width = master_width;
        int stack_height = (usable_height - (visible_clients - 2) * GAP) / (visible_clients - 1);
        int mapped = 0;
        for (int i = 0; i < num_clients; i++) {
            if (clients[i]->workspace != current_workspace) {
                XUnmapWindow(display, clients[i]->window);
                continue;
            }
            if (mapped == 0) {
                clients[i]->x = GAP;
                clients[i]->y = GAP + BAR_HEIGHT;
                clients[i]->width = master_width;
                clients[i]->height = usable_height;
            } else {
                clients[i]->x = GAP + master_width + GAP;
                clients[i]->y = GAP + BAR_HEIGHT + (mapped - 1) * (stack_height + GAP);
                clients[i]->width = stack_width;
                clients[i]->height = stack_height;
            }
            XConfigureWindow(display, clients[i]->window,
                            CWX | CWY | CWWidth | CWHeight,
                            &(XWindowChanges){
                                .x = clients[i]->x,
                                .y = clients[i]->y,
                                .width = clients[i]->width,
                                .height = clients[i]->height
                            });
            XMapWindow(display, clients[i]->window);
            mapped++;
        }
    }
    update_status_bar();
}

void add_window(Window w) {
    if (num_clients >= MAX_WINDOWS) return;
    
    XWindowAttributes attr;
    XGetWindowAttributes(display, w, &attr);
    
    clients[num_clients] = malloc(sizeof(Client));
    if (!clients[num_clients]) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    
    clients[num_clients]->window = w;
    clients[num_clients]->x = attr.x;
    clients[num_clients]->y = attr.y;
    clients[num_clients]->width = attr.width;
    clients[num_clients]->height = attr.height;
    clients[num_clients]->workspace = current_workspace;  // New windows go to current workspace
    num_clients++;
    tile_windows();
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

void switch_workspace(int new_workspace) {
    if (new_workspace >= 0 && new_workspace < NUM_WORKSPACES) {
        current_workspace = new_workspace;
        tile_windows();
    }
}

void update_status_bar() {
    char status[256];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    // Simulate battery and volume (replace with actual system calls if desired)
    int battery = 75;  // Placeholder
    int volume = 50;   // Placeholder
    
    snprintf(status, sizeof(status), "[%d] %d%% | Vol: %d%% | %02d:%02d | WS: %d/%d",
             current_workspace + 1, battery, volume, t->tm_hour, t->tm_min,
             current_workspace + 1, NUM_WORKSPACES);
    
    XClearWindow(display, status_bar);
    XDrawString(display, status_bar, DefaultGC(display, DefaultScreen(display)),
                10, BAR_HEIGHT - 5, status, strlen(status));
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
    // Workspace switching (Win + 1, 2, 3, 4)
    for (int i = 0; i < NUM_WORKSPACES; i++) {
        XGrabKey(display, XKeysymToKeycode(display, XK_1 + i), Mod4Mask,
                root, True, GrabModeAsync, GrabModeAsync);
    }
    
    // Mouse binding
    XGrabButton(display, 1, Mod1Mask, root, True, 
               ButtonPressMask, GrabModeAsync, GrabModeAsync, 
               None, None);

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
                else if (ev.xkey.state & Mod4Mask) {
                    for (int i = 0; i < NUM_WORKSPACES; i++) {
                        if (ev.xkey.keycode == XKeysymToKeycode(display, XK_1 + i)) {
                            switch_workspace(i);
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
            case Expose:
                if (ev.xexpose.window == status_bar) {
                    update_status_bar();
                }
                break;
        }
    }
    
    XCloseDisplay(display);
    return 0;
}
