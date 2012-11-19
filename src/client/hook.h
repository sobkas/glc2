/**
 * \file src/client/hook.h
 * \brief hook into libraries
 * \author Sebastian Wick <wick.sebastian@gmail.com>
 * \date 2012
 */

/**
 * \defgroup client_hook hook
 *  \{
 */

#ifndef GLC2_CLIENT_HOOK_H
#define GLC2_CLIENT_HOOK_H

#define __PUBLIC __attribute__ ((visibility ("default")))
#define __PRIVATE __attribute__ ((visibility ("hidden")))

#ifdef __cplusplus
extern "C" {
#endif

void hook_init();

void hook_destroy();

/**
 * \brief register a hook to a library
 * (const char *lib_name, const char *symbol_name1, void **function_pointer1, const char *symbol_nameN, void **function_pointerN, NULL)
 * loads the functions (symbol_name) from the shared object (lib_name) into the function pointers and redirects access on
 * the shared object functions to the functions of the current shared object
 * \param lib_name name of the shared object
 * \param symbol_name1 name of the symbol
 * \param function_pointer1 a pointer in which the real symbol is loaded
 * \param symbol_nameN
 * \param function_pointerN
 * \param NULL
 * \return 0 on success otherwise an error code
 */
int hook_register(const char *lib_name, ...);

/**
 * \brief get a symbol which is overriden by hook_register
 * \param symbol name of the symbol
 * \return the symbol or NULL if not found
 */
void *hook_sym(const char *symbol);

#ifdef __cplusplus
}
#endif

#endif

/**  \} */
