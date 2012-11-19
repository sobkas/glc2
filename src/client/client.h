/**
 * \file src/client/client.h
 * \brief client
 * \author Sebastian Wick <wick.sebastian@gmail.com>
 * \date 2012
 */

/**
 * \defgroup client client
 *  \{
 */

#ifndef GLC2_CLIENT_CLIENT_H
#define GLC2_CLIENT_CLIENT_H

#include "format.h"
#include "packetstream.h"

#ifdef __cplusplus
extern "C" {
#endif

enum glc_client_state {
    GLC_CLIENT_NONE = 0,
    GLC_CLIENT_CONNECTING = 1 << 0,
    GLC_CLIENT_CONNECTED = 1 << 1
};

typedef struct glc_client_options_s {
    /** memory mode */
    int mmode;
    /** memory size; buffer size */
    size_t msize;
} glc_client_options;

typedef struct glc_client_s {
    int id;
    enum glc_client_state state;
    ps_buffer_t buffer;
    ps_packet_t packet;
    ps_buffer_t server_buffer;
    ps_packet_t server_packet;
} glc_client;


glc_client *glc_client_create(glc_client_options *options, int *err);

void glc_client_destroy(glc_client *client);

int glc_client_connect(glc_client *client, key_t key, size_t timeout);

int glc_client_message_sent(glc_client *client, glc_message_header_t *phdr, void *pmsg, size_t pmsg_size, int flags);

int glc_client_message_receive(glc_client *client, glc_message_header_t **phdr, void **pmsg, size_t *pmsg_size, int flags);

#ifdef __cplusplus
}
#endif

#endif

/**  \} */
