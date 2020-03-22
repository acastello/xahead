#define _GNU_SOURCE
#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <X11/Xlib.h>

#define WINE_SYSTEM_PREFIX "C:\\windows\\system32\\"

Window (*real_XCreateWindow)(Display *display, Window parent, int x, int y,
        unsigned int width, unsigned int height, unsigned  int  border_width,
        int depth, unsigned int class,  Visual *visual, unsigned long valuemask,
        XSetWindowAttributes *attributes);

void (*real_wine_init)(int argc, char *argv[], char *error, int error_size);

Window placeholder;

void create_placeholder(void)
{
    if (placeholder) {
        fputs("!! Trying to recreate placeholder window", stderr);
        exit(1);
    }
    XInitThreads();
    Display *dpy = XOpenDisplay(NULL);
    placeholder = real_XCreateWindow(dpy, DefaultRootWindow(dpy), 10, 10,
            100, 100, 0, CopyFromParent, CopyFromParent, CopyFromParent, 0,NULL);
    XMapWindow(dpy, placeholder);
    XFlush(dpy);
}

Window XCreateWindow(Display *display, Window parent, int x, int y,
        unsigned int width, unsigned int height, unsigned  int  border_width,
        int depth, unsigned int class,  Visual *visual, unsigned long valuemask,
        XSetWindowAttributes *attributes)
{
    static int top_ignored = 8;
    int top_window = parent == XDefaultRootWindow(display);
    if (top_window) {
        top_ignored--;
    }

    if (placeholder && top_window && top_ignored == 0) {
        XChangeWindowAttributes(display, placeholder, valuemask, attributes);
        return placeholder;
    }

    return real_XCreateWindow(display, parent, x, y, width, height,
            border_width, depth, class, visual, valuemask, attributes);
}

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
        if (!placeholder)
            create_placeholder();
    }
    real_wine_init(argc, argv, error, error_size);
}

#define DLCHECKERROR(var) \
    do {                               \
        if (!var) {                    \
            fputs(dlerror(), stderr);  \
            fputs(dlerror(), stderr);  \
            exit(1);                   \
        }                              \
    } while (0);

void __attribute__ ((constructor)) __init(void)
{
    void *real_X11 = dlopen("libX11.so", RTLD_NOW | RTLD_GLOBAL);
    DLCHECKERROR(real_X11);
    real_XCreateWindow = dlsym(real_X11, "XCreateWindow");
    DLCHECKERROR(real_XCreateWindow);

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
