#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>


Window (*real_XCreateWindow)(Display *display, Window parent, int x, int y,
        unsigned int width, unsigned int height, unsigned  int  border_width,
        int depth, unsigned int class,  Visual *visual, unsigned long valuemask,
        XSetWindowAttributes *attributes);

int (*real_XMapWindow)(Display *display, Window window);


Window orig_windows[16];
int orig_window_index = 0;

Window XCreateWindow(Display *display, Window parent, int x, int y,
        unsigned int width, unsigned int height, unsigned  int  border_width,
        int depth, unsigned int class,  Visual *visual, unsigned long valuemask,
        XSetWindowAttributes *attributes)
{
    if (orig_window_index < 16 && parent == XDefaultRootWindow(display)) {
        XChangeWindowAttributes(display, orig_windows[orig_window_index++],
                valuemask, attributes);
        printf("win: 0x%lx\n", orig_windows[orig_window_index]);
        return orig_windows[orig_window_index];
    }

    Window win = real_XCreateWindow(display, parent, x, y, width, height,
            border_width, depth, class, visual, valuemask, attributes);
    printf("win: %lu\n", win);
    return win;
}

int XMapWindow(Display *display, Window window)
{
    int i;
    for (i = 0; i < 16; i++) {
        if (orig_windows[i] == window) {
            printf("mapped %d 0x%lx\n", i, window);
        }
    }
    return real_XMapWindow(display, window);
}



void __attribute__ ((constructor)) __init(void)
{
    void *realX11 = dlopen("libX11.so", RTLD_NOW | RTLD_GLOBAL);
    if (!realX11) {
        fputs(dlerror(), stderr);
        fputs(dlerror(), stderr);
        exit(1);
    }
    real_XCreateWindow = dlsym(realX11, "XCreateWindow");
    real_XMapWindow = dlsym(realX11, "XMapWindow");

    XInitThreads();
    Display *dpy = XOpenDisplay(NULL);

    XSetWindowAttributes orig_attributes;
    int i;
    for (i=0; i<16; i++) {
        orig_windows[i] = real_XCreateWindow(dpy, DefaultRootWindow(dpy), 10,
                10, 100, 100, 0, CopyFromParent, CopyFromParent, CopyFromParent,
                0, &orig_attributes);
        real_XMapWindow(dpy, orig_windows[i]);
    }
    XFlush(dpy);
}
