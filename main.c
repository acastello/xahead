/** \file */

#define _GNU_SOURCE
#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <X11/Xlib.h>

#include "xahead.h"

#ifndef MAX_TOP_WINDOWS
/**
 * Probably won't be surpassed before a regular, viewable window is mapped
 */
#define MAX_TOP_WINDOWS 32
#endif
#define WINE_SYSTEM_PREFIX "C:\\windows\\system32\\"

/**
 * To be dlsym-initialized with the actual `wine_init` if necessary
 */
static void (*real_wine_init)(int argc, char *argv[], char *error, int error_size);

/**
 * To be dlsym-initialized with the actual XCreateWindow
 */
static Window (*real_XCreateWindow)(Display *display, Window parent, int x, int y,
        unsigned int width, unsigned int height, unsigned  int  border_width,
        int depth, unsigned int class,  Visual *visual, unsigned long valuemask,
        XSetWindowAttributes *attributes);

/**
 * To be dlsym-initialized with the actual XMapWindow
 */
static int (*real_XMapWindow)(Display *display, Window w);

/**
 * Created and mapped as soon as possible in index-configured programs. To be
 * returned on the creation of configured, first visible regular window
 * \note _visible_ as in successfully and deliberately mapped: is passed to a
 * successful XMapWindow call
 * \note _regular_ as in managed (no `override_redirect`) and mapped to the root
 * window
 */
static Window placeholder;

/**
 * Create and map a single global placeholder window, used only in execution
 * mode (index is configured for progname)
 */
static void create_placeholder(void)
{
    if (placeholder) {
        fputs("!! Trying to recreate placeholder window", stderr);
        exit(1);
    }
    XInitThreads();
    Display *dpy = XOpenDisplay(NULL);
    placeholder = real_XCreateWindow(dpy, DefaultRootWindow(dpy), 10, 10,
            100, 100, 0, CopyFromParent, CopyFromParent, CopyFromParent, 0,NULL);
    real_XMapWindow(dpy, placeholder);
    XFlush(dpy);
}

/**
 * Set by *_init, specifies index of window to be spawned, or 0 if none is
 * configured
 */
static int configured_index = -1;
/**
 * Set by *_init, specifies program name for saving the new index
 */
static char *progname;

/**
 * Overload wine_init to provide additional initialization and loading
 * configuration
 */
void wine_init(int argc, char *argv[], char *error, int error_size)
{
    if (
            argc > 0 &&
            strncmp(
                argv[1],
                WINE_SYSTEM_PREFIX,
                sizeof(WINE_SYSTEM_PREFIX) - 1
                )
       ) {
        progname = argv[1];
        configured_index = load_index(progname, 1);
        if (!placeholder && configured_index > 0)
            create_placeholder();
    }
    real_wine_init(argc, argv, error, error_size);
}

/**
 * To be updated when spawning windows if no index is configured yet. Windows
 * are added in order of appearance
 */
static Window top_windows[MAX_TOP_WINDOWS + 1];
/**
 * Overload XCreateWindow to keep track of created windows in configure mode or
 * return the placeholder in execution mode
 */
Window XCreateWindow(Display *display, Window parent, int x, int y,
        unsigned int width, unsigned int height, unsigned  int  border_width,
        int depth, unsigned int class,  Visual *visual, unsigned long valuemask,
        XSetWindowAttributes *attributes)
{
    static int top_windows_created = 0;
    int is_top_normal =
        parent == XDefaultRootWindow(display) &&
        (!attributes || !attributes->override_redirect);
    if (top_windows_created < configured_index && is_top_normal){
        top_windows_created++;

        if (placeholder && top_windows_created == configured_index) {
            XChangeWindowAttributes(display, placeholder, valuemask, attributes);
            return placeholder;
        }
    }

    Window w = real_XCreateWindow(display, parent, x, y, width, height,
            border_width, depth, class, visual, valuemask, attributes);

    if (
            is_top_normal &&
            configured_index <= 0 &&
            configured_index < MAX_TOP_WINDOWS
       ) {
        top_windows[top_windows_created++] = w;
    }

    return w;
}

/**
 * Overload XMapWindow to detect the first regular visible window in configure
 * mode
 */
int XMapWindow(Display *display, Window w)
{
    int visible = real_XMapWindow(display, w);
    XWindowAttributes attrs;
    XGetWindowAttributes(display, w, &attrs);
    int i;
    for (i = 0; i < MAX_TOP_WINDOWS && top_windows[i]; i++) {
        if (top_windows[i] == w) {
            save_index(basename(progname), 1, i+1);
        }
    }
    return visible;
}

/**
 * Convenience macro to automatically exit on failure to load symbols
 */
#define DLCHECKERROR(var) \
    do {                               \
        if (!var) {                    \
            fputs(dlerror(), stderr);  \
            fputs(dlerror(), stderr);  \
            exit(1);                   \
        }                              \
    } while (0);

/**
 * Early init to detect program name
 */
void __attribute__ ((constructor)) __init(void)
{
    void *real_X11 = dlopen("libX11.so", RTLD_NOW | RTLD_GLOBAL);
    DLCHECKERROR(real_X11);
    real_XCreateWindow = dlsym(real_X11, "XCreateWindow");
    DLCHECKERROR(real_XCreateWindow);
    real_XMapWindow = dlsym(real_X11, "XMapWindow");
    DLCHECKERROR(real_XMapWindow);

    char *execname = basename(program_invocation_name);
    if (
            !strcmp(execname, "wine") ||
            !strcmp(execname, "wine32") ||
            !strcmp(execname, "wine64")
       ){
        void *real_wine = dlopen("libwine.so", RTLD_NOW | RTLD_GLOBAL);
        DLCHECKERROR(real_wine);
        real_wine_init = dlsym(real_wine, "wine_init");
        DLCHECKERROR(real_wine_init);
    }
}
