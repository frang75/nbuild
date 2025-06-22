/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: wsite.inl
 *
 */

/* Html5 WebSite */

#include "html5.ixx"

__EXTERN_C

void wsite_add_res(WSite *site, const char_t *resname, Html5Status *status);

void wsite_add_icon(WSite *site, const char_t *resname, const char_t *outname, const color_t color, const uint32_t width, Html5Status *status);

void wsite_stm_res(const WSite *site, Stream *html);

uint32_t wsite_level(const WSite *site);

real32_t wsite_header_height(const WSite *site);

real32_t wsite_nav_height(const WSite *site);

real32_t wsite_lnav_width(const WSite *site);

real32_t wsite_post_width(const WSite *site);

real32_t wsite_rcol_width(const WSite *site);

Image *wsite_image(const WSite *site, const char_t *resname, Html5Status *status);

String *wsite_optimize_img(const WSite *site, const char_t *imgpath, const uint32_t max_width, uint32_t *width, Html5Status *status);

String *wsite_math_img(const WSite *site, const char_t *math_data, const uint32_t math_size, const char_t *name, const bool_t clip_borders, uint32_t *width, Html5Status *status);

String *wsite_math_txt(const WSite *site, const char_t *name);

const char_t *wsite_lang(const WSite *site);

const char_t *wsite_folder(const WSite *site);

const char_t *wsite_prjname(const WSite *site);

const String *wsite_get_footer(const WSite *site);

__END_C
