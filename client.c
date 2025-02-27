#include "bitswm.h"
#include "config.h"
#include <stdlib.h>

void tile_windows() {
    if (num_clients == 0) return;

    int usable_width = screen_width - 2 * GAP;
    int usable_height = screen_height - 3 * GAP - BAR_HEIGHT;

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
            clients[i]->y = GAP * 2 + BAR_HEIGHT;
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
                clients[i]->y = GAP * 2 + BAR_HEIGHT;
                clients[i]->width = master_width;
                clients[i]->height = usable_height;
            } else {
                clients[i]->x = GAP + master_width + GAP;
                clients[i]->y = GAP * 2 + BAR_HEIGHT;
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
                clients[i]->y = GAP * 2 + BAR_HEIGHT;
                clients[i]->width = master_width;
                clients[i]->height = usable_height;
            } else {
                clients[i]->x = GAP + master_width + GAP;
                clients[i]->y = GAP * 2 + BAR_HEIGHT + (mapped - 1) * (stack_height + GAP);
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
    clients[num_clients]->workspace = current_workspace;
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
