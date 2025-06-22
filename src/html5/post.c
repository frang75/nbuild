/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: post.c
 *
 */

/* Html5 Post */

#include "post.h"
#include "post.inl"
#include "css.h"
#include "html5.h"
#include "wsite.inl"
#include <core/bhash.h>
#include <core/event.h>
#include <core/heap.h>
#include <core/hfile.h>
#include <core/stream.h>
#include <core/strings.h>
#include <sewer/bstd.h>
#include <sewer/cassert.h>
#include <sewer/types.h>

struct _post_t
{
    WSite *site;
    real32_t width;
    color_t bkcolor;
    color_t txcolor;
    color_t tocbkcolor;
    color_t hdcolor;
    color_t bqcolor;
    color_t bqbkcolor;
    color_t lkcolor;
    color_t deccolor;
    real32_t line_height;
    real32_t fsize;
    real32_t fsize_h1;
    real32_t fsize_h2;
    real32_t fsize_h3;
    uint32_t h2_index;
    uint32_t h3_index;
    uint32_t li_level;
    uint32_t img_width;
};

/* static const real32_t i_VER_PADDING = 16.f; */
static const real32_t i_HOR_PADDING = 50.f;

/*---------------------------------------------------------------------------*/

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
    const real32_t fsize_h3)
{
    Post *post = heap_new0(Post);
    post->width = width;
    post->bkcolor = bkcolor;
    post->txcolor = txcolor;
    post->tocbkcolor = tocbkcolor;
    post->hdcolor = hdcolor;
    post->bqcolor = bqcolor;
    post->bqbkcolor = bqbkcolor;
    post->lkcolor = lkcolor;
    post->deccolor = deccolor;
    post->line_height = line_height;
    post->fsize = fsize;
    post->fsize_h1 = fsize_h1;
    post->fsize_h2 = fsize_h2;
    post->fsize_h3 = fsize_h3;
    return post;
}

/*---------------------------------------------------------------------------*/

void post_destroy(Post **post)
{
    cassert_no_null(post);
    cassert_no_null(*post);
    heap_delete(post, Post);
}

/*---------------------------------------------------------------------------*/

void post_site(Post *post, WSite *site)
{
    cassert_no_null(post);
    cassert(post->site == NULL);
    post->site = site;
}

/*---------------------------------------------------------------------------*/

real32_t post_width(const Post *post)
{
    cassert_no_null(post);
    return post->width;
}

/*---------------------------------------------------------------------------*/

void post_css(const Post *post, CSS *css)
{
    real32_t w1, w2;
    String *width1, *width2;
    cassert_no_null(post);
    w1 = wsite_lnav_width(post->site);
    w2 = wsite_rcol_width(post->site);
    width1 = str_printf("calc(100%% - %.0fpx)", w1 + w2);
    width2 = str_printf("calc(100%% - %.0fpx)", w1);
    css_sel(css, ".post-div");
    css_str(css, "display", "block");
    css_str(css, "float", "left");
    css_str(css, "position", "relative");
    css_str(css, "box-sizing", "border-box");
    css_px(css, "top", 0.f);
    css_px(css, "left", 0.f);
    css_str(css, "width", tc(width1));
    css_rgba(css, "background-color", post->tocbkcolor);
    css_sele(css);

    css_media_max(css, w1 + w2 + post->width);
    css_sel(css, ".post-div");
    css_str(css, "width", tc(width2));
    css_sele(css);
    css_mediae(css);

    css_media_max(css, w1 + post->width);
    css_sel(css, ".post-div");
    /* css_px(css, "left", 0.f); */
    css_str(css, "width", "100%");
    css_sele(css);
    css_mediae(css);

    css_sel(css, ".post");
    css_rgba(css, "background-color", post->bkcolor);
    css_px4(css, "padding", 0.f, i_HOR_PADDING, 0.f, i_HOR_PADDING);
    css_str(css, "box-sizing", "border-box");
    css_px(css, "font-size", post->fsize);
    css_px(css, "line-height", post->line_height);
    css_px(css, "width", post->width);
    css_rgba(css, "color", post->txcolor);
    css_sele(css);

    css_media_max(css, post->width);
    css_sel(css, ".post");
    css_str(css, "width", "100%");
    /* css_px(css, "font-size", post->fsize + 3.f);
    css_px(css, "line-height", post->line_height + 3.f); */
    css_sele(css);
    css_mediae(css);

    css_sel(css, ".post-nav");
    css_str(css, "display", "block");
    css_px(css, "height", 40.f);
    css_px(css, "margin-top", 10.f);
    css_sele(css);

    css_sel(css, ".post-nav-button");
    css_str(css, "display", "block");
    css_px(css, "border-width", 1.f);
    css_str(css, "border-style", "solid");
    css_px2(css, "padding", 8.f, 16.f);
    css_str(css, "text-decoration", "none");
    css_px(css, "font-size", 17.f);
    css_sele(css);

    css_sel(css, ".post-nav-button:hover");
    css_str(css, "box-shadow", "0 8px 16px 0 rgba(0,0,0,0.2), 0 6px 20px 0 rgba(0,0,0,0.2)");
    css_sele(css);

    css_sel(css, ".post-nav-left");
    css_str(css, "float", "left");
    css_rgba(css, "border-color", 0xFFcccccc);
    css_rgba(css, "color", post->txcolor);
    css_sele(css);

    css_sel(css, ".post-nav-right");
    css_str(css, "float", "right");
    css_rgba(css, "border-color", post->lkcolor);
    css_rgba(css, "background-color", post->lkcolor);
    css_rgba(css, "color", 0xFFFFFFFF);
    css_sele(css);

    css_sel(css, ".post p.epig");
    css_px(css, "margin-top", 15.f);
    css_sele(css);

    css_sel(css, ".post ul");
    /* css_str(css, "list-style", "none"); */
    css_px(css, "margin-top", 10.f);
    css_px(css, "padding-left", 20.f);
    css_px(css, "padding-right", 10.f);
    css_sele(css);

    css_sel(css, ".post li");
    css_str(css, "text-align", "justify");
    css_px(css, "margin-bottom", 15.f);
    css_px(css, "font-size", post->fsize);

    /* css_px(css, "line-height", post->line_height); */
    css_sele(css);

    css_sel(css, ".post li a");
    css_rgba(css, "color", post->lkcolor);
    css_str(css, "text-decoration", "none");
    css_sele(css);

    css_sel(css, ".post li a:hover");
    /* css_str(css, "font-weight", "bold"); */
    css_str(css, "text-decoration", "underline");
    css_sele(css);

    /*
    css_sel(css, ".post li::before");
    css_str(css, "content", "\"• \"");
    css_rgba(css, "color", post->deccolor);
    css_sele(css);
    */

    css_sel(css, ".post li img");
    css_px4(css, "margin", 0.f, 5.f, 0.f, 0.f);
    css_str(css, "vertical-align", "middle");
    css_sele(css);

    css_sel(css, ".post-hr");
    css_px(css, "margin-top", post->line_height);
    css_px(css, "margin-bottom", post->line_height);
    css_px(css, "border-top-width", 1.f);
    css_rgba(css, "border-top-color", 0XFFeeeeee);
    css_sele(css);

    css_sel(css, "a.anchor");
    css_str(css, "display", "block");
    css_str(css, "position", "relative");
    css_px(css, "top", -(wsite_nav_height(post->site) + 5.f));
    css_str(css, "visibility", "hidden");
    css_sele(css);

    css_sel(css, ".post h1");
    css_px4(css, "padding", post->line_height, 0.f, post->line_height, 0.f);
    /* css_px(css, "padding-top", i_VER_PADDING); */
    css_str(css, "font-family", "HeadFont,serif");
    css_px(css, "font-size", post->fsize_h1);
    css_px(css, "line-height", post->fsize_h1 + 3.f);
    css_str(css, "font-weight", "bold");
    css_rgba(css, "color", post->hdcolor);
    css_sele(css);

    css_sel(css, ".post h2");
    css_px4(css, "margin", post->line_height, 0.f, post->line_height, 0.f);
    css_str(css, "font-family", "HeadFont,serif");
    css_px(css, "font-size", post->fsize_h2);
    css_px(css, "line-height", post->fsize_h2 + 3.f);
    css_str(css, "font-weight", "bold");
    css_rgba(css, "color", post->hdcolor);
    css_sele(css);

    css_sel(css, ".post h3");
    css_px4(css, "margin", post->line_height, 0.f, post->line_height, 0.f);
    css_str(css, "font-family", "HeadFont,serif");
    css_px(css, "font-size", post->fsize_h3);
    css_str(css, "font-weight", "bold");
    css_px(css, "line-height", post->fsize_h3 + 3.f);
    css_rgba(css, "color", post->hdcolor);
    css_sele(css);

    css_sel(css, ".post p");
    css_str(css, "text-align", "justify");
    css_px(css, "text-indent", 10.f);
    css_px(css, "margin-bottom", post->line_height);
    css_px(css, "font-size", post->fsize);
    css_px(css, "line-height", post->line_height);
    css_sele(css);

    css_sel(css, ".post p a");
    css_rgba(css, "color", post->lkcolor);
    /* css_str(css, "font-family", "MonoFont,monospace"); */
    css_str(css, "text-decoration", "none");
    css_sele(css);

    css_sel(css, ".post p a:hover");
    css_str(css, "text-decoration", "underline");
    /* css_str(css, "font-weight", "bold"); */
    css_sele(css);

    css_sel(css, "p.noind");
    css_px(css, "text-indent", 0.f);
    css_sele(css);

    css_sel(css, ".post blockquote");
    css_rgba(css, "background-color", post->bqbkcolor);
    css_rgba(css, "border-color", post->bqcolor);
    css_px4(css, "border-width", 1.f, 1.f, 1.f, 6.f);
    css_str(css, "border-style", "solid");
    css_px(css, "margin-top", post->line_height);
    css_px(css, "margin-bottom", post->line_height);
    css_px(css, "padding", 16.f);
    css_str(css, "font-style", "italic");
    css_sele(css);

    css_sel(css, ".post blockquote code");
    css_rgba(css, "background-color", post->bqbkcolor);
    css_str(css, "font-style", "normal");
    css_sele(css);

    css_sel(css, ".post blockquote a");
    css_rgba(css, "color", post->lkcolor);
    css_sele(css);

    css_sel(css, ".post figure");
    css_px4(css, "margin", post->line_height, 0.f, post->line_height, 0.f);
    css_str(css, "text-align", "center");
    css_sele(css);

    css_sel(css, ".post figure figcaption");
    css_str(css, "text-align", "center");
    css_str(css, "margin-left", "auto");
    css_str(css, "margin-right", "auto");
    css_sele(css);

    css_sel(css, ".top");
    css_px(css, "margin-bottom", 5.f);
    css_px(css, "font-size", 15.f);
    css_sele(css);

    css_sel(css, ".center");
    css_str(css, "text-align", "center");
    css_str(css, "margin-left", "auto");
    css_str(css, "margin-right", "auto");
    css_sele(css);

    css_sel(css, ".bottom");
    css_px(css, "margin-top", 5.f);
    css_px(css, "font-size", 15.f);
    css_sele(css);

    css_sel(css, ".post img");
    css_str(css, "max-width", "100%");
    css_str(css, "vertical-align", "text-top");
    css_str(css, "height", "auto");
    css_sele(css);

    css_sel(css, ".post p img");
    css_str(css, "vertical-align", "sub");
    css_sele(css);

    css_sel(css, ".post .img2 img");
    css_str(css, "box-sizing", "border-box");
    css_px4(css, "padding", 0.f, 10.f, 0.f, 10.f);
    css_str(css, "max-width", "45%");
    css_str(css, "height", "auto");
    css_sele(css);

    css_sel(css, ".post summary:hover::marker");
    css_rgba(css, "color", post->lkcolor);
    css_sele(css);

    css_sel(css, ".post .fdetails");
    css_px_important(css, "margin-top", 5);
    css_sele(css);

    css_sel(css, ".post pre");
    css_px(css, "padding", 10.f);
    css_rgba(css, "background-color", post->tocbkcolor);
    css_str(css, "overflow", "auto");
    css_str(css, "box-sizing", "border-box");
    css_str(css, "font-family", "MonoFont,monospace");
    css_px(css, "font-size", 16.f);
    css_sele(css);

    css_sel(css, ".post code");
    css_str(css, "font-family", "MonoFont,monospace");
    css_str(css, "font-weight", "500");
    css_px(css, "font-size", post->fsize);
    css_sele(css);

    css_sel(css, ".post-toc");
    css_px4(css, "margin", 20.f, 30.f, 20.f, 20.f);
    /*css_px4(css, "padding", 0.f, 10.f, 0.f, 0.f);
    css_rgba(css, "background-color", post->tocbkcolor);
    css_str(css, "border-style", "solid");
    css_px(css, "border-radius", 10.f);
    css_px(css, "border-width", 2.f);
    css_rgba(css, "border-color", post->deccolor);*/
    css_sele(css);

    css_sel(css, ".post-toc ul");
    /* css_str(css, "list-style", "none"); */
    css_px(css, "padding-left", 15.f);
    css_px4(css, "margin", 0.f, 0.f, 0.f, 0.f);
    css_px(css, "padding-left", 20.f);
    css_sele(css);

    /*css_sel(css, ".post-toc li::before");
    css_str(css, "content", "\"•  \"");
    css_rgba(css, "color", post->deccolor);
    css_sele(css);*/

    css_sel(css, ".post-toc ul li");
    css_px(css, "font-size", post->fsize);
    css_px4(css, "margin", 0.f, 0.f, 0.f, 0.f);
    css_str(css, "text-align", "left");
    css_sele(css);

    css_sel(css, ".post-toc li a");
    css_rgba(css, "color", post->hdcolor);
    css_str(css, "text-decoration", "none");
    css_sele(css);

    css_sel(css, ".post-toc li a:hover");
    css_str(css, "text-decoration", "underline");
    /* css_str(css, "font-weight", "bold"); */
    css_sele(css);

    css_sel(css, ".post figcaption a");
    css_rgba(css, "color", post->lkcolor);
    css_str(css, "text-decoration", "none");
    css_sele(css);

    css_sel(css, ".post figcaption a:hover");
    css_str(css, "text-decoration", "underline");
    /* css_str(css, "font-weight", "bold"); */
    css_sele(css);

    css_sel(css, ".post-dec, .post-dec a");
    css_rgba_important(css, "color", post->deccolor);
    css_str(css, "font-weight", "bold");
    css_sele(css);

    /*     css_sel(css, ".post-dec a");
    css_rgba_important(css, "color", post->deccolor);
    css_str(css, "font-weight", "bold");
    css_sele(css); */

    css_media_max(css, post->width);
    /*css_sel(css, ".post p, li, blockquote, code");
    css_px(css, "font-size", 18.f);
    css_sele(css);*/

    /*css_sel(css, ".post figcaption");
    css_px(css, "font-size", 16.f);
    css_sele(css);*/

    css_sel(css, ".post .img2 img");
    css_px4(css, "padding", 10.f, 0.f, 10.f, 0.f);
    css_str(css, "max-width", "100%");
    css_sele(css);

    /*css_sel(css, ".post pre");
    css_px(css, "font-size", 18.f);
    css_sele(css);*/

    css_mediae(css);

    css_media_max(css, 400.f);
    css_sel(css, ".post");
    css_px4(css, "padding", 0.f, 15.f, 0.f, 15.f);
    css_sele(css);
    css_sel(css, ".post p");
    css_str(css, "text-align", "left");
    css_sele(css);
    css_sel(css, ".post li");
    css_str(css, "text-align", "left");
    css_sele(css);
    css_mediae(css);

    css_sel(css, ".post footer");
    css_str(css, "color", "#999");
    css_str(css, "line-height", "normal");
    css_str(css, "text-align", "center");
    css_px(css, "padding-bottom", 30.f);
    css_px(css, "font-size", 12.f);
    css_sele(css);

    css_sel(css, ".post footer a");
    css_str(css, "color", "#999");
    css_sele(css);

    css_direct(css, ".nowrap { white-space: nowrap; overflow: hidden; }");

    str_destroy(&width1);
    str_destroy(&width2);
}

/*---------------------------------------------------------------------------*/

void post_js(const Post *post, const jssec_t sec, Stream *js)
{
    cassert_no_null(post);
    switch (sec)
    {
    case ekJS_CODE:
        stm_writef(js, "\nvar post = document.getElementsByClassName(\"post-div\")[0];\n");
        stm_writef(js, "function on_post_wscroll() {\n");
        stm_printf(js, "if (window.pageYOffset >= %.0f)\n", wsite_header_height(post->site));
        stm_writef(js, "    post.style.left = lnav.offsetWidth;\n"); /* %.2f\n", wsite_lnav_width(post->site)); */
        stm_writef(js, "else\n");
        stm_writef(js, "    post.style.left = 0;\n");
        stm_writef(js, "}\n");
        break;

    case ekJS_ONSCROLL:
        stm_writef(js, "on_post_wscroll();\n");
        break;

    case ekJS_ONRESIZE:
        stm_writef(js, "on_post_wscroll();\n");
        break;

    case ekJS_ONLOAD:
        break;
        cassert_default();
    }
}

/*---------------------------------------------------------------------------*/

void post_html(const Post *post, Stream *html, Listener *listener)
{
    unref(post);
    stm_writef(html, "\n<div class='post-div'>\n");
    stm_writef(html, "<div class='post'>\n");
    stm_writef(html, "<h1>");
    listener_event(listener, ekPAGE_TITLE, post, html, NULL, Post, Stream, void);
    stm_writef(html, "</h1>\n");
    listener_event(listener, ekPAGE_CONTENT, post, html, NULL, Post, Stream, void);
    stm_writef(html, "\n");
    stm_writef(html, "</div> <!-- post -->\n");
    stm_writef(html, "</div> <!-- post-div -->\n");
}

/*---------------------------------------------------------------------------*/

uint32_t post_level(const Post *post)
{
    cassert_no_null(post);
    return wsite_level(post->site);
}

/*---------------------------------------------------------------------------*/

void post_nav_buttons(const Post *post, const char_t *prev_url, const char_t *prev_title, const char_t *next_url, const char_t *next_title, Stream *html)
{
    if (prev_url == NULL && next_url == NULL)
        return;

    stm_writef(html, "\n<div class='post-nav'>\n");

    if (prev_url != NULL)
    {
        stm_printf(html, "\n<a href='%s' title='%s'>\n", prev_url, prev_title);
        stm_writef(html, "<div class='post-nav-button post-nav-left'>\n");
        if (str_equ_c(wsite_lang(post->site), "es") == TRUE)
            stm_writef(html, "❮ Anterior");
        else
            stm_writef(html, "❮ Back");
        stm_writef(html, "\n</div> <!-- post-nav-button post-nav-left -->\n");
        stm_writef(html, "</a>\n");
    }

    if (next_url != NULL)
    {
        stm_printf(html, "\n<a href='%s' title='%s'>\n", next_url, next_title);
        stm_writef(html, "<div class='post-nav-button post-nav-right'>\n");
        if (str_equ_c(wsite_lang(post->site), "es") == TRUE)
            stm_writef(html, "Siguiente ❯");
        else
            stm_writef(html, "Next ❯");
        stm_writef(html, "\n</div> <!-- post-nav-button post-nav-right -->\n");
        stm_writef(html, "</a>\n");
    }

    stm_writef(html, "\n</div> <!-- post-nav -->\n");
    ((Post *)post)->h2_index = 0;
    ((Post *)post)->h3_index = 0;
}

/*---------------------------------------------------------------------------*/

static void i_close_li(const Post *post, Stream *html)
{
    uint32_t i = 0;
    cassert_no_null(post);
    for (i = 0; i < post->li_level; ++i)
        stm_writef(html, "</ul>\n");
    ((Post *)post)->li_level = 0;
}

/*---------------------------------------------------------------------------*/

void post_line(const Post *post, Stream *html)
{
    i_close_li(post, html);
    stm_writef(html, "\n<hr class='post-hr'>\n");
}

/*---------------------------------------------------------------------------*/

void post_epig(const Post *post, Stream *html)
{
    cassert_no_null(post);
    cassert(post->li_level == 0);
    stm_writef(html, "\n<p class='epig'><i>");
}

/*---------------------------------------------------------------------------*/

void post_epig_end(const Post *post, Stream *html)
{
    cassert_no_null(post);
    cassert(post->li_level == 0);
    stm_writef(html, "</i></p>\n");
}

/*---------------------------------------------------------------------------*/

void post_toc(const Post *post, Stream *html)
{
    cassert_no_null(post);
    cassert(post->li_level == 0);
    ((Post *)post)->h2_index = 0;
    ((Post *)post)->h3_index = 0;
    stm_writef(html, "\n<div class='post-toc'>\n");
    stm_writef(html, "<ul>\n");
}

/*---------------------------------------------------------------------------*/

void post_toc_h2(const Post *post, const char_t *id, Stream *html)
{
    cassert_no_null(post);
    cassert(post->li_level == 0);
    if (post->h3_index > 0)
    {
        stm_writef(html, "</a></li>\n");
        stm_writef(html, "</ul>\n");
        ((Post *)post)->h3_index = 0;
    }
    else if (post->h2_index > 0)
    {
        stm_writef(html, "</a></li>\n");
    }

    ((Post *)post)->h2_index += 1;
    stm_printf(html, "<li><a href='#%s'>%d. ", id, post->h2_index);
}

/*---------------------------------------------------------------------------*/

void post_toc_h3(const Post *post, const char_t *id, Stream *html)
{
    cassert_no_null(post);
    cassert(post->li_level == 0);
    if (post->h3_index > 0)
    {
        stm_writef(html, "</a></li>\n");
    }
    else
    {
        cassert(post->h2_index > 0);
        stm_writef(html, "</a></li>\n");
        stm_writef(html, "<ul>\n");
    }

    ((Post *)post)->h3_index += 1;
    stm_printf(html, "<li><a href='#%s'>%d.%d. ", id, post->h2_index, post->h3_index);
}

/*---------------------------------------------------------------------------*/

void post_toc_end(const Post *post, Stream *html, Html5Status *status)
{
    cassert_no_null(post);
    cassert(post->li_level == 0);
    if (post->h3_index > 0)
    {
        stm_writef(html, "</a></li>\n");
        stm_writef(html, "</ul>\n");
    }
    else if (post->h2_index > 0)
    {
        stm_writef(html, "</a></li>\n");
    }
    else
    {
        String *err = str_printf("[post]-Empty Table-Of-Content.");
        html5_status_err(status, &err);
    }

    stm_writef(html, "<ul>\n");
    stm_writef(html, "</div> <!-- post-toc -->\n");
    ((Post *)post)->h2_index = 0;
    ((Post *)post)->h3_index = 0;
}

/*---------------------------------------------------------------------------*/

void post_h2(const Post *post, const char_t *id, const char_t *klass, const bool_t show_number, Stream *html)
{
    ((Post *)post)->h2_index += 1;
    ((Post *)post)->h3_index = 0;
    i_close_li(post, html);
    stm_printf(html, "\n<a name='%s' class='anchor'></a>\n", id);
    if (show_number == TRUE)
    {
        if (str_empty_c(klass) == TRUE)
            stm_printf(html, "<h2>%d. ", post->h2_index);
        else
            stm_printf(html, "<h2 class='%s'>%d. ", klass, post->h2_index);
    }
    else
    {
        if (str_empty_c(klass) == TRUE)
            stm_writef(html, "<h2>");
        else
            stm_printf(html, "<h2 class='%s'>", klass);
    }
}

/*---------------------------------------------------------------------------*/

void post_h2_end(const Post *post, Stream *html)
{
    unref(post);
    stm_writef(html, "</h2>\n");
}

/*---------------------------------------------------------------------------*/

void post_h3(const Post *post, const char_t *id, const char_t *klass, const bool_t show_number, Stream *html)
{
    ((Post *)post)->h3_index += 1;
    i_close_li(post, html);
    stm_printf(html, "\n<a name='%s' class='anchor'></a>\n", id);
    if (show_number == TRUE)
    {
        if (str_empty_c(klass) == TRUE)
            stm_printf(html, "<h3>%d.%d. ", post->h2_index, post->h3_index);
        else
            stm_printf(html, "<h3 class='%s'>%d.%d. ", klass, post->h2_index, post->h3_index);
    }
    else
    {
        if (str_empty_c(klass) == TRUE)
            stm_writef(html, "<h3>");
        else
            stm_printf(html, "<h3 class='%s'>", klass);
    }
}

/*---------------------------------------------------------------------------*/

void post_h3_end(const Post *post, Stream *html)
{
    unref(post);
    stm_writef(html, "</h3>\n");
}

/*---------------------------------------------------------------------------*/

void post_p(const Post *post, const bool_t indent, Stream *html)
{
    i_close_li(post, html);
    if (indent)
        stm_writef(html, "\n<p>");
    else
        stm_writef(html, "\n<p class='noind'>");
}

/*---------------------------------------------------------------------------*/

void post_p_end(const Post *post, Stream *html)
{
    unref(post);
    stm_writef(html, "</p>\n");
}

/*---------------------------------------------------------------------------*/

void post_a(const Post *post, const char_t *url, Stream *html)
{
    unref(post);
    stm_printf(html, "<a href='%s' title='%s'>", url, url);
}

/*---------------------------------------------------------------------------*/

void post_a_end(const Post *post, Stream *html)
{
    unref(post);
    stm_printf(html, "</a>");
}

/*---------------------------------------------------------------------------*/

void post_bq(const Post *post, Stream *html)
{
    i_close_li(post, html);
    stm_writef(html, "\n<blockquote>");
}

/*---------------------------------------------------------------------------*/

void post_bq_end(const Post *post, Stream *html)
{
    unref(post);
    stm_writef(html, "</blockquote>\n");
}

/*---------------------------------------------------------------------------*/

static void i_img(const Post *post, const char_t *imgpath, const char_t *alt, const uint32_t max_width, const bool_t force_width, uint32_t *width, Stream *html, Html5Status *status)
{
    String *imgbase;
    cassert_no_null(post);
    cassert_no_null(width);
    imgbase = wsite_optimize_img(post->site, imgpath, max_width, width, status);
    if (imgbase != NULL)
    {
        if (force_width == TRUE)
            stm_printf(html, "<img src='../../img/%s/%s' alt='%s' style='min-width:%dpx'/>", wsite_folder(post->site), tc(imgbase), alt ? alt : "", *width);
        else
            stm_printf(html, "<img src='../../img/%s/%s' alt='%s' />", wsite_folder(post->site), tc(imgbase), alt ? alt : "");

        str_destroy(&imgbase);
    }
    else
    {
        String *err = str_printf("[post]-Error processing image '%s'.", imgpath);
        html5_status_err(status, &err);
        stm_writef(html, "<img src='UNKNOWN'/>\n");
    }
}

/*---------------------------------------------------------------------------*/

void post_li(const Post *post, const char_t *imgpath, const uint32_t width_px, Stream *html, Html5Status *status)
{
    if (post->li_level == 0)
    {
        stm_writef(html, "\n<ul>\n");
    }
    else if (post->li_level == 2)
    {
        stm_writef(html, "</ul>\n");
    }
    else
    {
        cassert(post->li_level == 1);
    }

    ((Post *)post)->li_level = 1;
    stm_writef(html, "\n<li>");
    if (imgpath != NULL)
    {
        uint32_t width;
        i_img(post, imgpath, NULL, width_px, TRUE, &width, html, status);
    }
}

/*---------------------------------------------------------------------------*/

void post_li_end(const Post *post, Stream *html)
{
    unref(post);
    stm_writef(html, "</li>\n");
}

/*---------------------------------------------------------------------------*/

void post_lili(const Post *post, Stream *html)
{
    if (post->li_level == 1)
    {
        stm_writef(html, "<ul>\n");
    }
    else
    {
        cassert(post->li_level == 2);
    }

    ((Post *)post)->li_level = 2;
    stm_writef(html, "\n<li>");
}

/*---------------------------------------------------------------------------*/

void post_lili_end(const Post *post, Stream *html)
{
    unref(post);
    stm_writef(html, "</li>\n");
}

/*---------------------------------------------------------------------------*/

void post_ul_end(const Post *post, Stream *html)
{
    i_close_li(post, html);
}

/*---------------------------------------------------------------------------*/

void post_img(const Post *post, const char_t *imgpath, const char_t *alt, const uint32_t max_width, Stream *html, Html5Status *status)
{
    uint32_t mmax_width, width;
    cassert_no_null(post);
    mmax_width = (uint32_t)(post->width - i_HOR_PADDING * 2.f);
    if (max_width < mmax_width)
        mmax_width = max_width;
    stm_writef(html, "\n<figure>\n");
    i_img(post, imgpath, alt, mmax_width, FALSE, &width, html, status);
    stm_writef(html, "\n");
    ((Post *)post)->img_width = width;
}

/*---------------------------------------------------------------------------*/

void post_img2(const Post *post, const char_t *imgpath1, const char_t *imgpath2, const char_t *alt1, const char_t *alt2, const uint32_t max_width, Stream *html, Html5Status *status)
{
    uint32_t width1;
    uint32_t width2;
    uint32_t mmax_width;
    cassert_no_null(post);
    mmax_width = (uint32_t)((post->width - i_HOR_PADDING * 2.f) / 2.f);
    if (max_width < mmax_width)
        mmax_width = max_width;
    stm_writef(html, "\n<figure class='img2'>\n");
    i_img(post, imgpath1, alt1, mmax_width, FALSE, &width1, html, status);
    stm_writef(html, "\n");
    i_img(post, imgpath2, alt2, mmax_width, FALSE, &width2, html, status);
    stm_writef(html, "\n");
    ((Post *)post)->img_width = width1 + width2;
}

/*---------------------------------------------------------------------------*/

void post_img_end(const Post *post, Stream *html)
{
    unref(post);
    stm_writef(html, "</figure>\n");
}

/*---------------------------------------------------------------------------*/

void post_img_caption(const Post *post, const char_t *id, Stream *html)
{
    uint32_t width;
    cassert_no_null(post);
    width = max_u32(post->img_width - 20, 200);
    stm_printf(html, "<figcaption class='bottom', style='max-width:%dpx'>\n", width);
    post_img_ref(post, id, html);
    stm_writef(html, ": ");
}

/*---------------------------------------------------------------------------*/

void post_math_img_caption(const Post *post, const char_t *id, const char_t *name, Stream *html)
{
    uint32_t width;
    String *mathtxt = NULL;
    String *formula = NULL;

    cassert_no_null(post);
    width = max_u32(post->img_width - 20, 200);
    mathtxt = wsite_math_txt(post->site, name);
    formula = hfile_string(tc(mathtxt), NULL);

    stm_printf(html, "<figcaption class='bottom', style='max-width:%dpx'>\n", width);
    stm_writef(html, "<span class='post-dec'>");
    stm_printf(html, "<a href='../../img/%s/%s.txt' title='%s'>", wsite_folder(post->site), name, tc(formula));

    if (str_equ_c(wsite_lang(post->site), "es") == TRUE)
        stm_printf(html, "Fórmula %s", id);
    else
        stm_printf(html, "Formula %s", id);

    stm_writef(html, "</a></span>");

    stm_writef(html, ": ");
    str_destroy(&mathtxt);
    str_destopt(&formula);
}

/*---------------------------------------------------------------------------*/

void post_img_caption_end(const Post *post, Stream *html)
{
    unref(post);
    stm_writef(html, "</figcaption>\n");
}

/*---------------------------------------------------------------------------*/

void post_img_ref(const Post *post, const char_t *id, Stream *html)
{
    cassert_no_null(post);
    stm_writef(html, "<span class='post-dec'>");

    if (str_equ_c(wsite_lang(post->site), "es") == TRUE)
        stm_printf(html, "Figura %s", id);
    else
        stm_printf(html, "Figure %s", id);

    stm_writef(html, "</span>");
}

/*---------------------------------------------------------------------------*/

void post_img_math_ref(const Post *post, const char_t *id, Stream *html)
{
    cassert_no_null(post);
    stm_writef(html, "<span class='post-dec'>");

    if (str_equ_c(wsite_lang(post->site), "es") == TRUE)
        stm_printf(html, "Fórmula %s", id);
    else
        stm_printf(html, "Formula %s", id);

    stm_writef(html, "</span>");
}

/*---------------------------------------------------------------------------*/

void post_img_nofigure(const Post *post, const char_t *imgpath, const uint32_t max_width, const bool_t force_width, Stream *html, Html5Status *status)
{
    uint32_t width = 0;
    unref(post);
    i_img(post, imgpath, NULL, max_width, force_width, &width, html, status);
}

/*---------------------------------------------------------------------------*/

void post_math_img(const Post *post, const Stream *math, const char_t *name, const char_t *desc, Stream *html, Html5Status *status)
{
    String *imgbase = NULL;
    uint32_t width = 0;
    const char_t *math_data = (const char_t *)stm_buffer(math);
    uint32_t math_size = stm_buffer_size(math);

    cassert_no_null(post);
    stm_writef(html, "\n<figure>\n");
    imgbase = wsite_math_img(post->site, math_data, math_size, name, FALSE, &width, status);
    if (imgbase != NULL)
    {
        stm_printf(html, "<img src='../../img/%s/%s' alt='%s' />", wsite_folder(post->site), tc(imgbase), desc);
        str_destroy(&imgbase);
    }
    else
    {
        String *err = str_printf("[post]-Error processing image formula '%s'.", name);
        stm_writef(html, "<img src='UNKNOWN'/>\n");
        html5_status_err(status, &err);
    }

    stm_writef(html, "\n");
    ((Post *)post)->img_width = width;
}

/*---------------------------------------------------------------------------*/

void post_math_inline(const Post *post, const String *math, Stream *html, Html5Status *status)
{
    String *equation = str_printf("\\begin{equation*}\n%s\n\\end{equation*}\n", tc(math));
    uint32_t hash = bhash_from_block((const byte_t *)tc(equation), str_len(equation));
    char_t name[32];
    uint32_t width = 0;
    String *imgbase = NULL;

    cassert_no_null(post);
    bstd_sprintf(name, sizeof(name), "%08x", hash);

    imgbase = wsite_math_img(post->site, tc(equation), str_len(equation), name, TRUE, &width, status);
    if (imgbase != NULL)
    {
        stm_printf(html, "<img src='../../img/%s/%s' />", wsite_folder(post->site), tc(imgbase));
        str_destroy(&imgbase);
    }
    else
    {
        String *err = str_printf("[post]-Error processing image formula '%s'.", name);
        html5_status_err(status, &err);
        stm_writef(html, "<img src='UNKNOWN'/>\n");
    }

    str_destroy(&equation);
}

/*---------------------------------------------------------------------------*/

void post_footer(const Post *post, Stream *html)
{
    const String *footer;
    cassert_no_null(post);
    stm_writef(html, "<footer>\n");
    footer = wsite_get_footer(post->site);
    str_writef(html, footer);
    stm_writef(html, "</footer>\n");
}
