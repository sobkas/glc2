/**
 * \file src/client/x11.h
 * \brief x11 subsystem
 * \author Sebastian Wick <wick.sebastian@gmail.com>
 * \date 2012
 */

/**
 * \defgroup client_x11 x11
 *  \{
 */

#ifndef GLC2_CLIENT_X11_H
#define GLC2_CLIENT_X11_H

#define __PUBLIC __attribute__ ((visibility ("default")))
#define __PRIVATE __attribute__ ((visibility ("hidden")))

#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/XKBlib.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief x11_shortcut structure
 */
typedef struct {
    /** array of KeySym keys */
	KeySym *key;
	/** the number of keys in the array key */
	int keys;
} x11_shortcut;

/**
 * \brief type of the event to fire
 * \note SHORTCUT_TYPE_REPEAT does not fire at all
 */
typedef enum {
    /** fire the event when all keys of the shortcut are pressed */
    SHORTCUT_TYPE_DOWN = 1,
    /** fire the event when a key of the active shortcut is released */
    SHORTCUT_TYPE_UP = 2,
    /** not used yet */
    SHORTCUT_TYPE_REPEAT = 4
} x11_shortcut_type;

/**
 * \brief x11 calibration structure
 */
typedef struct {
    /** brightness from 0 to 100; default 0 */
    float r_brightness, g_brightness, b_brightness;
    /** contrast in percent; default 100 */
    float r_contrast, g_contrast, b_contrast;
    /** gamma from in percent; default 100 */
    float r_gamma, g_gamma, b_gamma;
} x11_calibration;


void x11_init();

void x11_destroy();

/**
 * \brief create a shortcut from a string
 * \param str string of keys (Xlib KeySym) sperated by " ,.-|:"
 * \return NULL on failure
 */
x11_shortcut* x11_shortcut_create(const char *str);

/**
 * \brief registers a callback to a shortcut
 * \param shortcut x11_shortcut object
 * \param callback callback function
 * \param user_data additional data which gets passed to the callback when fired
 * \return 0 on success
 */
int x11_register_shortcut(x11_shortcut_type type, const x11_shortcut *shortcut, void (*callback)(x11_shortcut_type, x11_shortcut *, void *), const void *user_data);

/**
 * \brief unregisters a callback from a shortcut
 * \param shortcut x11_shortcut object
 * \param callback callback function
 * \return 0 on success
 */
int x11_unregister_shortcut(x11_shortcut_type type, const x11_shortcut *shortcut, void (*callback)(x11_shortcut_type, x11_shortcut *, void *), const void *user_data);

/**
 * \brief destroys an x11_shortcut object
 * \param shortcut x11_shortcut object
 * \return 0 on success
 */
int x11_shortcut_destroy(x11_shortcut *shortcut);

/**
 * \brief register a function to be called whenever an x11 event occurs
 * \param callback the function to be called
 * \return 0 on success
 */
int x11_register_event(void (*callback)(Display *dpy, XEvent *event, void *), void *udata);

/**
 * \brief unregister a function
 * \param callback the function to remove
 * \return 0 on success
 */
int x11_unregister_event(void (*callback)(Display *dpy, XEvent *event, void *), void *udata);

/**
 * \brief register a function which is called whenever a drawable is destroyed using xlib
 * \param callback the function to call
 * \param udata additionall data passed to the callback
 * \return 0 on success
 */
int x11_register_destroy_drawable(void (*callback)(Drawable, void *), void *udata);

/**
 * \brief unregister a function
 * \param callback the function to remove
 * \param udata additionall data passed to the callback
 * \return 0 on success
 */
int x11_unregister_destroy_drawable(void (*callback)(Drawable, void *), void *udata);

/**
 * \brief get the calibration of the screen in which Drawable d is located
 * \param dpy the display connection
 * \param d the drawable whose screen calibration is read
 * \param calibration the structure in which the calibration information is stored
 * \return 0 on success
 */
int x11_get_calibration(Display *dpy, Drawable d, x11_calibration *calibration);

#ifdef __cplusplus
}
#endif

#endif

/**  \} */
