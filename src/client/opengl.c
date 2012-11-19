#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>

#include "opengl.h"
#include "hook.h"
#include "x11.h"
#include "event.h"
#include "log.h"


typedef void (*FuncPtr)(void);
typedef void (*glGenBuffersProc)(GLsizei n, GLuint *buffers);
typedef FuncPtr (*GLXGetProcAddressProc)(const GLubyte *procName);
typedef void (*glDeleteBuffersProc)(GLsizei n, const GLuint *buffers);
typedef void (*glBufferDataProc)(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
typedef void (*glBindBufferProc)(GLenum target, GLuint buffer);
typedef GLvoid *(*glMapBufferProc)(GLenum target, GLenum access);
typedef GLboolean (*glUnmapBufferProc)(GLenum target);

struct {
    opengl_ctx *ctx;
    int pbo_init;

    glc_event *ctx_create;
    glc_event *ctx_destroy;
    glc_event *ctx_resize;
    glc_event *ctx_swap_def;
    glc_event *ctx_swap_low;
    glc_event *ctx_swap_high;
    glc_event *ctx_swap_after;
    glc_event *ctx_finish;
    glc_event *ctx_finish_after;

	GLXGetProcAddressProc glXGetProcAddress;
	glGenBuffersProc glGenBuffers;
	glDeleteBuffersProc glDeleteBuffers;
	glBufferDataProc glBufferData;
	glBindBufferProc glBindBuffer;
	glMapBufferProc glMapBuffer;
	glUnmapBufferProc glUnmapBuffer;

    FuncPtr (*glXGetProcAddressARB)(const GLubyte *proc_name);
    void (*glXSwapBuffers)(Display *dpy, GLXDrawable drawable);
    void (*glFinish)(void);

    void (*glXDestroyWindow)(Display *dpy, GLXWindow win);
    void (*glXDestroyPixmap)(Display *dpy, GLXPixmap pixmap);
    void (*glXDestroyPbuffer)(Display *dpy, GLXPbuffer pbuf);
} opengl = {0};

__PUBLIC FuncPtr glXGetProcAddressARB(const GLubyte *proc_name);
__PUBLIC void glXSwapBuffers(Display *dpy, GLXDrawable drawable);
__PUBLIC void glFinish(void);

__PUBLIC void glXDestroyWindow(Display *dpy, GLXWindow win);
__PUBLIC void glXDestroyPixmap(Display *dpy, GLXPixmap pixmap);
__PUBLIC void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf);

static int opengl_pbo_init();
static opengl_ctx *opengl_context_get(Display *dpy, GLXDrawable drawable);
static void opengl_context_destroy(GLXDrawable drawable, void *udata);


void opengl_init() {
    int err = hook_register("libGL.so.1", 
        "glXGetProcAddressARB", &opengl.glXGetProcAddressARB,
        "glXSwapBuffers",       &opengl.glXSwapBuffers,
        "glFinish",             &opengl.glFinish,
        "glXDestroyWindow",     &opengl.glXDestroyWindow,
        "glXDestroyPixmap",     &opengl.glXDestroyPixmap,
        "glXDestroyPbuffer",    &opengl.glXDestroyPbuffer,
        NULL);

    if(err) {
        fprintf(stderr, "glc2 init failed: can't register hooks for opengl");
        exit(1);
    }

    x11_register_destroy_drawable(opengl_context_destroy, NULL);
}

void opengl_destroy() {
    glc_free_event(opengl.ctx_create);
    glc_free_event(opengl.ctx_destroy);
    glc_free_event(opengl.ctx_resize);
    glc_free_event(opengl.ctx_swap_def);
    glc_free_event(opengl.ctx_swap_low);
    glc_free_event(opengl.ctx_swap_high);
    glc_free_event(opengl.ctx_swap_after);
    glc_free_event(opengl.ctx_finish);
    glc_free_event(opengl.ctx_finish_after);
}

int opengl_pbo_init() {
    if(opengl.pbo_init)
        return 0;

    const char *extensions = (const char *) glGetString(GL_EXTENSIONS);

    if(extensions == NULL)
        return EINVAL;

    if(!strstr(extensions, "GL_ARB_pixel_buffer_object"))
        return ENOTSUP;

    void *handle = dlopen("libGL.so.1", RTLD_LAZY);
    if(!handle)
        return ENOTSUP;

    opengl.glXGetProcAddress = dlsym(handle, "glXGetProcAddressARB");
    if(!opengl.glXGetProcAddress)
        return ENOTSUP;
    opengl.glGenBuffers = (glGenBuffersProc) opengl.glXGetProcAddress((const GLubyte *) "glGenBuffersARB");
    if(!opengl.glGenBuffers)
        return ENOTSUP;
    opengl.glDeleteBuffers = (glDeleteBuffersProc) opengl.glXGetProcAddress((const GLubyte *) "glDeleteBuffersARB");
    if(!opengl.glDeleteBuffers)
        return ENOTSUP;
    opengl.glBufferData = (glBufferDataProc) opengl.glXGetProcAddress((const GLubyte *) "glBufferDataARB");
    if(!opengl.glBufferData)
        return ENOTSUP;
    opengl.glBindBuffer = (glBindBufferProc) opengl.glXGetProcAddress((const GLubyte *) "glBindBufferARB");
    if(!opengl.glBindBuffer)
        return ENOTSUP;
    opengl.glMapBuffer = (glMapBufferProc) opengl.glXGetProcAddress((const GLubyte *) "glMapBufferARB");
    if(!opengl.glMapBuffer)
        return ENOTSUP;
    opengl.glUnmapBuffer = (glUnmapBufferProc) opengl.glXGetProcAddress((const GLubyte *) "glUnmapBufferARB");
    if(!opengl.glUnmapBuffer)
        return ENOTSUP;

    dlclose(handle);
    opengl.pbo_init = 1;
    return 0;
}

int opengl_pbo_supported() {
    return (opengl_pbo_init() != ENOTSUP);
}

opengl_pbo *opengl_pbo_create(unsigned int buffer_size, GLenum usage) {
    if(opengl_pbo_init())
        return NULL;

    opengl_pbo *pbo = malloc(sizeof(*pbo));
    pbo->buffer_size = buffer_size;
    pbo->usage = usage;
    pbo->active = 0;

    GLint binding;

    glc_log(GLC_DEBUG, "opengl", "creating PBO");

    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING_ARB, &binding);
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    opengl.glGenBuffers(1, &pbo->id);
    opengl.glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo->id);
    opengl.glBufferData(GL_PIXEL_PACK_BUFFER_ARB, pbo->buffer_size, NULL, pbo->usage);

    glPopAttrib();
    opengl.glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, binding);
    return pbo;
}

int opengl_pbo_destroy(opengl_pbo *pbo) {
	glc_log(GLC_DEBUG, "opengl", "destroying PBO");

	opengl.glDeleteBuffers(1, &pbo->id);
	free(pbo);
	return 0;
}

int opengl_pbo_start(opengl_pbo *pbo, int x, int y, int w, int h, GLenum format, GLenum buffer, int pack_alignment) {
    GLint binding;
    if(pbo->active)
        return EAGAIN;

    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING_ARB, &binding);
    glPushAttrib(GL_PIXEL_MODE_BIT);
    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

    opengl.glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo->id);

    glReadBuffer(buffer);
    glPixelStorei(GL_PACK_ALIGNMENT, pack_alignment);
    glReadPixels(x, y, w, h, format, GL_UNSIGNED_BYTE, NULL);

    pbo->active = 1;

    glPopClientAttrib();
    glPopAttrib();
    opengl.glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, binding);
    return 0;
}

int opengl_pbo_read(opengl_pbo *pbo, void (*callback)(void *, void *), void *udata) {
    GLvoid *buf;
    GLint binding;

    if(!pbo->active)
        return EAGAIN;

    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING_ARB, &binding);

    opengl.glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo->id);
    buf = opengl.glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
    if(!buf)
        return EINVAL;

    callback(buf, udata);

    opengl.glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
    pbo->active = 0;

    opengl.glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, binding);
    return 0;
}

int opengl_pbo_resize(opengl_pbo *pbo, unsigned int buffer_size, GLenum usage) {
    if(pbo->active)
        return EAGAIN;

    GLint binding;

    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING_ARB, &binding);
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    opengl.glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, pbo->id);
    opengl.glBufferData(GL_PIXEL_PACK_BUFFER_ARB, buffer_size, NULL, usage);

    glPopAttrib();
    opengl.glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, binding);

    pbo->buffer_size = buffer_size;
    pbo->usage = usage;

    return 0;
}

int opengl_pixels_read(void *to, int x, int y, int w, int h, GLenum format, GLenum buffer, int pack_alignment) {
    glPushAttrib(GL_PIXEL_MODE_BIT);
    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

    glReadBuffer(buffer);
    glPixelStorei(GL_PACK_ALIGNMENT, pack_alignment);
    glReadPixels(x, y, w, h, format, GL_UNSIGNED_BYTE, to);

    glPopClientAttrib();
    glPopAttrib();

    return 0;
}

int opengl_register_context_create(void (*callback)(opengl_ctx *, void *), void *udata) {
    return glc_register_event(&opengl.ctx_create, callback, udata);
}

int opengl_unregister_context_create(void (*callback)(opengl_ctx *, void *), void *udata) {
    return glc_unregister_event(&opengl.ctx_create, callback, udata);
}

int opengl_register_context_destroy(void (*callback)(opengl_ctx *, void *), void *udata) {
    return glc_register_event(&opengl.ctx_destroy, callback, udata);
}

int opengl_unregister_context_destroy(void (*callback)(opengl_ctx *, void *), void *udata) {
    return glc_unregister_event(&opengl.ctx_destroy, callback, udata);
}

int opengl_register_context_resize(void (*callback)(opengl_ctx *, int, int, void *), void *udata) {
    return glc_register_event(&opengl.ctx_resize, callback, udata);
}

int opengl_unregister_context_resize(void (*callback)(opengl_ctx *, int, int, void *), void *udata) {
    return glc_unregister_event(&opengl.ctx_resize, callback, udata);
}

int opengl_register_context_swap(void (*callback)(opengl_ctx *, void *), void *udata, int priority) {
    switch(priority) {
        case 0:
            return glc_register_event(&opengl.ctx_swap_def, callback, udata);
        case -1:
            return glc_register_event(&opengl.ctx_swap_low, callback, udata);
        case 1:
            return glc_register_event(&opengl.ctx_swap_high, callback, udata);
        default:
            return -1;
    }
}

int opengl_unregister_context_swap(void (*callback)(opengl_ctx *, void *), void *udata, int priority) {
    switch(priority) {
        case 0:
            return glc_unregister_event(&opengl.ctx_swap_def, callback, udata);
        case -1:
            return glc_unregister_event(&opengl.ctx_swap_low, callback, udata);
        case 1:
            return glc_unregister_event(&opengl.ctx_swap_high, callback, udata);
        default:
            return -1;
    }
}

int opengl_register_context_swap_after(void (*callback)(opengl_ctx *, void *), void *udata) {
    return glc_register_event(&opengl.ctx_swap_after, callback, udata);
}

int opengl_unregister_context_swap_after(void (*callback)(opengl_ctx *, void *), void *udata) {
    return glc_unregister_event(&opengl.ctx_swap_after, callback, udata);
}

int opengl_register_context_finish(void (*callback)(opengl_ctx *, void *), void *udata) {
    return glc_register_event(&opengl.ctx_finish, callback, udata);
}

int opengl_unregister_context_finish(void (*callback)(opengl_ctx *, void *), void *udata) {
    return glc_unregister_event(&opengl.ctx_finish, callback, udata);
}

int opengl_register_context_finish_after(void (*callback)(opengl_ctx *, void *), void *udata) {
    return glc_register_event(&opengl.ctx_finish_after, callback, udata);
}

int opengl_unregister_context_finish_after(void (*callback)(opengl_ctx *, void *), void *udata) {
    return glc_unregister_event(&opengl.ctx_finish_after, callback, udata);
}

opengl_ctx *opengl_context_first() {
    return opengl.ctx;
}

opengl_ctx *opengl_context_next(opengl_ctx *ctx) {
    if(!ctx)
        return opengl.ctx;
    return ctx->next;
}

opengl_ctx *opengl_context_get(Display *dpy, GLXDrawable drawable) {
    opengl_ctx *ctx = opengl.ctx;

    Window rootWindow;
    int unused;

    // return the ctx if it exists
    while(ctx) {
        if(ctx->id == drawable) {
            unsigned int width, height;

            XGetGeometry(dpy, drawable, &rootWindow, &unused, &unused, &width, &height,
                (unsigned int *) &unused, (unsigned int *) &unused);
            //glXQueryDrawable(ctx->dpy, ctx->id, GLX_WIDTH, &width);
            //glXQueryDrawable(ctx->dpy, ctx->id, GLX_HEIGHT, &height);

            if(ctx->width != width || ctx->height != height) {
                int o_width = ctx->width, o_height = ctx->height;
                ctx->width = width;
                ctx->height = height;
                GLC_EVENT_FIRE_ARGS(opengl.ctx_resize, void(*)(opengl_ctx *, int, int, void *), ctx, o_width, o_height);
            }
            return ctx;
        }
        ctx = ctx->next;
    }

    // no ctx found -> create new ctx
    ctx = malloc(sizeof(*ctx));
    ctx->id = drawable;
    ctx->dpy = dpy;

    //glXQueryDrawable(dpy, drawable, GLX_WIDTH, &ctx->width);
    //glXQueryDrawable(dpy, drawable, GLX_HEIGHT, &ctx->height);

    XGetGeometry(dpy, drawable, &rootWindow, &unused, &unused, &ctx->width, &ctx->height,
        (unsigned int *) &unused, (unsigned int *) &unused);

    ctx->next = opengl.ctx;
    opengl.ctx = ctx;
    GLC_EVENT_FIRE_ARGS(opengl.ctx_create, void(*)(opengl_ctx *, void *), ctx);

    return ctx;
}

void opengl_context_destroy(GLXDrawable drawable, void *udata) {
    (void)(udata);
    opengl_ctx *ctx = opengl.ctx, *prev = NULL;
    while(ctx) {
        if(ctx->id == drawable) {
            GLC_EVENT_FIRE_ARGS(opengl.ctx_destroy, void(*)(opengl_ctx *, void *), ctx);
            if(prev)
                prev->next = ctx->next;
            else
                opengl.ctx = ctx->next;
            free(ctx);
            return;
        }
        prev = ctx;
        ctx = ctx->next;
    }
}

FuncPtr glXGetProcAddressARB(const GLubyte *proc_name) {
    FuncPtr ret = hook_sym((char*) proc_name);
    if(ret)
        return ret;

    return opengl.glXGetProcAddressARB(proc_name);
}

void glXSwapBuffers(Display *dpy, GLXDrawable drawable) {
    opengl_ctx *ctx = opengl_context_get(dpy, drawable);

    GLC_EVENT_FIRE_ARGS(opengl.ctx_swap_high, void (*)(opengl_ctx *, void *), ctx);
    GLC_EVENT_FIRE_ARGS(opengl.ctx_swap_def, void (*)(opengl_ctx *, void *), ctx);
    GLC_EVENT_FIRE_ARGS(opengl.ctx_swap_low, void (*)(opengl_ctx *, void *), ctx);
    opengl.glXSwapBuffers(dpy, drawable);
    GLC_EVENT_FIRE_ARGS(opengl.ctx_swap_after, void (*)(opengl_ctx *, void *), ctx);

}

void glFinish(void) {
    Display *dpy = glXGetCurrentDisplay();
    GLXDrawable drawable = glXGetCurrentDrawable();
    opengl_ctx *ctx = opengl_context_get(dpy, drawable);

    GLC_EVENT_FIRE_ARGS(opengl.ctx_finish, void (*)(opengl_ctx *, void *), ctx);
    opengl.glFinish();
    GLC_EVENT_FIRE_ARGS(opengl.ctx_finish_after, void (*)(opengl_ctx *, void *), ctx);
}

void glXDestroyWindow(Display *dpy, GLXWindow win) {
    opengl_context_destroy(win, NULL);
    opengl.glXDestroyWindow(dpy, win);
}

void glXDestroyPixmap(Display *dpy, GLXPixmap pixmap) {
    opengl_context_destroy(pixmap, NULL);
    opengl.glXDestroyPixmap(dpy, pixmap);
}

void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf) {
    opengl_context_destroy(pbuf, NULL);
    opengl.glXDestroyPbuffer(dpy, pbuf);
}

