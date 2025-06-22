/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: css.c
 *
 */

/* Html5 CSS */

#include "css.inl"
#include "css.h"
#include <draw2d/color.h>
#include <core/heap.h>
#include <core/stream.h>
#include <sewer/cassert.h>

struct _css_t
{
    Stream *stm;
    bool_t block_open;
    bool_t media_open;
};

/*---------------------------------------------------------------------------*/

CSS *css_create(const char_t *pathname, ferror_t *error)
{
    Stream *stm = stm_to_file(pathname, error);
    if (stm != NULL)
    {
        CSS *css = heap_new0(CSS);
        css->stm = stm;
        return css;
    }
    return NULL;
}

/*---------------------------------------------------------------------------*/

void css_destroy(CSS **css)
{
    cassert_no_null(css);
    cassert_no_null(*css);
    cassert((*css)->media_open == FALSE);
    cassert((*css)->block_open == FALSE);
    stm_close(&(*css)->stm);
    heap_delete(css, CSS);
}

/*---------------------------------------------------------------------------*/

void css_comment(CSS *css, const char_t *comment)
{
    cassert_no_null(css);
    stm_writef(css->stm, "/* ");
    stm_writef(css->stm, comment);
    stm_writef(css->stm, " */\n");
}

/*---------------------------------------------------------------------------*/

void css_sel(CSS *css, const char_t *sel)
{
    cassert_no_null(css);
    cassert(css->block_open == FALSE);
    stm_writef(css->stm, sel);
    stm_writef(css->stm, "{");
    css->block_open = TRUE;
}

/*---------------------------------------------------------------------------*/

void css_sele(CSS *css)
{
    cassert_no_null(css);
    cassert(css->block_open == TRUE);
    stm_writef(css->stm, "}");
    if (css->media_open == FALSE)
        stm_writef(css->stm, "\n");
    css->block_open = FALSE;
}

/*---------------------------------------------------------------------------*/

void css_media_max(CSS *css, const real32_t width)
{
    cassert_no_null(css);
    cassert(css->media_open == FALSE);
    stm_printf(css->stm, "@media(max-width:%.0fpx){", width);
    css->media_open = TRUE;
}

/*---------------------------------------------------------------------------*/

void css_mediae(CSS *css)
{
    cassert_no_null(css);
    cassert(css->block_open == FALSE);
    cassert(css->media_open == TRUE);
    stm_writef(css->stm, "}\n");
    css->media_open = FALSE;
}

/*---------------------------------------------------------------------------*/

void css_px(CSS *css, const char_t *prop, const real32_t value)
{
    cassert_no_null(css);
    stm_writef(css->stm, prop);
    stm_writef(css->stm, ":");
    if (value == 0.f)
        stm_writef(css->stm, "0;");
    else
        stm_printf(css->stm, "%.0fpx;", value);
}

/*---------------------------------------------------------------------------*/

void css_px_important(CSS *css, const char_t *prop, const real32_t value)
{
    cassert_no_null(css);
    stm_writef(css->stm, prop);
    stm_writef(css->stm, ":");
    if (value == 0.f)
        stm_writef(css->stm, "0 !important;");
    else
        stm_printf(css->stm, "%.0fpx !important;", value);
}

/*---------------------------------------------------------------------------*/

void css_px2(CSS *css, const char_t *prop, const real32_t value1, const real32_t value2)
{
    cassert_no_null(css);
    stm_writef(css->stm, prop);
    stm_writef(css->stm, ":");
    if (value1 == 0.f)
        stm_writef(css->stm, "0 ");
    else
        stm_printf(css->stm, "%.0fpx ", value1);

    if (value2 == 0.f)
        stm_writef(css->stm, "0;");
    else
        stm_printf(css->stm, "%.0fpx;", value2);
}

/*---------------------------------------------------------------------------*/

void css_px4(CSS *css, const char_t *prop, const real32_t value1, const real32_t value2, const real32_t value3, const real32_t value4)
{
    cassert_no_null(css);
    stm_writef(css->stm, prop);
    stm_writef(css->stm, ":");
    if (value1 == 0.f)
        stm_writef(css->stm, "0 ");
    else
        stm_printf(css->stm, "%.0fpx ", value1);

    if (value2 == 0.f)
        stm_writef(css->stm, "0 ");
    else
        stm_printf(css->stm, "%.0fpx ", value2);

    if (value3 == 0.f)
        stm_writef(css->stm, "0 ");
    else
        stm_printf(css->stm, "%.0fpx ", value3);

    if (value4 == 0.f)
        stm_writef(css->stm, "0;");
    else
        stm_printf(css->stm, "%.0fpx;", value4);
}

/*---------------------------------------------------------------------------*/

void css_int(CSS *css, const char_t *prop, const int32_t value)
{
    cassert_no_null(css);
    stm_writef(css->stm, prop);
    stm_writef(css->stm, ":");
    stm_printf(css->stm, "%d;", value);
}

/*---------------------------------------------------------------------------*/

void css_real(CSS *css, const char_t *prop, const real32_t value)
{
    cassert_no_null(css);
    stm_writef(css->stm, prop);
    stm_writef(css->stm, ":");
    stm_printf(css->stm, "%.2f;", value);
}

/*---------------------------------------------------------------------------*/

void css_str(CSS *css, const char_t *prop, const char_t *value)
{
    cassert_no_null(css);
    stm_writef(css->stm, prop);
    stm_writef(css->stm, ":");
    stm_writef(css->stm, value);
    stm_writef(css->stm, ";");
}

/*---------------------------------------------------------------------------*/

void css_rgba(CSS *css, const char_t *prop, const color_t color)
{
    uint8_t r, g, b, a;
    cassert_no_null(css);
    stm_writef(css->stm, prop);
    stm_writef(css->stm, ":");
    color_get_rgba(color, &r, &g, &b, &a);
    if (a == 255)
        stm_printf(css->stm, "rgb(%d,%d,%d);", r, g, b);
    else
        stm_printf(css->stm, "rgba(%d,%d,%d,%.2f);", r, g, b, a / 255.f);
}

/*---------------------------------------------------------------------------*/

void css_rgba_important(CSS *css, const char_t *prop, const color_t color)
{
    uint8_t r, g, b, a;
    cassert_no_null(css);
    stm_writef(css->stm, prop);
    stm_writef(css->stm, ":");
    color_get_rgba(color, &r, &g, &b, &a);
    if (a == 255)
        stm_printf(css->stm, "rgb(%d,%d,%d) !important;", r, g, b);
    else
        stm_printf(css->stm, "rgba(%d,%d,%d,%.2f) !important;", r, g, b, a / 255.f);
}

/*---------------------------------------------------------------------------*/

void css_direct(CSS *css, const char_t *cssline)
{
    cassert_no_null(css);
    stm_writef(css->stm, cssline);
    stm_writef(css->stm, "\n");
}
