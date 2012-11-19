#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

#include "x11.h"
#include "hook.h"
#include "event.h"
#include "log.h"


struct x11_shortcut_key {
    KeyCode keycode;
    KeySym keysym;
    struct x11_shortcut_key *next;
};
typedef struct x11_shortcut_key x11_shortcut_key;

struct x11_shortcut_event {
    x11_shortcut_type type;
    int is_down;
    const x11_shortcut *shortcut;
    void (*callback)(x11_shortcut_type, x11_shortcut *, void *);
    const void *user_data;

    struct x11_shortcut_event *next;
};
typedef struct x11_shortcut_event x11_shortcut_event;

struct {
    x11_shortcut_event *shortcut_events;
    glc_event *x11_event;
    Time last_event_time;
    KeyCode last_keycode;
    x11_shortcut_key *keys_pressed;
    glc_event *destroy_drawable;

    int (*XNextEvent)(Display *display, XEvent *event_return);
	int (*XPeekEvent)(Display *, XEvent *);
	int (*XWindowEvent)(Display *, Window, long, XEvent *);
	int (*XMaskEvent)(Display *, long, XEvent *);
	Bool (*XCheckWindowEvent)(Display *, Window, long, XEvent *);
	Bool (*XCheckMaskEvent)(Display *, long, XEvent *);
	Bool (*XCheckTypedEvent)(Display *, long, XEvent *);
	Bool (*XCheckTypedWindowEvent)(Display *, Window, int, XEvent *);

	int (*XIfEvent)(Display *, XEvent *, Bool (*)(), XPointer);
	Bool (*XCheckIfEvent)(Display *, XEvent *, Bool (*)(), XPointer);
	int (*XPeekIfEvent)(Display *, XEvent *, Bool (*)(), XPointer);

    Window (*XCreateWindow)(Display *display, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width,
        int depth, unsigned int class, Visual *visual, unsigned long valuemask, XSetWindowAttributes *attributes);
    int (*XDestroyWindow)(Display *display, Window w);
    int (*XFreePixmap)(Display *display, Pixmap pixmap);
} x11 = {0};

static void x11_event(Display *dpy, XEvent *event);
static void x11_shortcut_check(Display *dpy, XEvent *ev);

__PUBLIC int XNextEvent(Display *display, XEvent *event_return);
__PUBLIC int XPeekEvent(Display *display, XEvent *event_return);
__PUBLIC int XWindowEvent(Display *display, Window w, long event_mask, XEvent *event_return);
__PUBLIC Bool XCheckWindowEvent(Display *display, Window w, long event_mask, XEvent *event_return);
__PUBLIC int XMaskEvent(Display *display, long event_mask, XEvent *event_return);
__PUBLIC Bool XCheckMaskEvent(Display *display, long event_mask, XEvent *event_return);
__PUBLIC Bool XCheckTypedEvent(Display *display, int event_type, XEvent *event_return);
__PUBLIC Bool XCheckTypedWindowEvent(Display *display, Window w, int event_type, XEvent *event_return);
__PUBLIC int XIfEvent(Display *display, XEvent *event_return, Bool (*predicate)(), XPointer arg);
__PUBLIC Bool XCheckIfEvent(Display *display, XEvent *event_return, Bool (*predicate)(), XPointer arg);
__PUBLIC int XPeekIfEvent(Display *display, XEvent *event_return, Bool (*predicate)(), XPointer arg);

__PUBLIC int XDestroyWindow(Display *display, Window w);
__PUBLIC Window XCreateWindow(Display *display, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width,
    int depth, unsigned int class, Visual *visual, unsigned long valuemask, XSetWindowAttributes *attributes);
__PUBLIC int XFreePixmap(Display *display, Pixmap pixmap);

void x11_init() {
    int err = hook_register("libX11.so.6", 
        "XNextEvent",               &x11.XNextEvent,
        "XPeekEvent",               &x11.XPeekEvent,
        "XWindowEvent",             &x11.XWindowEvent,
        "XMaskEvent",               &x11.XMaskEvent,
        "XCheckWindowEvent",        &x11.XCheckWindowEvent,
        "XCheckMaskEvent",          &x11.XCheckMaskEvent,
        "XCheckTypedEvent",         &x11.XCheckTypedEvent,
        "XCheckTypedWindowEvent",   &x11.XCheckTypedWindowEvent,
        "XIfEvent",                 &x11.XIfEvent,
        "XCheckIfEvent",            &x11.XCheckIfEvent,
        "XPeekIfEvent",             &x11.XPeekIfEvent,

        "XDestroyWindow",           &x11.XDestroyWindow,
        "XCreateWindow",            &x11.XCreateWindow,
        "XFreePixmap",              &x11.XFreePixmap,
        NULL);

    if(err) {
        fprintf(stderr, "glc2 init failed: can't register hooks for x11");
        exit(1);
    }
}

void x11_destroy() {
    x11_shortcut_event *se = x11.shortcut_events, *se_t;
    while(se) {
        se_t = se;
        se = se->next;
        free(se_t);
    }

    x11_shortcut_key *kp = x11.keys_pressed, *kp_t;
    while(kp) {
        kp_t = kp;
        kp = kp->next;
        free(kp_t);
    }

    glc_free_event(x11.x11_event);
    glc_free_event(x11.destroy_drawable);
}

x11_shortcut* x11_shortcut_create(const char *str) {
    x11_shortcut *shortcut = malloc(sizeof(*shortcut));
    if(!shortcut)
        return NULL;

    shortcut->keys = 0;
    shortcut->key = NULL;

    char *tmp = malloc(strlen(str)+1);
    if(!tmp) {
        free(shortcut);
        return NULL;
    }
    memset(tmp, 0, strlen(str)+1);
    strcpy(tmp, str);

    char *token;
    char *ptr = tmp;
    KeySym* tmp_keysym;

    while((token = strtok(ptr, " ,.-|:"))) {
        shortcut->keys++;
        tmp_keysym = realloc(shortcut->key, sizeof(*shortcut->key)*shortcut->keys);

        if(!tmp_keysym) {
            if(shortcut->key)
                free(shortcut->key);
            free(tmp);
            free(shortcut);
            return NULL;
        }

        shortcut->key = tmp_keysym;
        shortcut->key[shortcut->keys-1] = XStringToKeysym(token);
        if(shortcut->key[shortcut->keys-1] == 0) {
            glc_log(GLC_WARNING, "x11", "unknown token %s in shortcut", token);
        }
        ptr = NULL;
    }

    free(tmp);
    return shortcut;
}

int x11_register_shortcut(x11_shortcut_type type, const x11_shortcut *shortcut, void (*callback)(x11_shortcut_type, x11_shortcut *, void *), const void *user_data) {
    x11_shortcut_event *event = malloc(sizeof(*event));
    if(!event)
        return 1;

    event->type = type;
    event->is_down = 0;
    event->shortcut = shortcut;
    event->callback = callback;
    event->user_data = user_data;
    event->next = x11.shortcut_events;
    x11.shortcut_events = event;

    return 0;
}

int x11_unregister_shortcut(x11_shortcut_type type, const x11_shortcut *shortcut, void (*callback)(x11_shortcut_type, x11_shortcut *, void *), const void *udata) {
    x11_shortcut_event *del, *prev = NULL, *event = x11.shortcut_events;

    while(event) {
        if(event->next->shortcut == shortcut && (event->next->type & type) &&
          (udata == NULL || udata == event->user_data) &&
          (callback == NULL || event->next->callback == callback)) {
            if(prev) {
                prev->next = event->next;
            }
            else {
                x11.shortcut_events = event->next;
            }

            del = event;
            event = event->next;
            free(del);

            if(callback != NULL)
                return 0;
        }
        else {
            prev = event;
            event = event->next;
        }
    }

    if(callback == NULL)
        return 0;
    return 1;
}

void x11_shortcut_check(Display *dpy, XEvent *ev) {
    int i, m1, m2;
    x11_shortcut_event *event = x11.shortcut_events;
    x11_shortcut_key *pressed;
    const x11_shortcut *shortcut;
    KeySym ev_sym = XkbKeycodeToKeysym(dpy, ev->xkey.keycode, 0, 0);

    while(event != NULL) {
        /* check if all keys for an event are down*/
        m1 = 1;
        shortcut = event->shortcut;
        for(i=0; i<(shortcut->keys); i++) {
            m2 = 0;
            pressed = x11.keys_pressed;
            while(pressed != NULL) {
                if(pressed->keysym == shortcut->key[i]) {
                    m2 = 1;
                    break;
                }
                pressed = pressed->next;
            }

            if(!m2) {
                m1 = 0;
                break;
            }
        }

        if(m1) {
            /* if all are pressed, check if the key which fired this check (ev) is in the shortcut */
            shortcut = event->shortcut;
            for(i=0; i<(shortcut->keys); i++) {
                if(shortcut->key[i] == ev_sym) {
                    event->is_down = 1;
                    if(event->type & SHORTCUT_TYPE_DOWN)
                        event->callback(SHORTCUT_TYPE_DOWN, (x11_shortcut *) event->shortcut, (void *) event->user_data);
                    break;
                }
            }
        }
        if(event->is_down && !m1) {
            event->is_down = 0;
            if(event->type & SHORTCUT_TYPE_UP)
                event->callback(SHORTCUT_TYPE_UP, (x11_shortcut *) event->shortcut, (void *) event->user_data);
        }

        event = event->next;
    }
}

int x11_shortcut_destroy(x11_shortcut *shortcut) {
    x11_unregister_shortcut(SHORTCUT_TYPE_DOWN | SHORTCUT_TYPE_UP | SHORTCUT_TYPE_REPEAT, shortcut, NULL, NULL);
    free(shortcut);
    return 0;
}

int x11_register_event(void (*callback)(Display *, XEvent *, void *), void *udata) {
    return glc_register_event(&x11.x11_event, callback, udata);
}

int x11_unregister_event(void (*callback)(Display *dpy, XEvent *event, void *), void *udata) {
    return glc_unregister_event(&x11.x11_event, callback, udata);
}

void x11_event(Display *dpy, XEvent *event) {
    if (!dpy || !event)
        return;

    GLC_EVENT_FIRE_ARGS(x11.x11_event, void (*)(Display *dpy, XEvent *event, void *), dpy, event);

    // update x11.keys_pressed and call x11_shortcut_check if there is a new key
    if (event->type == KeyPress) {
        if (event->xkey.time == x11.last_event_time && event->xkey.keycode == x11.last_keycode)
            return;

        x11_shortcut_key *pressed = x11.keys_pressed;
        while(pressed != NULL) {
            if(pressed->keycode == event->xkey.keycode) {
                return;
            }
            pressed = pressed->next;
        }

        x11_shortcut_key *key = malloc(sizeof(*key));
        if(!key)
            return;
        key->next = x11.keys_pressed;
        x11.keys_pressed = key;
        key->keycode = event->xkey.keycode;
        key->keysym = XkbKeycodeToKeysym(dpy, key->keycode, 0, 0);


        x11_shortcut_check(dpy, event);
    }
    else if(event->type == KeyRelease) {
        x11_shortcut_key *del, *pressed = x11.keys_pressed;
        x11.last_event_time = event->xkey.time;
        x11.last_keycode = event->xkey.keycode;

        if(XEventsQueued(dpy, QueuedAfterReading)) {
            XEvent nev;
            x11.XPeekEvent(dpy, &nev);
            if(nev.type == KeyPress && nev.xkey.time == event->xkey.time &&
              nev.xkey.keycode == event->xkey.keycode) {
                /* key repeat */
                return;
            }
        }

        if(pressed != NULL && pressed->keycode == event->xkey.keycode) {
            del = pressed;
            x11.keys_pressed = pressed = pressed->next;
            free(del);
        }

        while(pressed != NULL && pressed->next != NULL) {
            if(pressed->next->keycode == event->xkey.keycode) {
                del = pressed->next;
                pressed->next = pressed->next->next;

                free(del);
            }
            pressed = pressed->next;
        }

        x11_shortcut_check(dpy, event);
    }
}

int x11_register_destroy_drawable(void (*callback)(Drawable, void *), void *udata) {
    return glc_register_event(&x11.destroy_drawable, callback, udata);
}

int x11_unregister_destroy_drawable(void (*callback)(Drawable, void *), void *udata) {
    return glc_unregister_event(&x11.destroy_drawable, callback, udata);
}

int x11_get_calibration(Display *dpy, Drawable d, x11_calibration *calibration) {
    // this function is inspired by xcalib
    XWindowAttributes attr;
    int ramp_size,
        screen;
    if(!XGetWindowAttributes(dpy, d, &attr))
        return 1;
    screen = XScreenNumberOfScreen(attr.screen);
    if(!XF86VidModeGetGammaRampSize(dpy, screen, &ramp_size))
        return 2;

    unsigned short r_ramp[ramp_size];
    unsigned short g_ramp[ramp_size];
    unsigned short b_ramp[ramp_size];
    XF86VidModeGetGammaRamp(dpy, screen, ramp_size, r_ramp, g_ramp, b_ramp);

    float redMin = (double)r_ramp[0] / 65535.0;
    float redMax = (double)r_ramp[ramp_size - 1] / 65535.0;
    calibration->r_brightness = redMin * 100;
    calibration->r_contrast = (redMax - redMin) / (1.0 - redMin) * 100.0;
    calibration->r_gamma = log((r_ramp[ramp_size/2]/65535.0 - redMin) / (redMax-redMin)) / log(.5) * 100;

    float greenMin = (double)g_ramp[0] / 65535.0;
    float greenMax = (double)g_ramp[ramp_size - 1] / 65535.0;
    calibration->g_brightness = greenMin * 100;
    calibration->g_contrast = (greenMax - greenMin) / (1.0 - greenMin) * 100.0;
    calibration->g_gamma = log((g_ramp[ramp_size/2]/65535.0 - greenMin) / (greenMax-greenMin)) / log(.5) * 100;

    float blueMin = (double)b_ramp[0] / 65535.0;
    float blueMax = (double)b_ramp[ramp_size - 1] / 65535.0;
    calibration->b_brightness = blueMin * 100;
    calibration->b_contrast = (blueMax - blueMin) / (1.0 - blueMin) * 100.0;
    calibration->b_gamma = log((b_ramp[ramp_size/2]/65535.0 - blueMin) / (blueMax-blueMin)) / log(.5) * 100;

    return 0;
}

int XNextEvent(Display *display, XEvent *event_return) {
    int ret = x11.XNextEvent(display, event_return);
    x11_event(display, event_return);
    return ret;
}

int XPeekEvent(Display *display, XEvent *event_return) {
    int ret = x11.XPeekEvent(display, event_return);
    x11_event(display, event_return);
    return ret;
}

int XWindowEvent(Display *display, Window w, long event_mask, XEvent *event_return) {
    int ret = x11.XWindowEvent(display, w, event_mask, event_return);
    x11_event(display, event_return);
    return ret;
}

Bool XCheckWindowEvent(Display *display, Window w, long event_mask, XEvent *event_return) {
    Bool ret = x11.XCheckWindowEvent(display, w, event_mask, event_return);
    if (ret)
	    x11_event(display, event_return);
    return ret;
}

int XMaskEvent(Display *display, long event_mask, XEvent *event_return) {
    int ret = x11.XMaskEvent(display, event_mask, event_return);
    x11_event(display, event_return);
    return ret;
}

Bool XCheckMaskEvent(Display *display, long event_mask, XEvent *event_return) {
    Bool ret = x11.XCheckMaskEvent(display, event_mask, event_return);
    if (ret)
        x11_event(display, event_return);
    return ret;
}

Bool XCheckTypedEvent(Display *display, int event_type, XEvent *event_return) {
    Bool ret = x11.XCheckTypedEvent(display, event_type, event_return);
    if (ret)
        x11_event(display, event_return);
    return ret;
}

Bool XCheckTypedWindowEvent(Display *display, Window w, int event_type, XEvent *event_return) {
    Bool ret = x11.XCheckTypedWindowEvent(display, w, event_type, event_return);
    if (ret)
        x11_event(display, event_return);
    return ret;
}

int XIfEvent(Display *display, XEvent *event_return, Bool (*predicate)(), XPointer arg) {
    int ret = x11.XIfEvent(display, event_return, predicate, arg);
    x11_event(display, event_return);
    return ret;
}

Bool XCheckIfEvent(Display *display, XEvent *event_return, Bool (*predicate)(), XPointer arg) {
    Bool ret = x11.XCheckIfEvent(display, event_return, predicate, arg);
    if (ret)
        x11_event(display, event_return);
    return ret;
}

int XPeekIfEvent(Display *display, XEvent *event_return, Bool (*predicate)(), XPointer arg) {
    int ret = x11.XPeekIfEvent(display, event_return, predicate, arg);
    x11_event(display, event_return);
    return ret;
}

Window XCreateWindow(Display *display, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width,
  int depth, unsigned int class, Visual *visual, unsigned long valuemask, XSetWindowAttributes *attributes) {
    return x11.XCreateWindow(display, parent, x, y, width, height, border_width, depth, class, visual, valuemask, attributes);
}

int XDestroyWindow(Display *display, Window w) {
    GLC_EVENT_FIRE_ARGS(x11.destroy_drawable, void (*)(Drawable, void *), w);
    return x11.XDestroyWindow(display, w);
}

int XFreePixmap(Display *display, Pixmap pixmap) {
    GLC_EVENT_FIRE_ARGS(x11.destroy_drawable, void (*)(Drawable, void *), pixmap);
    return x11.XFreePixmap(display, pixmap);
}

