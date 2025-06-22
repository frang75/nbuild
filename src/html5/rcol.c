/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: rcol.c
 *
 */

/* Html5 Right Column */

#include "rcol.h"
#include "rcol.inl"
#include "css.h"
#include "wsite.inl"
#include <core/heap.h>
#include <core/stream.h>
#include <core/strings.h>
#include <sewer/cassert.h>

struct _rcol_t
{
    WSite *site;
    real32_t width;
    color_t bkcolor;
    String *img;
    String *text;
    String *url;
};

static const real32_t i_VER_PADDING = 16.f;
static const real32_t i_HOR_PADDING = 10.f;

/*---------------------------------------------------------------------------*/

RCol *rcol_create(
    const real32_t width,
    const color_t bkcolor)
{
    RCol *rcol = heap_new0(RCol);
    rcol->width = width;
    rcol->bkcolor = bkcolor;
    return rcol;
}

/*---------------------------------------------------------------------------*/

void rcol_add_banner(RCol *rcol, const char_t *img, const char_t *text, const char_t *url, Html5Status *status)
{
    cassert_no_null(rcol);
    cassert(rcol->img == NULL);
    rcol->img = str_c(img);
    rcol->text = str_c(text);
    rcol->url = str_c(url);
    wsite_add_res(rcol->site, tc(rcol->img), status);
    /* wsite_add_res(rcol->site, tc(rcol->url)); */
}

/*---------------------------------------------------------------------------*/

void rcol_remove_banners(RCol *rcol)
{
    cassert_no_null(rcol);
    str_destopt(&rcol->img);
    str_destopt(&rcol->text);
    str_destopt(&rcol->url);
}

/*---------------------------------------------------------------------------*/

void rcol_destroy(RCol **rcol)
{
    cassert_no_null(rcol);
    cassert_no_null(*rcol);
    str_destopt(&(*rcol)->img);
    str_destopt(&(*rcol)->text);
    str_destopt(&(*rcol)->url);
    heap_delete(rcol, RCol);
}

/*---------------------------------------------------------------------------*/

void rcol_site(RCol *rcol, WSite *site)
{
    cassert_no_null(rcol);
    cassert(rcol->site == NULL);
    rcol->site = site;
}

/*---------------------------------------------------------------------------*/

real32_t rcol_width(const RCol *rcol)
{
    cassert_no_null(rcol);
    return rcol->width;
}

/*---------------------------------------------------------------------------*/

void rcol_css(const RCol *rcol, CSS *css)
{
    real32_t w1, w2, h1, h2;
    String *height;
    cassert_no_null(rcol);
    w1 = wsite_lnav_width(rcol->site);
    w2 = wsite_post_width(rcol->site);
    h1 = wsite_header_height(rcol->site);
    h2 = wsite_nav_height(rcol->site);
    height = str_printf("calc(100vh - %.0fpx)", h1 + h2);
    css_sel(css, ".rcol-div");
    css_str(css, "display", "block");
    css_str(css, "float", "left");
    css_str(css, "position", "relative");
    css_str(css, "box-sizing", "border-box");
    css_px4(css, "padding", i_VER_PADDING, i_HOR_PADDING, i_VER_PADDING, i_HOR_PADDING);
    css_px(css, "top", 0.f);
    css_px(css, "left", 0.f);
    css_px(css, "width", rcol->width);
    css_str(css, "height", tc(height));
    css_str(css, "overflow-y", "auto");
    css_str(css, "box-shadow", "0 8px 10px -5px rgba(0,0,0,.2), 0 16px 24px 2px rgba(0,0,0,.14), 0 6px 30px 5px rgba(0,0,0,.12)");
    css_rgba(css, "background-color", rcol->bkcolor);
    css_sele(css);

    css_sel(css, ".rcol-sticky");
    css_str(css, "position", "fixed");
    css_px(css, "top", wsite_nav_height(rcol->site));
    css_sele(css);

    css_sel(css, ".rcol");
    css_str(css, "width", "100%");
    css_str(css, "text-align", "center");
    css_sele(css);

    css_sel(css, ".rcol img");
    css_str(css, "max-width", "50%");
    css_sele(css);

    css_sel(css, ".rcol a");
    css_str(css, "color", "#999");
    css_str(css, "text-decoration", "none");
    css_sele(css);

    css_media_max(css, w1 + w2 + rcol->width);
    css_sel(css, ".rcol-div");
    css_str(css, "display", "none");
    css_sele(css);
    css_mediae(css);
    str_destroy(&height);
}

/*---------------------------------------------------------------------------*/

void rcol_js(const RCol *rcol, const jssec_t sec, Stream *js)
{
    cassert_no_null(rcol);
    switch (sec)
    {
    case ekJS_CODE:
        stm_writef(js, "\nvar rcol = document.getElementsByClassName(\"rcol-div\")[0];\n");
        stm_writef(js, "function on_rcol_wscroll() {\n");
        stm_printf(js, "if (window.pageYOffset >= %.0f) {\n", wsite_header_height(rcol->site));
        stm_writef(js, "    rcol.classList.add(\"rcol-sticky\");\n");
        stm_printf(js, "    rcol.style.height = window.innerHeight - %.0f;\n", wsite_nav_height(rcol->site));
        stm_writef(js, "    rcol.style.left = lnav.offsetWidth + post.offsetWidth;\n");
        stm_writef(js, "}\n");
        stm_writef(js, "else {\n");
        stm_writef(js, "    rcol.classList.remove(\"rcol-sticky\");\n");
        stm_printf(js, "    rcol.style.height = window.innerHeight - %.0f + window.pageYOffset;\n", wsite_nav_height(rcol->site) + wsite_header_height(rcol->site));
        stm_writef(js, "    rcol.style.left = 0;\n");
        stm_writef(js, "} }\n");
        break;

    case ekJS_ONSCROLL:
        stm_writef(js, "on_rcol_wscroll();\n");
        break;

    case ekJS_ONRESIZE:
        stm_writef(js, "on_rcol_wscroll();\n");
        break;

    case ekJS_ONLOAD:
        break;
        cassert_default();
    }
}

/*---------------------------------------------------------------------------*/

void rcol_html(const RCol *rcol, Stream *html, Listener *listener)
{
    cassert_no_null(rcol);
    unref(listener);
    stm_writef(html, "\n<div class='rcol-div'>\n");
    stm_writef(html, "<div class='rcol'>\n");
    if (rcol->url != NULL)
    {
        stm_writef(html, "<a href='");
        /* wsite_stm_res(rcol->site, html) */;
        stm_writef(html, tc(rcol->url));
        stm_writef(html, "'/>\n");
        stm_writef(html, "<img src='");
        wsite_stm_res(rcol->site, html);
        stm_writef(html, tc(rcol->img));
        stm_writef(html, "'/><br/>\n");
        stm_writef(html, tc(rcol->text));
        stm_writef(html, "</a></p>\n");
    }
    stm_writef(html, "</div> <!-- rcol -->\n");
    stm_writef(html, "</div> <!-- rcol-div -->\n");
}
