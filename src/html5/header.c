/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: header.c
 *
 */

/* Html5 Header */

#include "header.h"
#include "header.inl"
#include "wsite.inl"
#include "css.h"
#include <draw2d/image.h>
#include <core/arrpt.h>
#include <core/event.h>
#include <core/heap.h>
#include <core/stream.h>
#include <core/strings.h>
#include <sewer/cassert.h>
#include <sewer/unicode.h>

struct _header_t
{
    WSite *site;
    String *banner;
    String *text;
    String *home;
    color_t bkcolor;
    color_t txcolor;
    real32_t fsize;
    real32_t imgw;
    real32_t imgh;
    real32_t textw;
    real32_t height;
};

/*---------------------------------------------------------------------------*/

static const real32_t i_HOR_PADDING = 16.f;
static const real32_t i_VER_PADDING = 10.f;
static const real32_t i_LETTER_SPACING = 4.f;
static const real32_t i_GLYPH_MEDIUM = 0.55f;

/*---------------------------------------------------------------------------*/

Header *header_create(const char_t *banner, const color_t txcolor, const color_t bkcolor, const real32_t fsize)
{
    Header *header = heap_new0(Header);
    header->banner = str_c(banner);
    header->text = str_c("Site Header");
    header->home = str_c("");
    header->txcolor = txcolor;
    header->bkcolor = bkcolor;
    header->fsize = fsize;
    return header;
}

/*---------------------------------------------------------------------------*/

void header_destroy(Header **header)
{
    cassert_no_null(header);
    cassert_no_null(*header);
    str_destroy(&(*header)->banner);
    str_destroy(&(*header)->text);
    str_destroy(&(*header)->home);
    heap_delete(header, Header);
}

/*---------------------------------------------------------------------------*/

void header_text(Header *header, const char_t *text)
{
    cassert_no_null(header);
    str_upd(&header->text, text);
}

/*---------------------------------------------------------------------------*/

void header_home(Header *header, const char_t *url)
{
    cassert_no_null(header);
    str_upd(&header->home, url);
}

/*---------------------------------------------------------------------------*/

void header_site(Header *header, WSite *site, Html5Status *status)
{
    cassert_no_null(header);
    header->site = site;
    wsite_add_res(site, tc(header->banner), status);
}

/*---------------------------------------------------------------------------*/

real32_t header_height(const Header *header)
{
    cassert_no_null(header);
    return header->height;
}

/*---------------------------------------------------------------------------*/

static void i_params(Header *header, Html5Status *status)
{
    cassert_no_null(header);
    if (header->imgw < 1.f || header->imgh < 1.f)
    {
        Image *img = wsite_image(header->site, tc(header->banner), status);
        if (img != NULL)
        {
            header->imgw = (real32_t)image_width(img);
            header->imgh = (real32_t)image_height(img);
            image_destroy(&img);
        }
        else
        {
            header->imgw = 160.f;
            header->imgh = 40.f;
        }
    }

    {
        real32_t n = (real32_t)unicode_nchars(tc(header->text), ekUTF8);
        header->textw = (n * header->fsize * i_GLYPH_MEDIUM) + ((n - 1.f) * i_LETTER_SPACING);
    }

    header->height = header->imgh + i_VER_PADDING * 2.f;
}

/*---------------------------------------------------------------------------*/

void header_css(const Header *header, CSS *css, Html5Status *status)
{
    real32_t min = 0.f;
    cassert_no_null(header);
    i_params((Header *)header, status);

    css_sel(css, ".header-div");
    css_px(css, "height", header->height);
    css_rgba(css, "background-color", header->bkcolor);
    css_sele(css);

    css_sel(css, ".header");
    css_rgba(css, "background-color", header->bkcolor);
    css_px2(css, "padding", 0.f, i_HOR_PADDING);
    css_int(css, "z-index", 2);
    css_str(css, "box-sizing", "border-box");
    css_sele(css);

    css_sel(css, ".header-image");
    css_str(css, "float", "left");
    css_str(css, "position", "relative");
    css_px(css, "top", i_VER_PADDING);
    css_str(css, "box-sizing", "border-box");
    css_sele(css);

    css_sel(css, ".header-text");
    css_str(css, "display", "block");
    css_str(css, "float", "right");
    css_str(css, "text-align", "right");
    css_str(css, "position", "relative");
    css_px(css, "top", i_VER_PADDING + header->imgh - header->fsize - 16);
    css_px(css, "padding-right", i_HOR_PADDING);
    css_str(css, "box-sizing", "border-box");
    css_px(css, "font-size", header->fsize);
    css_px(css, "letter-spacing", i_LETTER_SPACING);
    css_rgba(css, "color", header->txcolor);
    css_sele(css);

    css_sel(css, ".header-text img");
    css_str(css, "vertical-align", "middle");
    css_px(css, "padding-left", 5.f);
    css_sele(css);

    min = header->imgw + header->textw + 3.f * i_HOR_PADDING;
    css_media_max(css, min);

    css_sel(css, ".header-string");
    css_str(css, "display", "none");
    css_sele(css);

    css_sel(css, ".header-text");
    css_str(css, "float", "left");
    css_str(css, "text-align", "left");
    css_px(css, "padding-top", 7.f);
    css_px(css, "padding-left", 10.f);
    css_sele(css);

    /*     css_sel(css, ".header-image");
    css_str(css, "display", "block");
    css_str(css, "float", "none");
    css_sele(css);

    css_sel(css, ".header-image img");
    css_str(css, "display", "block");
    css_str(css, "margin", "0 auto");
    css_sele(css);
 */
    css_mediae(css);
}

/*---------------------------------------------------------------------------*/

void header_js(const Header *header, const jssec_t sec, Stream *js)
{
    unref(header);
    unref(sec);
    unref(js);
}

/*---------------------------------------------------------------------------*/

void header_html(const Header *header, const ArrPt(String) *langs, Stream *html, Listener *listener)
{
    cassert_no_null(header);
    stm_writef(html, "<div class='header-div'>\n");
    stm_writef(html, "<div class='header'>\n");

    stm_writef(html, "\n<div class='header-image'>\n");
    stm_writef(html, "<a href='");
    str_writef(html, header->home);
    stm_writef(html, "'><img src='");
    wsite_stm_res(header->site, html);
    stm_writef(html, tc(header->banner));
    stm_writef(html, "' alt='");
    stm_writef(html, wsite_prjname(header->site));
    stm_writef(html, " logo'/></a>\n");
    stm_writef(html, "</div> <!-- header-image -->\n");

    stm_writef(html, "\n<div class='header-text'>\n");

    if (arrpt_size(langs, String) > 0)
    {
        arrpt_foreach_const(lang, langs, String)
            {
                stm_writef(html, "<a href='");
                listener_event(listener, ekPAGE_TRANSLATE, tc(lang), html, NULL, char_t, Stream, void);
                stm_writef(html, "'><img src='");
                wsite_stm_res(header->site, html);
                stm_writef(html, tc(lang));
                stm_writef(html, ".png'/></a>");
            }
        arrpt_end()

        stm_writef(html, "<br/>");
    }

    stm_writef(html, "<div class='header-string'>");
    stm_writef(html, tc(header->text));
    stm_writef(html, "</div>");
    stm_writef(html, "\n</div> <!-- header-text -->\n");

    stm_writef(html, "\n</div> <!-- header -->\n");
    stm_writef(html, "</div> <!-- header-div -->\n");
}
