/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: post.h
 *
 */

/* Html5 Post */

#include "html5.hxx"

__EXTERN_C

Post *post_create(
    const real32_t width,
    const color_t bkcolor,
    const color_t txcolor,
    const color_t tocbkcolor,
    const color_t hdcolor,
    const color_t bqcolor,
    const color_t bqbkcolor,
    const color_t lkcolor,
    const color_t deccolor,
    const real32_t line_height,
    const real32_t fsize,
    const real32_t fsize_h1,
    const real32_t fsize_h2,
    const real32_t fsize_h3);

uint32_t post_level(const Post *post);

void post_nav_buttons(const Post *post, const char_t *prev_url, const char_t *prev_title, const char_t *next_url, const char_t *next_title, Stream *html);

void post_line(const Post *post, Stream *html);

void post_epig(const Post *post, Stream *html);

void post_epig_end(const Post *post, Stream *html);

void post_toc(const Post *post, Stream *html);

void post_toc_h2(const Post *post, const char_t *id, Stream *html);

void post_toc_h3(const Post *post, const char_t *id, Stream *html);

void post_toc_end(const Post *post, Stream *html, Html5Status *status);

void post_h2(const Post *post, const char_t *id, const char_t *klass, const bool_t show_number, Stream *html);

void post_h2_end(const Post *post, Stream *html);

void post_h3(const Post *post, const char_t *id, const char_t *klass, const bool_t show_number, Stream *html);

void post_h3_end(const Post *post, Stream *html);

void post_p(const Post *post, const bool_t indent, Stream *html);

void post_p_end(const Post *post, Stream *html);

void post_a(const Post *post, const char_t *url, Stream *html);

void post_a_end(const Post *post, Stream *html);

void post_bq(const Post *post, Stream *html);

void post_bq_end(const Post *post, Stream *html);

void post_li(const Post *post, const char_t *imgpath, const uint32_t width_px, Stream *html, Html5Status *status);

void post_li_end(const Post *post, Stream *html);

void post_lili(const Post *post, Stream *html);

void post_lili_end(const Post *post, Stream *html);

void post_ul_end(const Post *post, Stream *html);

void post_img(const Post *post, const char_t *imgpath, const char_t *alt, const uint32_t max_width, Stream *html, Html5Status *status);

void post_img2(const Post *post, const char_t *imgpath1, const char_t *imgpath2, const char_t *alt1, const char_t *alt2, const uint32_t max_width, Stream *html, Html5Status *status);

void post_img_end(const Post *post, Stream *html);

void post_img_caption(const Post *post, const char_t *id, Stream *html);

void post_math_img_caption(const Post *post, const char_t *id, const char_t *name, Stream *html);

void post_img_caption_end(const Post *post, Stream *html);

void post_img_ref(const Post *post, const char_t *id, Stream *html);

void post_img_math_ref(const Post *post, const char_t *id, Stream *html);

void post_img_nofigure(const Post *post, const char_t *imgpath, const uint32_t max_width, const bool_t force_width, Stream *html, Html5Status *status);

void post_math_img(const Post *post, const Stream *math, const char_t *name, const char_t *desc, Stream *html, Html5Status *status);

void post_math_inline(const Post *post, const String *math, Stream *html, Html5Status *status);

void post_footer(const Post *post, Stream *html);

__END_C
