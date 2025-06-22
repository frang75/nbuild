/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: nav.h
 *
 */

/* Html5 Nav */

#include "html5.hxx"

__EXTERN_C

Nav *nav_create(
    const char_t *home_icon,
    const char_t *menu_icon,
    const color_t bkcolor,
    const color_t txcolor,
    const color_t hvcolor,
    const color_t secolor,
    const real32_t fsize);

void nav_destroy(Nav **nav);

void nav_clear_items(Nav *nav);

void nav_set_home(Nav *nav, const char_t *url);

void nav_add_item(Nav *nav, const char_t *text, const char_t *url, const char_t *hover);

__END_C
