/**
 * \file src/client/event.h
 * \brief simple callback events
 * \author Sebastian Wick <wick.sebastian@gmail.com>
 * \date 2012
 */

/**
 * \defgroup client_event event
 *  \{
 */

#ifndef GLC2_CLIENT_EVENT_H
#define GLC2_CLIENT_EVENT_H

/**
 * \brief fire a glc_event
 * \param EVENT glc_event structure pointing to the start of a list
 * \param TYPE type of the callback to call (argument is allways void *)
 */
#define GLC_EVENT_FIRE(EVENT, TYPE) \
    { \
        glc_event *__event = EVENT; \
        for(;__event;__event = __event->next) { \
            ( (TYPE) __event->callback)(__event->udata); \
        } \
    }

/**
 * \brief fire a glc_event with additional arguments
 * \param EVENT glc_event structure pointing to the start of a list
 * \param TYPE type of the callback to call (type of the following arguments plus void *udata)
 * \param ARGS... arguments passed to the callbacks
 */
#define GLC_EVENT_FIRE_ARGS(EVENT, TYPE, ARGS...) \
    { \
        glc_event *__event = EVENT; \
        for(;__event;__event = __event->next) { \
            ( (TYPE) __event->callback)(ARGS, __event->udata); \
        } \
    }

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief glc_event structure
   this is a linked list. you should keep track of the first element and make sure it is initialized to NULL
 */
struct glc_event {
    /** the callback to call when the event occurs */
    void *callback;
    /** additional data passed to the callback */
    void *udata;

    /** next element of the list or NULL if last one */
    struct glc_event *next;
};
typedef struct glc_event glc_event;

/**
 * \brief register a callback for an event
 * \param event glc_event structure pointing to the start of a list
 * \param callback function which is called when the event occurs
 * \param udata data passed to the callback
 */
int glc_register_event(glc_event **event, void *callback, void *udata);

/**
 * \brief unregister a callback for an event
 * \param event glc_event structure pointing to the start of a list
 * \param callback function to unregister
 * \param udata data passed to the callback
 */
int glc_unregister_event(glc_event **event, void *callback, void *udata);

/**
 * \brief destroy all glc_event structures from the list
 * \param event glc_event structure pointing to the start of a list
 */
void glc_free_event(glc_event *event);

#ifdef __cplusplus
}
#endif

#endif

/**  \} */
