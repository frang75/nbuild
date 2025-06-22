/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: scode.h
 *
 */

/* Html5 Code Syntax Highlighting */

#include "html5.hxx"

__EXTERN_C

SCode *scode_create(
    const color_t clbkgnd,
    const color_t cltext,
    const color_t clline,
    const color_t clfuncs,
    const color_t cltypes,
    const color_t clconsts);

const char_t *scode_func_class(const SCode *code);

const char_t *scode_type_class(const SCode *code);

const char_t *scode_const_class(const SCode *code);

void scode_ref(const SCode *code, const char_t *list_id, Stream *html);

void scode_begin(const SCode *code, const caption_t caption, const char_t *list_id, Stream *html);

void scode_html(const SCode *code, Stream *html, const char_t *lang, const uint32_t start_line, const byte_t *data, const uint32_t size, Listener *listener);

__END_C
