#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "client.h"

#define UNUSED(x) (void)(x)

#define DUMP(S) { \
    unsigned int i; \
    const unsigned char * const px = (unsigned char*)&S; \
    for (i = 0; i < sizeof(S); ++i) printf("%02X ", px[i]); \
}

#define DUMP_P(P, S) { \
    unsigned int i; \
    const unsigned char * const px = (unsigned char*)P; \
    for (i = 0; i < S; ++i) printf("%02X ", px[i]); \
}


glc_client *glc_client_create(glc_client_options *options, int *err) {
    glc_client_options def = {0600, 1024 * 1024 * 25};
    glc_client *c = malloc(sizeof(*c));
    if(!c) {
        *err = ENOMEM;
        return NULL;
    }

    if(!options) {
        options = &def;
    }

    c->id = -1;
    c->state = GLC_CLIENT_NONE;

    int e;
    ps_bufferattr_t attr;
    if((e = ps_bufferattr_init(&attr))) {
        *err = e;
        return NULL;
    }

    if((e = ps_bufferattr_setflags(&attr, PS_BUFFER_PSHARED | PS_SHM_CREATE))) {
        *err = e;
        return NULL;
    }

    if((e = ps_bufferattr_setshmmode(&attr, options->mmode))) {
        *err = e;
        return NULL;
    }

    if((e = ps_bufferattr_setsize(&attr, options->msize))) {
        *err = e;
        return NULL;
    }

    if((e = ps_buffer_init(&c->buffer, &attr))) {
        *err = e;
        return NULL;
    }

    ps_bufferattr_destroy(&attr);

    if((e = ps_packet_init(&c->packet, &c->buffer))) {
        ps_buffer_destroy(&c->buffer);
        *err = e;
        return NULL;
    }

    // TODO start thread which handles client timeouts

    *err = 0;
    return c;
}

void glc_client_destroy(glc_client *client) {
    assert(client != NULL);

    ps_packet_destroy(&client->packet);
    ps_buffer_destroy(&client->buffer);
    free(client);
}

int glc_client_connect(glc_client *client, key_t key, size_t timeout) {
    UNUSED(timeout);

    int err = 0;

    if(client->state != GLC_CLIENT_NONE)
         return EALREADY;
    client->state = GLC_CLIENT_CONNECTING;


    ps_bufferattr_t attr;
    if((err = ps_bufferattr_init(&attr)))
        goto error1;

    if((err = ps_bufferattr_setflags(&attr, PS_BUFFER_PSHARED)))
        goto error2;

    if((err = ps_bufferattr_setshmkey(&attr, key)))
        goto error2;

    if((err = ps_buffer_init(&client->server_buffer, &attr)))
        goto error2;

    if((err = ps_packet_init(&client->server_packet, &client->server_buffer)))
        goto error3;


    glc_message_header_t hdr;
    hdr.type = GLC_MESSAGE_CONNECT;

    glc_connect_message_t msg;
    msg.node = -1;
    msg.shmid = client->buffer.shmid;

    if((err = glc_client_message_sent(client, &hdr, &msg, sizeof(msg), 0)))
        goto error4;


    glc_message_header_t *rhdr = NULL;
    glc_connect_message_t *rmsg = NULL;
    size_t rmsg_size;


    if((err = glc_client_message_receive(client, &rhdr, (void *)&rmsg, &rmsg_size, 0)))
        goto error4;

    if(rhdr->type != GLC_MESSAGE_CONNECT || rmsg->node == -1)
        return ECONNREFUSED;

    client->id = rmsg->node;

    return 0;

error4:
    ps_packet_destroy(&client->server_packet);
error3:
error2:
    ps_bufferattr_destroy(&attr);
error1:
    client->state = GLC_CLIENT_NONE;
    return err;
}

int glc_client_message_sent(glc_client *client, glc_message_header_t *phdr, void *pmsg, size_t pmsg_size, int flags) {
    if(!(client->state & GLC_CLIENT_CONNECTING) && !(client->state & GLC_CLIENT_CONNECTED))
        return ENOTCONN;

    glc_message_header_t hdr;
    hdr.type = GLC_MESSAGE_NETWORK;

    glc_network_header_t network;
    network.node = client->id;
    network.payload_header = *phdr;
    network.payload_size = pmsg_size;

    int err = 0;
    if((err = ps_packet_open(&client->server_packet, PS_PACKET_WRITE | flags)))
        return err;

    if((err = ps_packet_write(&client->server_packet, &hdr, sizeof(hdr))))
        return err;
    if((err = ps_packet_write(&client->server_packet, &network, sizeof(network))))
        return err;
    if((err = ps_packet_write(&client->server_packet, pmsg, pmsg_size)))
        return err;

    if((err = ps_packet_close(&client->server_packet)))
        return err;

    return 0;
}

int glc_client_message_receive(glc_client *client, glc_message_header_t **phdr, void **pmsg, size_t *pmsg_size, int flags) {
    int err = 0;
    if((err = ps_packet_open(&client->packet, PS_PACKET_READ | flags)))
        return err;

    size_t size;
    if((err = ps_packet_getsize(&client->packet, &size)))
        return err;

    void *data = malloc(size);
    if(!data)
        return ENOMEM;

    if((err = ps_packet_read(&client->packet, data, size)))
        return err;

    if((err = ps_packet_close(&client->packet))) {
        free(data);
        return err;
    }

    *phdr = data;
    *pmsg = data + sizeof(glc_message_header_t);
    *pmsg_size = size - sizeof(glc_message_header_t);

    return 0;
}

