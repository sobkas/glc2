// FIXME!

#ifndef GLC2_SERVER_H
#define GLC2_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct glc_server_s glc_server;
typedef struct glc_server_options_s glc_server_options;
typedef struct glc_server_msg_s glc_server_msg;

glc_server *glc_server_create(glc_server_options *options, int *err);

void glc_server_destroy(glc_server *server);

int glc_server_select(glc_server *server, glc_server_msg **msg);

#ifdef __cplusplus
}
#endif

#endif
