/**
 * \file src/server/server.h
 * \brief server
 * \author Sebastian Wick <wick.sebastian@gmail.com>
 * \date 2012
 */

/**
 * \defgroup server server
 *  \{
 */
 
#ifndef GLC2_SERVER_SERVER_H
#define GLC2_SERVER_SERVER_H

#include "packetstream.h"
#include "format.h"

#define __PUBLIC __attribute__ ((visibility ("default")))

#ifdef __cplusplus
extern "C" {
#endif

typedef struct glc_server_options_s {
    /** shared memory key */
    key_t shmkey;
    /** memory mode */
    int mmode;
    /** memory size; buffer size */
    size_t msize;
} glc_server_options;

typedef struct glc_server_s {
    ps_buffer_t buffer;
    ps_packet_t packet;
    int max_client;
    struct glc_client_s *clients;
    int (*error_handler)(int);
} glc_server;

typedef struct glc_client_s {
    int node;
    int shmid;
    ps_buffer_t buffer;
    ps_packet_t packet;
    struct glc_client_s *next;
} glc_client;

typedef struct {
    int node;
    glc_message_header_t header;
    size_t size;
    void *data;
} glc_server_msg;


__PUBLIC glc_server *glc_server_create(glc_server_options *options, int *err);

__PUBLIC void glc_server_destroy(glc_server *server);

__PUBLIC int glc_server_run(glc_server *server, int flags);

__PUBLIC int glc_server_msg_receive(glc_server *server, glc_message_header_t *hdr, void **msg, size_t *msg_size,  int flags);

__PUBLIC int glc_server_msg_sent(glc_server *server, int node, glc_message_header_t *hdr, void *msg, size_t size, int flags);

__PUBLIC int glc_server_set_errorhandler(glc_server *server, int (*handler)(int));

__PUBLIC int glc_server_msg_destroy(glc_server_msg *msg);

__PUBLIC int glc_server_client_new(glc_server *server, int shmid, int *client);

__PUBLIC int glc_server_client_destroy(glc_server *server, int node);

__PUBLIC glc_client *glc_server_client_get(glc_server *server, int node);

#ifdef __cplusplus
extern "C" {
#endif

#endif

/**  \} */
