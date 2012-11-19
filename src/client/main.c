/**
 * \file src/client/main.c
 * \brief main entry point
 * \author Sebastian Wick <wick.sebastian@gmail.com>
 * \date 2012
 */

/**
 * \defgroup client_main client main
 *  \{
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "hook.h"
#include "x11.h"
#include "opengl.h"
#include "client.h"

#define __CONSTRUCTOR __attribute__((constructor))
#define __DESTRUCTOR __attribute__((destructor))
#define UNUSED(x) (void)(x)

void glc_init();
void glc_destroy();
void glc_start();
void glc_stop();


__CONSTRUCTOR void glc_init() {
    hook_init();
    x11_init();
    opengl_init();

    opengl_register_context_create(glc_start, NULL);
}

__DESTRUCTOR void glc_destroy() {
    glc_stop();

    opengl_destroy();
    x11_destroy();
    hook_destroy();
}

void glc_start() {
    opengl_unregister_context_create(glc_start, NULL);

    /* start control thread
       create ps
       init logging
       connect to server
    */

    // FIXME
    int err = 0;
    glc_client *client = glc_client_create(NULL, &err);
    if(!client) {
        fprintf(stderr, "glc_client_create failed: %s (%d)\n", strerror(err), err);
        exit(err);
    }

    key_t key = 0x676C6332;
    if((err = glc_client_connect(client, key, -1))) {
        fprintf(stderr, "glc_client_connect failed: %s (%d)\n", strerror(err), err);
        glc_client_destroy(client);
        exit(err);
    }

    fprintf(stderr, "connected\n");

    glc_client_destroy(client);
}

void glc_stop() {
    /* stop control thread
       
    */
}


/**  \} */
