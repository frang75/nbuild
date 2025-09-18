/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: lnav.c
 *
 */

/* Html5 Laterial Nav */

#include "lnav.h"
#include "lnav.inl"
#include "css.h"
#include "wsite.inl"
#include <core/event.h>
#include <core/heap.h>
#include <core/stream.h>
#include <core/strings.h>
#include <sewer/cassert.h>
#include <sewer/unicode.h>

struct _lnav_t
{
    WSite *site;
    real32_t width;
    color_t bkcolor;
    color_t hbkcolor;
    color_t sbkcolor;
    color_t txcolor;
    color_t hvcolor;
    color_t secolor;
    real32_t fsize1;
    real32_t fsize2;
};

/*---------------------------------------------------------------------------*/

LNav *lnav_create(
    const real32_t width,
    const color_t bkcolor,
    const color_t hbkcolor,
    const color_t sbkcolor,
    const color_t txcolor,
    const color_t hvcolor,
    const color_t secolor,
    const real32_t fsize1,
    const real32_t fsize2)
{
    LNav *lnav = heap_new0(LNav);
    lnav->width = width;
    lnav->bkcolor = bkcolor;
    lnav->hbkcolor = hbkcolor;
    lnav->sbkcolor = sbkcolor;
    lnav->txcolor = txcolor;
    lnav->hvcolor = hvcolor;
    lnav->secolor = secolor;
    lnav->fsize1 = fsize1;
    lnav->fsize2 = fsize2;
    return lnav;
}

/*---------------------------------------------------------------------------*/

void lnav_destroy(LNav **lnav)
{
    heap_delete(lnav, LNav);
}

/*---------------------------------------------------------------------------*/

void lnav_site(LNav *lnav, WSite *site)
{
    cassert_no_null(lnav);
    cassert(lnav->site == NULL);
    lnav->site = site;
}

/*---------------------------------------------------------------------------*/

real32_t lnav_width(const LNav *lnav)
{
    cassert_no_null(lnav);
    return lnav->width;
}

/*---------------------------------------------------------------------------*/

void lnav_css(const LNav *lnav, CSS *css)
{
    real32_t w1, h1, h2;
    String *height;
    cassert_no_null(lnav);
    w1 = wsite_post_width(lnav->site);
    h1 = wsite_header_height(lnav->site);
    h2 = wsite_nav_height(lnav->site);
    height = str_printf("calc(100vh - %.0fpx)", h1 + h2);
    css_sel(css, ".lnav-div");
    css_str(css, "display", "block");
    css_str(css, "float", "left");
    css_str(css, "position", "relative");
    css_str(css, "box-sizing", "border-box");
    css_px(css, "width", lnav->width);
    css_str(css, "height", tc(height));
    css_str(css, "overflow-y", "auto");
    css_int(css, "z-index", 5);
    css_str(css, "box-shadow", "0 8px 10px -5px rgba(0,0,0,.2), 0 16px 24px 2px rgba(0,0,0,.14), 0 6px 30px 5px rgba(0,0,0,.12)");
    css_rgba(css, "background-color", lnav->bkcolor);
    css_sele(css);

    css_sel(css, ".lnav-sticky");
    css_str(css, "position", "fixed");
    css_px(css, "top", wsite_nav_height(lnav->site));
    /* css_str(css, "width", "100%"); */
    css_sele(css);

    css_media_max(css, w1 + lnav->width);
    css_sel(css, ".lnav-div");
    css_str(css, "display", "none");
    css_sele(css);
    css_mediae(css);

    css_sel(css, ".lnav a");
    css_rgba(css, "color", lnav->txcolor);
    css_str(css, "text-decoration", "none");
    css_sele(css);

    css_sel(css, ".lnav-l1");
    css_px4(css, "padding", 10.f, 5.f, 6.f, 15.f);
    css_px(css, "font-size", lnav->fsize1);
    css_sele(css);

    css_sel(css, ".lnav-l2");
    css_px4(css, "padding", 4.f, 5.f, 4.f, 25.f);
    css_px(css, "font-size", lnav->fsize2);
    css_sele(css);

    css_sel(css, ".lnav-hov:hover");
    css_rgba(css, "color", lnav->hvcolor);
    css_rgba(css, "background-color", lnav->hbkcolor);
    css_sele(css);

    css_sel(css, ".lnav-sel");
    css_rgba(css, "color", lnav->secolor);
    css_rgba(css, "background-color", lnav->sbkcolor);
    css_sele(css);

    css_sel(css, ".lnav-visible");
    css_str(css, "display", "block!important");
    css_str(css, "position", "fixed");
    css_sele(css);

    str_destroy(&height);
}

/*---------------------------------------------------------------------------*/

void lnav_js(const LNav *lnav, const jssec_t sec, Stream *js)
{
    cassert_no_null(lnav);
    switch (sec)
    {
    case ekJS_CODE:
        stm_writef(js, "\n");
        stm_writef(js, "var lnav = document.getElementsByClassName(\"lnav-div\")[0];\n");
        stm_writef(js, "function on_lnav_wscroll() {\n");
        stm_printf(js, "if (window.pageYOffset >= %.0f) {\n", wsite_header_height(lnav->site));
        stm_writef(js, "    lnav.classList.add(\"lnav-sticky\");\n");
        stm_printf(js, "    lnav.style.height = window.innerHeight - %.0f;\n", wsite_nav_height(lnav->site));
        stm_writef(js, "}\n");
        stm_writef(js, "else {\n");
        stm_writef(js, "    lnav.classList.remove(\"lnav-sticky\");\n");
        stm_printf(js, "    lnav.style.height = window.innerHeight - %.0f + window.pageYOffset;\n", wsite_nav_height(lnav->site) + wsite_header_height(lnav->site));
        stm_writef(js, "} }\n");
        stm_writef(js, "\n");
        stm_writef(js, "function on_lnav_load() {\n");
        stm_writef(js, "var lnavitem = document.getElementsByClassName('lnav-sel')[0];\n");
        stm_writef(js, "if (lnavitem != null)\n");
        stm_writef(js, "    lnavitem.scrollIntoView({behavior: 'instant', block: 'center'});\n");
        stm_writef(js, "}\n");
        break;

    case ekJS_ONSCROLL:
        stm_writef(js, "on_lnav_wscroll();\n");
        break;

    case ekJS_ONRESIZE:
        stm_writef(js, "on_lnav_wscroll();\n");
        break;

    case ekJS_ONLOAD:
        stm_writef(js, "on_lnav_load();\n");
        break;

    default:
        cassert_default(sec);
    }
}

/*---------------------------------------------------------------------------*/

void lnav_html(const LNav *lnav, Stream *html, Listener *listener)
{
    unref(lnav);
    stm_writef(html, "\n<div class='lnav-div'>\n");
    stm_writef(html, "<div class='lnav'>\n");
    stm_writef(html, "<ul>\n");
    listener_event(listener, ekLNAV_CONTENT, lnav, html, NULL, LNav, Stream, void);
    stm_writef(html, "</ul>\n");
    stm_writef(html, "</div> <!-- lnav -->\n");
    stm_writef(html, "</div> <!-- lnav-div -->\n");
}

/*---------------------------------------------------------------------------*/

static void i_write_l(LNav *lnav, const char_t *l, const char_t *text, const char_t *url, const bool_t selected, Stream *html)
{
    char_t cl[32];
    unref(lnav);
    str_copy_c(cl, 32, l);
    if (selected == TRUE)
    {
        if (url != NULL)
            str_cat_c(cl, 32, " lnav-hov lnav-sel");
        else
            str_cat_c(cl, 32, " lnav-sel");
    }
    else
    {
        if (url != NULL)
            str_cat_c(cl, 32, " lnav-hov");
    }

    if (url != NULL)
        stm_printf(html, "<a href='%s'>", url);
    else
        stm_writef(html, "<a>");

    stm_printf(html, "<li class='%s'>", cl);
    stm_writef(html, text);
    stm_writef(html, "</li>");
    stm_writef(html, "</a>\n");
}

/*---------------------------------------------------------------------------*/

void lnav_write_l1(LNav *lnav, const char_t *text, const char_t *url, const bool_t selected, Stream *html)
{
    i_write_l(lnav, "lnav-l1", text, url, selected, html);
}

/*---------------------------------------------------------------------------*/

void lnav_write_l2(LNav *lnav, const char_t *text, const char_t *url, const bool_t selected, Stream *html)
{
    i_write_l(lnav, "lnav-l2", text, url, selected, html);
}
