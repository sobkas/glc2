#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <dlfcn.h>

#include "elfhacks.h"
#include "hook.h"


struct {
    void *glc_handle;
    void *(*dlopen)(const char *filename, int flag);
    void *(*dlsym)(void *handle, const char *symbol);
    void *(*dlvsym)(void *handle, const char *symbol, const char *version);
    void *(*__libc_dlsym)(void *handle, const char *symbol);
} lib = {0};

struct hook_symbol {
    const char *sym_name;
    void *sym;
    void *real_sym;

    struct hook_symbol *next;
};
typedef struct hook_symbol hook_symbol;
hook_symbol *symbols = NULL;


__PUBLIC void *dlopen(const char *filename, int flag);
__PUBLIC void *dlsym(void *handle, const char *symbol);
__PUBLIC void *dlvsym(void *handle, const char *symbol, const char *version);
__PUBLIC void *__libc_dlsym(void *handle, const char *symbol);

void hook_init() {
    int err = 0;
    eh_obj_t libdl;
    eh_obj_t libc;

    if((err = eh_find_obj(&libdl, "*libdl.so*"))) {
        fprintf(stderr, "glc2 init failed: libdl.so is not present in memory\n");
        exit(1);
    }

    if((err = eh_find_sym(&libdl, "dlopen", (void *) &lib.dlopen))) {
        fprintf(stderr, "glc2 init failed: can't get real dlopen\n");
        exit(1);
    }

    if((err = eh_find_sym(&libdl, "dlsym", (void *) &lib.dlsym))) {
        fprintf(stderr, "glc2 init failed: can't get real dlsym\n");
        exit(1);
    }

    if((err = eh_find_sym(&libdl, "dlvsym", (void *) &lib.dlvsym))) {
        fprintf(stderr, "glc2 init failed: can't get real dlvsym\n");
        exit(1);
    }

    if((err = eh_find_obj(&libc, "*libc.so*"))) {
        fprintf(stderr, "glc2 init failed: libc.so is not present in memory\n");
        exit(1);
    }

    if((err = eh_find_sym(&libc, "__libc_dlsym", (void *) &lib.__libc_dlsym))) {
        fprintf(stderr, "glc2 init failed: can't get real __libc_dlsym\n");
        exit(1);
    }

    lib.glc_handle = lib.dlopen(GLC2_CLIENT_SO_NAME, RTLD_LAZY);
    if(!lib.glc_handle) {
        fprintf(stderr, "glc2 init failed: can't open glc2-capture.so for hooks\n");
        exit(1);
    }

    eh_destroy_obj(&libdl);
    eh_destroy_obj(&libc);
}


void hook_destroy() {
    hook_symbol *sym = symbols, *del = NULL;
    while(sym) {
        del = sym;
        sym = sym->next;
        free(del);
    }

    if(lib.glc_handle)
        dlclose(lib.glc_handle);
}


int hook_register(const char *lib_name, ...) {
    int err = 0;
    void *handle = lib.dlopen(lib_name, RTLD_LAZY);
    if(!handle)
        goto err;

    va_list ap;
    va_start(ap, lib_name);

    char *sym_name = NULL;
    void **real_sym = NULL;
    hook_symbol *sym = NULL;

    while((sym_name = va_arg(ap, char *))) {
        real_sym = va_arg(ap, void **);

        sym = malloc(sizeof(*sym));
        sym->sym_name = sym_name;
        sym->real_sym = lib.dlsym(handle, sym_name);
        sym->sym = lib.dlsym(lib.glc_handle, sym_name);
        sym->next = symbols;
        symbols = sym;

        if(!sym->sym || !sym->real_sym)
            goto err;

        *real_sym = sym->real_sym;
    }

    goto finish;

err:
    err = 1;
finish:
    va_end(ap);
    dlclose(handle);

    return err;
}

void *hook_sym(const char *symbol) {
    hook_symbol *sym = symbols;
    while(sym) {
        if(strcmp(sym->sym_name, symbol) == 0)
            return sym->sym;

        sym = sym->next;
    }
    return NULL;
}

void *dlopen(const char *filename, int flag) {
    return lib.dlopen(filename, flag);
}

void *dlsym(void *handle, const char *symbol) {
    void *ret = hook_sym(symbol);
    if (ret)
        return ret;

    return lib.dlsym(handle, symbol);
}

void *dlvsym(void *handle, const char *symbol, const char *version) {
    void *ret = hook_sym(symbol); /* should we too check for version? */
    if (ret)
        return ret;

    return lib.dlvsym(handle, symbol, version);
}

void *__libc_dlsym(void *handle, const char *symbol) {
    void *ret = hook_sym(symbol);
    if (ret)
        return ret;

    return lib.__libc_dlsym(handle, symbol);
}
