#ifndef CONFIG_H
#define CONFIG_H

/* Appearance */
#define GAP 15
#define BAR_HEIGHT 35
#define MAX_WINDOWS 100
#define NUM_WORKSPACES 4
#define NORD0 0x2E3440  // Background color
#define NORD4 0xD8DEE9  // Text color
#define NORD8 0x88C0D0  // Active workspace color (cyan)
#define FONT_NAME "CaskaydiaMono Nerd Font:size=22"
#define FALLBACK_FONT "fixed"

/* Workspace Logos */
static const char *WORKSPACE_LOGOS[] = {
    "   ",  // Workspace 1: Gear
    "󰈹   ",  // Workspace 2: Memo
    "   ",  // Workspace 3: Globe
    "   "   // Workspace 4: Gamepad
};

/* Keybindings */
typedef struct {
    KeySym keysym;
    unsigned int mod;
    void (*func)(void);
} Keybind;

static Keybind keybinds[] = {
    { XK_Tab,  Mod1Mask, switch_window },  // Alt+Tab: Switch window
    { XK_Up,   Mod1Mask, NULL },          // Alt+Up: Resize (handled in loop)
    { XK_Down, Mod1Mask, NULL },          // Alt+Down: Resize (handled in loop)
    { XK_q,    Mod4Mask, launch_kitty },  // Win+q: Kitty
    { XK_t,    Mod4Mask, launch_xterm },  // Win+t: Xterm
    { XK_f,    Mod4Mask, launch_firefox },// Win+f: Firefox
    { XK_r,    Mod4Mask, launch_rofi },   // Win+r: Rofi
    { XK_k,    Mod4Mask, kill_active_window }, // Win+k: Kill window
    // Workspace switching (Win+1 to Win+4) added dynamically in main
};

/* Mouse Bindings */
#define MOUSE_MOD Mod1Mask  // Alt for resizing
#define MOUSE_BUTTON 1      // Left click

#endif
