#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "server.h"

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

#define HANDLE_ERROR(S, R) (R == 0 ? 0 : (S->error_handler ? S->error_handler(R) : R ))


glc_server *glc_server_create(glc_server_options *options, int *err) {
    glc_server_options def = {0x676C6332, 0600, 1024 * 1024 * 25};
    glc_server *s = malloc(sizeof(*s));
    if(!s) {
        *err = ENOMEM;
        return NULL;
    }

    if(!options) {
        options = &def;
    }

    s->max_client = 1;
    s->clients = NULL;
    s->error_handler = NULL;
    

    int e;
    ps_bufferattr_t attr;
    if((e = ps_bufferattr_init(&attr))) {
        *err = e;
        return NULL;
    }

    if((e = ps_bufferattr_setshmkey(&attr, options->shmkey))) {
        *err = e;
        return NULL;
    }

    if((e = ps_bufferattr_setflags(&attr, PS_BUFFER_PSHARED | PS_SHM_CREATE | PS_SHM_EXCL))) {
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

    if((e = ps_buffer_init(&s->buffer, &attr))) {
        *err = e;
        return NULL;
    }

    ps_bufferattr_destroy(&attr);

    if((e = ps_packet_init(&s->packet, &s->buffer))) {
        ps_buffer_destroy(&s->buffer);
        *err = e;
        return NULL;
    }

    // TODO start thread which handles client timeouts

    *err = 0;
    return s;
}

void glc_server_destroy(glc_server *server) {
    assert(server != NULL);

    ps_packet_destroy(&server->packet);
    ps_buffer_destroy(&server->buffer);
    free(server);
}

int glc_server_msg_receive(glc_server *server, glc_message_header_t *header, void **msg, size_t *msg_size, int flags) {
    int err = 0, close_e = 0;
    if((err = ps_packet_open(&server->packet, PS_PACKET_READ | flags)))
        return err;

    size_t size;
    if((err = ps_packet_getsize(&server->packet, &size)))
        goto close;

    size -= sizeof(*header);
    if((err = ps_packet_read(&server->packet, header, sizeof(*header))))
        goto close;

    void *dest = malloc(size);
    if(!dest) {
        err = ENOMEM;
        goto close;
    }

    if((err = ps_packet_read(&server->packet, dest, size)))
        goto close;

    *msg = dest;
    *msg_size = size;

close:
    close_e = ps_packet_close(&server->packet);
    assert(close_e == 0);

    return err;
}

#if 0
int glc_server_msg_get(glc_server *server, glc_server_msg *msg, int flags) {
    // TODO: split to receive and the rest (proxy like)
    int err;
    int close;

    if((err = ps_packet_open(&server->packet, PS_PACKET_READ | flags))) {
        return err;
    }

    size_t size;
    if((err = ps_packet_getsize(&server->packet, &size)))
        goto close;

    if(size < (sizeof(glc_message_header_t) + sizeof(glc_network_header_t))) {
        err = EPROTO;
        goto close;
    }


    glc_message_header_t hdr;
    size -= sizeof(hdr);
    if((err = ps_packet_read(&server->packet, &hdr, sizeof(hdr))))
        goto close;

    if(hdr.type != GLC_MESSAGE_NETWORK) {
        err = EPROTO;
        goto close;
    }



    glc_network_header_t nhdr;
    size -= sizeof(nhdr);
    if((err = ps_packet_read(&server->packet, &nhdr, sizeof(nhdr))))
        goto close;

    msg->node = nhdr.node;
    msg->header = nhdr.payload_header;
    msg->size = nhdr.payload_size;
    msg->data = malloc(msg->size);

    if(!msg->data) {
        err = ENOMEM;
        goto close;
    }

    if((err = ps_packet_read(&server->packet, msg->data, msg->size)))
        goto close;


    if(msg->header.type == GLC_MESSAGE_CONNECT) {
        glc_connect_message_t *con = (glc_connect_message_t *)msg->data;
        if((err = glc_server_client_new(server, con->shmid, &con->node)))
            return err;

        hdr.type = GLC_MESSAGE_CONNECT;
        glc_server_msg_sent(server, con->node, &hdr, con, sizeof(*con), 0);
    }

close:
    close = ps_packet_close(&server->packet);
    assert(close == 0);

    return err;
}
#endif

int glc_server_run(glc_server *server, int flags) {
    int err = 0;
    glc_message_header_t hdr;
    void *nmsg = NULL;
    void *msg = NULL;
    size_t size = 0;

    while(1) {
        if((err = HANDLE_ERROR(server, glc_server_msg_receive(server, &hdr, &nmsg, &size, flags))))
            goto end;

        if(hdr.type != GLC_MESSAGE_NETWORK && (err = HANDLE_ERROR(server, EPROTO)))
            goto end;

        glc_network_header_t *nhdr = nmsg;
        glc_message_type_t msg_type = nhdr->payload_header.type;
        msg = nmsg + sizeof(*nhdr);
        size -= sizeof(*nhdr);

        if(msg_type == GLC_MESSAGE_CONNECT) {
            glc_connect_message_t *connect = msg;
            if((err = HANDLE_ERROR(server, glc_server_client_new(server, connect->shmid, &connect->node))))
                goto end;

            if((err = HANDLE_ERROR(server, glc_server_msg_sent(server, connect->node, &nhdr->payload_header, connect, sizeof(*connect), flags))))
                goto end;
        }
        else {
            printf("unknown message\n");
        }

        free(nmsg);
        nmsg = NULL;
    }


end:
    if(nmsg != NULL)
        free(nmsg);

    return err;
}

int glc_server_set_errorhandler(glc_server *server, int (*handler)(int)) {
    assert(server != NULL);
    server->error_handler = handler;
    return 0;
}

int glc_server_msg_sent(glc_server *server, int node, glc_message_header_t *hdr, void *msg, size_t size, int flags) {
    glc_client *c = glc_server_client_get(server, node);

    int err = 0;
    if((err = ps_packet_open(&c->packet, PS_PACKET_WRITE | flags)))
        return err;

    if((err = ps_packet_write(&c->packet, hdr, sizeof(*hdr))))
        goto close;

    if((err = ps_packet_write(&c->packet, msg, size)))
        goto close;

close:
    if((err = ps_packet_close(&c->packet)))
        return err;

    return err;
}

int glc_server_msg_destroy(glc_server_msg *msg) {
    assert(msg != NULL);
    assert(msg->data != NULL);

    free(msg->data);
    return 0;
}

int glc_server_client_new(glc_server *server, int shmid, int *client) {
    glc_client *c = calloc(1, sizeof(*c));
    if(!c)
        return ENOMEM;

    c->node = server->max_client++;
    c->shmid = shmid;
    c->next = server->clients;

    int err = 0;
    ps_bufferattr_t attr;
    if((err = ps_bufferattr_init(&attr)))
        goto error;

    if((err = ps_bufferattr_setflags(&attr, PS_BUFFER_PSHARED)))
        goto error;

    if((err = ps_bufferattr_setshmid(&attr, shmid)))
        goto error;

    if((err = ps_buffer_init(&c->buffer, &attr)))
        goto error;

    if((err = ps_packet_init(&c->packet, &c->buffer)))
        goto error;

    server->clients = c;
    *client = server->max_client-1;
    return 0;

error:
    free(c);
    return err;
}

int glc_server_client_destroy(glc_server *server, int node) {
    glc_client **client = &server->clients, *del;
    while(*client) {
        if((*client)->node == node) {
            del = *client;
            *client = (*client)->next;
            free(del);
            return 0;
        }
        client = &(*client)->next;
    }
    return EINVAL;
}

glc_client *glc_server_client_get(glc_server *server, int node) {
    glc_client *client = server->clients;
    while(client) {
        if(client->node == node)
            return client;
        client = client->next;
    }
    return NULL;
}


