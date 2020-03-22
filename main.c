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


Window placeholder = 0;
void create_placeholder(void)
{
    if (placeholder) {
        dprintf(2, "trying to create a placeholder before using existing one");
        exit(1);
    }

    XInitThreads();
    Display *dpy = XOpenDisplay(NULL);
    XSetWindowAttributes attrs;
    placeholder = real_XCreateWindow(dpy, DefaultRootWindow(dpy), 10, 10, 100,
            100, 0, CopyFromParent, CopyFromParent, CopyFromParent, 0, &attrs);
    XMapWindow(dpy, placeholder);
    XFlush(dpy);
}

Window XCreateWindow(Display *display, Window parent, int x, int y,
        unsigned int width, unsigned int height, unsigned  int  border_width,
        int depth, unsigned int class,  Visual *visual, unsigned long valuemask,
        XSetWindowAttributes *attributes)
{
    static int top_windows_ignored = 8;
    if (parent == XDefaultRootWindow(display))
        top_windows_ignored--;

    if (placeholder && parent == XDefaultRootWindow(display) && !top_windows_ignored) {
        XChangeWindowAttributes(display, placeholder, valuemask, attributes);
        placeholder = 0;
        return placeholder;
    }

    Window win = real_XCreateWindow(display, parent, x, y, width, height,
            border_width, depth, class, visual, valuemask, attributes);
    return win;
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
        create_placeholder();
    }
    real_wine_init(argc, argv, error, error_size);
}

#define DLCHECKERROR(var) \
    do {                               \
        if (!var) {                    \
            perror(dlerror());  \
            perror(dlerror());  \
            exit(1);                   \
        }                              \
    } while (0);

void __attribute__ ((constructor)) __init(void)
{
    void *realX11 = dlopen("libX11.so", RTLD_NOW | RTLD_GLOBAL);
    DLCHECKERROR(realX11);
    real_XCreateWindow = dlsym(realX11, "XCreateWindow");
    DLCHECKERROR(real_XCreateWindow);

    char *execname = basename(program_invocation_name);
    if (
            !strcmp(execname, "wine") ||
            !strcmp(execname, "wine32") ||
            !strcmp(execname, "wine64")
       ){
        void *realwine = dlopen("libwine.so", RTLD_NOW | RTLD_GLOBAL);
        DLCHECKERROR(realwine);
        real_wine_init = dlsym(realwine, "wine_init");
        DLCHECKERROR(real_wine_init);
    }
}
