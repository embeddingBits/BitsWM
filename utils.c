#include "bitswm.h"
#include "config.h"
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

void create_status_bar() {
    status_bar = XCreateSimpleWindow(display, root, GAP, GAP, screen_width - 2 * GAP, BAR_HEIGHT,
                                    0, NORD0, NORD0);
    XSetWindowBackground(display, status_bar, NORD0);
    XSelectInput(display, status_bar, ExposureMask);
    XMapWindow(display, status_bar);

    status_font = XftFontOpenName(display, DefaultScreen(display), FONT_NAME);
    if (!status_font) {
        fprintf(stderr, "Failed to load font '%s'\n", FONT_NAME);
        status_font = XftFontOpenName(display, DefaultScreen(display), FALLBACK_FONT);
    }
    xft_draw = XftDrawCreate(display, status_bar, 
                            DefaultVisual(display, DefaultScreen(display)),
                            DefaultColormap(display, DefaultScreen(display)));
    XftColorAllocName(display, DefaultVisual(display, DefaultScreen(display)),
                     DefaultColormap(display, DefaultScreen(display)),
                     "#D8DEE9", &xft_color);
    XftColorAllocName(display, DefaultVisual(display, DefaultScreen(display)),
                     DefaultColormap(display, DefaultScreen(display)),
                     "#88C0D0", &xft_active_color);
}

void update_status_bar() {
    char status[256];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    int battery = 75;
    int volume = 50;
    
    snprintf(status, sizeof(status), "%d%% | Vol: %d%% | %02d:%02d",
             battery, volume, t->tm_hour, t->tm_min);
    
    XClearWindow(display, status_bar);
    
    int x_offset = 10;
    for (int i = 0; i < NUM_WORKSPACES; i++) {
        const char *logo = WORKSPACE_LOGOS[i];
        XftColor *color = (i == current_workspace) ? &xft_active_color : &xft_color;
        XftDrawStringUtf8(xft_draw, color, status_font, x_offset, BAR_HEIGHT - 6,
                         (FcChar8 *)logo, strlen(logo));
        XGlyphInfo extents;
        XftTextExtentsUtf8(display, status_font, (FcChar8 *)logo, strlen(logo), &extents);
        x_offset += extents.width;
    }
    
    XGlyphInfo extents;
    XftTextExtentsUtf8(display, status_font, (FcChar8 *)status, strlen(status), &extents);
    int status_width = extents.width;
    XftDrawStringUtf8(xft_draw, &xft_color, status_font, 
                     screen_width - 2 * GAP - status_width - 10, BAR_HEIGHT - 6,
                     (FcChar8 *)status, strlen(status));
}

void launch_kitty() {
    if (fork() == 0) {
        execlp("kitty", "kitty", NULL);
        perror("Failed to launch kitty");
        exit(1);
    }
}

void launch_xterm() {
    if (fork() == 0) {
        execlp("xterm", "xterm", NULL);
        perror("Failed to launch xterm");
        exit(1);
    }
}

void launch_firefox() {
    if (fork() == 0) {
        execlp("firefox", "firefox", NULL);
        perror("Failed to launch firefox");
        exit(1);
    }
}

void launch_rofi() {
    if (fork() == 0) {
        execlp("rofi", "rofi", "-show", "drun", NULL);
        perror("Failed to launch rofi");
        exit(1);
    }
}

void kill_active_window() {
    Window focused;
    int revert;
    XGetInputFocus(display, &focused, &revert);
    if (focused != None && focused != root) {
        XKillClient(display, focused);
    }
}

void switch_workspace(int new_workspace) {
    if (new_workspace >= 0 && new_workspace < NUM_WORKSPACES) {
        current_workspace = new_workspace;
        tile_windows();
    }
}

void cleanup() {
    if (status_font) XftFontClose(display, status_font);
    if (xft_draw) XftDrawDestroy(xft_draw);
    if (display) {
        XftColorFree(display, DefaultVisual(display, DefaultScreen(display)),
                    DefaultColormap(display, DefaultScreen(display)), &xft_color);
        XftColorFree(display, DefaultVisual(display, DefaultScreen(display)),
                    DefaultColormap(display, DefaultScreen(display)), &xft_active_color);
        XCloseDisplay(display);
    }
}
