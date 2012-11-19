#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>

#include "log.h"

#define UNUSED(x) (void)(x)


void glc_log_init() {
}

void glc_log_destroy() {
    
}

void glc_log(int level, const char *module, const char *format, ...) {
    UNUSED(level);
    UNUSED(module);
    UNUSED(format);
    // FIXME
}

