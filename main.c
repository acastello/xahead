#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>


Window (*real_XCreateWindow)(Display *display, Window parent, int x, int y,
        unsigned int width, unsigned int height, unsigned  int  border_width,
        int depth, unsigned int class,  Visual *visual, unsigned long valuemask,
        XSetWindowAttributes *attributes);


Window orig_window;
int orig_window_index = 0;

Window XCreateWindow(Display *display, Window parent, int x, int y,
        unsigned int width, unsigned int height, unsigned  int  border_width,
        int depth, unsigned int class,  Visual *visual, unsigned long valuemask,
        XSetWindowAttributes *attributes)
{
    if (orig_window_index < 16 && parent == XDefaultRootWindow(display)) {
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
    printf("win! %lu\n", win);
    return win;
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

    XInitThreads();
    Display *dpy = XOpenDisplay(NULL);

    XSetWindowAttributes orig_attributes;
    orig_window = real_XCreateWindow(dpy, DefaultRootWindow(dpy), 10,
            10, 100, 100, 0, CopyFromParent, CopyFromParent, CopyFromParent,
            0, &orig_attributes);
    XMapWindow(dpy, orig_window);
    XFlush(dpy);
}
