/**
 * \file src/client/log.h
 * \brief logging for the client
 * \author Sebastian Wick <wick.sebastian@gmail.com>
 * \date 2012
 */

/**
 * \defgroup client_log client logging
 *  \{
 */

#ifndef GLC2_CLIENT_LOG_H
#define GLC2_CLIENT_LOG_H

#include <stdio.h>

#include "format.h"

#ifdef __cplusplus
extern "C" {
#endif

void glc_log_init();

void glc_log_destroy();

/**
 * \brief log
 */
void glc_log(int level, const char *module, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif

/**  \} */
