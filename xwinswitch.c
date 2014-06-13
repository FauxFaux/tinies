#include <stdio.h>
#include <X11/Xlib.h>

int main() {
    Display *disp = XOpenDisplay(NULL);
    if (!disp) {
        fprintf(stderr, "couldn't open display\n");
        return 1;
    }

    Window root = DefaultRootWindow(disp);

    XCloseDisplay(disp);
}

