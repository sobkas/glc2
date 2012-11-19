#include <stdlib.h>

#include "event.h"

int glc_register_event(glc_event **event, void *callback, void *udata) {
    glc_event *e = malloc(sizeof(*e));
    if(!e)
        return 1;

    e->callback = callback;
    e->udata = udata;
    e->next = *event;
    *event = e;

    return 0;
}

int glc_unregister_event(glc_event **event, void *callback, void *udata) {
    glc_event *e = *event, *prev = NULL;
    while(e) {
        if(e->callback == callback && e->udata == udata) {
            if(prev)
                prev->next = e->next;
            else
                *event = e->next;

            free(e);
            return 0;
        }

        prev = e;
        e = e->next;
    }

    return 1;
}

void glc_free_event(glc_event *event) {
    glc_event *n = NULL;
    while(event) {
        n = event;
        event = event->next;
        free(n);
    }
}
