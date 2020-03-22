#define _GNU_SOURCE
#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <X11/Xlib.h>

#define WINE_SYSTEM_PREFIX "C:\\windows\\system32\\"

Display *(*real_XOpenDisplay)(const char *display_name);

Window (*real_XCreateWindow)(Display *display, Window parent, int x, int y,
        unsigned int width, unsigned int height, unsigned  int  border_width,
        int depth, unsigned int class,  Visual *visual, unsigned long valuemask,
        XSetWindowAttributes *attributes);

int (*real_XMapWindow)(Display *display, Window window);

Display *XOpenDisplay(const char* display_name)
{
    FILE *log = fopen("/home/alex/log", "a+");
    fprintf(log, "DISPLAY INIT %s\n", program_invocation_name);
    fclose(log);
    return real_XOpenDisplay(display_name);
}

Window placeholder;
void create_placeholder(void)
{
    if (!placeholder) {
        printf("CREprogname: %s\n", program_invocation_name);
        XInitThreads();
        Display *dpy = real_XOpenDisplay(NULL);

        XSetWindowAttributes orig_attributes;
        placeholder = real_XCreateWindow(dpy, DefaultRootWindow(dpy), 10,
                10, 100, 100, 0, CopyFromParent, CopyFromParent, CopyFromParent,
                0, &orig_attributes);
        real_XMapWindow(dpy, placeholder);
        XFlush(dpy);
    }
}

int create_window_count = 0;
Window XCreateWindow(Display *display, Window parent, int x, int y,
        unsigned int width, unsigned int height, unsigned  int  border_width,
        int depth, unsigned int class,  Visual *visual, unsigned long valuemask,
        XSetWindowAttributes *attributes)
{
    printf("progname: %s\n", program_invocation_name);
    if (
            create_window_count < 8 &&
            parent == XDefaultRootWindow(display)
       ) {
        create_window_count++;
        if (placeholder && create_window_count == 8) {
            XChangeWindowAttributes(display, placeholder,
                    valuemask, attributes);
            printf("win: 0x%lx\n", placeholder);
            return placeholder;
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

void (*real_wine_init)(int argc, char *argv[], char *error, int error_size);

int wine_init_called = 0;
void wine_init(int argc, char *argv[], char *error, int error_size)
{
    FILE *log = fopen("/home/alex/log", "a+");
    fprintf(log, "WINE_INIT %s\n", program_invocation_name);
    int i;
    for (i = 0; i < argc; i++) {
        fprintf(
                log,
                "    %d %s %d\n",
                i,
                argv[i],
                strncmp(argv[i], WINE_SYSTEM_PREFIX, sizeof(WINE_SYSTEM_PREFIX)-1)
               );
    }
    if (
            argc > 0 &&
            0 != strncmp(
                argv[1],
                WINE_SYSTEM_PREFIX,
                sizeof(WINE_SYSTEM_PREFIX) - 1)
       ) {
        wine_init_called = 1;
        create_placeholder();
    }
    fclose(log);
    real_wine_init(argc, argv, error, error_size);
}

void __attribute__ ((constructor)) __init(void)
{
    FILE *log = fopen("/home/alex/log", "a+");
    fprintf(log, "INIT %d %s\n", getpid(), program_invocation_name);
    fclose(log);
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

    void *realwine = dlopen("libwine.so", RTLD_NOW | RTLD_GLOBAL);
    if (!realX11) {
        fputs(dlerror(), stderr);
        fputs(dlerror(), stderr);
        exit(1);
    }
    real_wine_init = dlsym(realwine, "wine_init");
}
