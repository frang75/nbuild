/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: draw2d.h
 *
 */

/* Operating system 2D drawing support */

#include "draw2d.hxx"

__EXTERN_C

_draw2d_api void draw2d_start(void);

_draw2d_api void draw2d_finish(void);

_draw2d_api void draw2d_preferred_monospace(const char_t *family);

__END_C
