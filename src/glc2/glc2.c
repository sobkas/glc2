/**
 * \file src/glc2/glc2.c
 * \brief glc2 program
 * \author Sebastian Wick <wick.sebastian@gmail.com>
 * \date 2012
 */

#include <stdio.h>
#include <error.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
//#include <glc2-server.h>
#include "server.h" // FIXME

#define UNUSED(x) (void)(x)

glc_server *server;

void terminate(int sig) {
    glc_server_destroy(server);
    server = NULL;

    printf("server addr is %p\n", server);
    exit(sig);
}

int handle_error(int err) {
    printf("error %d\n");
    return 0;
}

int main(int argc, char **argv) {
    UNUSED(argc);
    UNUSED(argv);
    printf("main\n");

    signal(SIGINT, terminate);

    int err = 0;
    server = glc_server_create(NULL, &err);
    if(!server) {
        fprintf(stderr, "glc_server_create failed: %s (%d)\n", strerror(err), err);
        exit(err);
    }

#if 0
    glc_server_msg msg;
    while(!(err = glc_server_msg_get(server, &msg, 0))) {
        printf("nothing\n");
        glc_server_msg_destroy(&msg);
    }
#endif


    glc_server_run(server, 0);

    fprintf(stderr, "glc_server_msg_get failed: %s (%d)\n", strerror(err), err);
    terminate(err);

    return 0;
}

