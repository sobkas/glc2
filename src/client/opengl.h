/**
 * \file src/client/opengl.h
 * \brief opengl subsystem
 * \author Sebastian Wick <wick.sebastian@gmail.com>
 * \date 2012
 */

/**
 * \defgroup client_opengl opengl
 *  \{
 */

#ifndef GLC2_CLIENT_OPENGL_H
#define GLC2_CLIENT_OPENGL_H

#define __PUBLIC __attribute__ ((visibility ("default")))
#define __PRIVATE __attribute__ ((visibility ("hidden")))

#include <GL/gl.h>
#include <GL/glx.h>


#ifdef __cplusplus
extern "C" {
#endif

struct opengl_ctx_s {
    GLXDrawable id;
    Display *dpy;
    unsigned int width;
    unsigned int height;
    struct opengl_ctx_s *next;
};
typedef struct opengl_ctx_s opengl_ctx;

/**
 * \brief opengl_pbo structure
 */
typedef struct {
    /** id used by opengl */
    GLuint id;
    /** the size of the buffer which will be read */
    unsigned int buffer_size;
    /** gives opengl a hint on how the pbo is used */
    GLenum usage;
    /** saves if the pbo is active e.g. if opengl is reading or writing */
    int active;
} opengl_pbo;


void opengl_init();

void opengl_destroy();

/**
 * \brief returns if pbo is supported
 * \return 1 if supported, 0 if not
 */
int opengl_pbo_supported();

/**
 * \brief create a pbo
 * \param buffer_size the size of the buffer which will be read
 * \param usage gives opengl a hint on how the pbo is used
 * \return a opengl_pbo structure pointer or NULL
 */
opengl_pbo *opengl_pbo_create(unsigned int buffer_size, GLenum usage);

/**
 * \brief destroy a pbo
 * \param pbo the pbo to destroy
 * \return 0 on success
 */
int opengl_pbo_destroy(opengl_pbo *pbo);

/**
 * \brief start the reading proccess of opengl
 * \param pbo the pbo structure
 * \param x the x offset in pixels of the framebuffer to start reading
 * \param y the y offset in pixels of the framebuffer to start reading
 * \param w the width in pixels of the framebuffer to read
 * \param h the height in pixels of the framebuffer to read
 * \param format the pixel format in which to read
 * \param buffer read from which buffer (GL_FRONT, GL_BACK)
 * \param pack_alignment the pack alignment of the data to read (1, 2, 4, 8)
 * \return 0 on success
 */
int opengl_pbo_start(opengl_pbo *pbo, int x, int y, int w, int h, GLenum format, GLenum buffer, int pack_alignment);

/**
 * \brief read the data pushed to the client
 * \param pbo the pbo structure
 * \param callback the function to call when the data is ready (this will be called immediately)
 * \param udata data passed to the callback
 * \return 0 on success
 */
int opengl_pbo_read(opengl_pbo *pbo, void (*callback)(void *, void *), void *udata);

/**
 * \brief resize the pbo
 * \param buffer_size the size of the buffer which will be read
 * \param usage gives opengl a hint on how the pbo is used
 * \return 0 on success
 */
int opengl_pbo_resize(opengl_pbo *pbo, unsigned int buffer_size, GLenum usage);

/**
 * \brief read pixels from a framebuffer
 * \param to the buffer to read to
 * \param x the x offset in pixels of the framebuffer to start reading
 * \param y the y offset in pixels of the framebuffer to start reading
 * \param w the width in pixels of the framebuffer to read
 * \param h the height in pixels of the framebuffer to read
 * \param format the pixel format in which to read
 * \param buffer read from which buffer (GL_FRONT, GL_BACK)
 * \param pack_alignment the pack alignment of the data to read (1, 2, 4, 8)
 * \return 0 on success
 */
int opengl_pixels_read(void *to, int x, int y, int w, int h, GLenum format, GLenum buffer, int pack_alignment);

opengl_ctx *opengl_context_first();
opengl_ctx *opengl_context_next(opengl_ctx *ctx);

#if 0

/**
 * \brief register a function which is called before the front and back buffers are swapped
 * \param callback the function to call
 * \param udata data passed to the callback
 * \return 0 on success
 */
int opengl_register_before_swap_buffer(void (*callback)(Display*, GLXDrawable, void *), void *udata);

/**
 * \brief unregsiter a function
 * \param callback the function
 * \param udata data passed to the callback
 * \return 0 on success
 */
int opengl_unregister_before_swap_buffer(void (*callback)(Display*, GLXDrawable, void *), void *udata);

/**
 * \brief register a function which is called after the front and back buffers are swapped
 * \param callback the function to call
 * \param udata data passed to the callback
 * \return 0 on success
 */
int opengl_register_after_swap_buffer(void (*callback)(Display*, GLXDrawable, void *), void *udata);

/**
 * \brief unregsiter a function
 * \param callback the function
 * \param udata data passed to the callback
 * \return 0 on success
 */
int opengl_unregister_after_swap_buffer(void (*callback)(Display*, GLXDrawable, void *), void *udata);

/**
 * \brief register a function which is called when gl_finish is called
 * \param callback the function to call
 * \param udata data passed to the callback
 * \return 0 on success
 */
int opengl_register_finish(void (*callback)(void *), void *udata);

/**
 * \brief unregsiter a function
 * \param callback the function
 * \param udata data passed to the callback
 * \return 0 on success
 */
int opengl_unregister_finish(void (*callback)(void *), void *udata);

/**
 * \brief register a function which is called when opengl destroys a drawable
 * \note this cannot keep track of all drawables so you should look at the x11 subsystem too
 * \param callback the function to call
 * \param udata data passed to the callback
 * \return 0 on success
 */
int opengl_register_destroy_drawable(void (*callback)(GLXDrawable, void *), void *udata);

/**
 * \brief unregsiter a function
 * \param callback the function
 * \param udata data passed to the callback
 * \return 0 on success
 */
int opengl_unregister_destroy_drawable(void (*callback)(GLXDrawable, void *), void *udata);

#endif

int opengl_register_context_create(void (*callback)(opengl_ctx *, void *), void *udata);

int opengl_unregister_context_create(void (*callback)(opengl_ctx *, void *), void *udata);

int opengl_register_context_destroy(void (*callback)(opengl_ctx *, void *), void *udata);

int opengl_unregister_context_destroy(void (*callback)(opengl_ctx *, void *), void *udata);

int opengl_register_context_resize(void (*callback)(opengl_ctx *, int, int, void *), void *udata);

int opengl_unregister_context_resize(void (*callback)(opengl_ctx *, int, int, void *), void *udata);

int opengl_register_context_swap(void (*callback)(opengl_ctx *, void *), void *udata, int priority);

int opengl_unregister_context_swap(void (*callback)(opengl_ctx *, void *), void *udata, int priority);

int opengl_register_context_swap_after(void (*callback)(opengl_ctx *, void *), void *udata);

int opengl_unregister_context_swap_after(void (*callback)(opengl_ctx *, void *), void *udata);

int opengl_register_context_finish(void (*callback)(opengl_ctx *, void *), void *udata);

int opengl_unregister_context_finish(void (*callback)(opengl_ctx *, void *), void *udata);

int opengl_register_context_finish_after(void (*callback)(opengl_ctx *, void *), void *udata);

int opengl_unregister_context_finish_after(void (*callback)(opengl_ctx *, void *), void *udata);

#ifdef __cplusplus
}
#endif

#endif

/**  \} */
