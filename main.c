#define _GNU_SOURCE
#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <X11/Xlib.h>

Display *(*real_XOpenDisplay)(const char *display_name);

Window (*real_XCreateWindow)(Display *display, Window parent, int x, int y,
        unsigned int width, unsigned int height, unsigned  int  border_width,
        int depth, unsigned int class,  Visual *visual, unsigned long valuemask,
        XSetWindowAttributes *attributes);

int (*real_XMapWindow)(Display *display, Window window);

Window orig_window;
Display *XOpenDisplay(const char* display_name)
{
    printf("DPYprogname: %s\n", program_invocation_name);
    if (
            !orig_window &&
            !strcmp(
                program_invocation_name,
                "/opt/World of Warcraft 3.3.5a/algalon/Wow.exe"
                )
       ) {
        XInitThreads();
        Display *dpy = real_XOpenDisplay(NULL);

        XSetWindowAttributes orig_attributes;
        orig_window = real_XCreateWindow(dpy, DefaultRootWindow(dpy), 10,
                10, 100, 100, 0, CopyFromParent, CopyFromParent, CopyFromParent,
                0, &orig_attributes);
        XMapWindow(dpy, orig_window);
        XFlush(dpy);
    }
    return real_XOpenDisplay(display_name);
}

int orig_window_index = 0;
Window XCreateWindow(Display *display, Window parent, int x, int y,
        unsigned int width, unsigned int height, unsigned  int  border_width,
        int depth, unsigned int class,  Visual *visual, unsigned long valuemask,
        XSetWindowAttributes *attributes)
{
    printf("progname: %s\n", program_invocation_name);
    if (
            orig_window &&
            orig_window_index < 16 &&
            parent == XDefaultRootWindow(display)
       ) {
        orig_window_index++;
        if (orig_window_index == 8) {
            XChangeWindowAttributes(display, orig_window,
                    valuemask, attributes);
            printf("win: 0x%lx\n", orig_window);
            return orig_window;
        }
    }

    Window win = real_XCreateWindow(display, parent, x, y, width, height,
            border_width, depth, class, visual, valuemask, attributes);
    printf("win! 0x%lx\n", win);
    return win;
}

int XMapWindow(Display *display, Window window)
{
    int result = real_XMapWindow(display, window);
    XWindowAttributes attrs;
    XGetWindowAttributes(display, window, &attrs);

    Window root;
    Window parent;
    Window *children[1024];
    unsigned int n;
    XQueryTree(display, window, &root, &parent, (Window **) &children, &n);
    printf("map 0x%lx (0x%lx): %d %d\n", window, parent, attrs.override_redirect, attrs.map_state);

    return result;
}



void __attribute__ ((constructor)) __init(void)
{
    printf("TOPprogname: %s\n", program_invocation_name);
    void *realX11 = dlopen("libX11.so", RTLD_NOW | RTLD_GLOBAL);
    if (!realX11) {
        fputs(dlerror(), stderr);
        fputs(dlerror(), stderr);
        exit(1);
    }
    real_XOpenDisplay = dlsym(realX11, "XOpenDisplay");
    real_XCreateWindow = dlsym(realX11, "XCreateWindow");
    real_XMapWindow = dlsym(realX11, "XMapWindow");

}
