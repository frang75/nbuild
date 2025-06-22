/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: wsite.h
 *
 */

/* Html5 WebSite */

#include "html5.hxx"

__EXTERN_C

WSite *wsite_create(const char_t *project_url, const char_t *respath, const char_t *webpath, const char_t *tempath, const color_t bkcolor, Html5Status *status);

void wsite_destroy(WSite **site);

void wsite_site_font(WSite *site, const char_t *font_file, Html5Status *status);

void wsite_head_font(WSite *site, const char_t *font_file, Html5Status *status);

void wsite_mono_font(WSite *site, const char_t *font_file, Html5Status *status);

void wsite_clear_comments(WSite *site);

void wsite_comment_c(WSite *site, const char_t *comment);

void wsite_comment(WSite *site, String **comment);

void wsite_head(WSite *site, Head *head, Html5Status *status);

void wsite_header(WSite *site, Header *header, Html5Status *status);

void wsite_nav(WSite *site, Nav *nav, Html5Status *status);

void wsite_lnav(WSite *site, LNav *lnav);

void wsite_post(WSite *site, Post *post);

void wsite_rcol(WSite *site, RCol *rcol);

void wsite_scode(WSite *site, SCode *code);

void wsite_footer(WSite *site, String **footer);

void wsite_add_lang(WSite *site, const char_t *lang, Html5Status *status);

void wsite_begin_lang(WSite *site, const char_t *lang, const char_t *title, Listener *listener, Html5Status *status);

void wsite_end_lang(WSite *site);

void wsite_begin_pack(WSite *site, const char_t *folder, Html5Status *status);

void wsite_end_pack(WSite *site);

void wsite_copy_landing(WSite *site, const char_t *landing_src, const char_t **except, const uint32_t except_size);

void wsite_begin_page(WSite *site, const char_t *name, const uint32_t menu_id, Listener *listener, Html5Status *status);

Header *wsite_gheader(WSite *site);

Nav *wsite_get_nav(WSite *site);

Post *wsite_get_post(WSite *site);

RCol *wsite_get_rcol(WSite *site);

SCode *wsite_get_scode(WSite *site);

__END_C
