/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: nav.c
 *
 */

/* Html5 Nav */

#include "nav.h"
#include "nav.inl"
#include "css.h"
#include "html5.h"
#include "wsite.inl"
#include <core/arrst.h>
#include <core/heap.h>
#include <core/stream.h>
#include <core/strings.h>
#include <sewer/cassert.h>
#include <sewer/unicode.h>

typedef struct _mitem_t MItem;
struct _mitem_t
{
    String *text;
    String *url;
    String *hover;
};

struct _nav_t
{
    WSite *site;
    String *home_icon;
    String *menu_icon;
    color_t bkcolor;
    color_t txcolor;
    color_t hvcolor;
    color_t secolor;
    real32_t fsize;
    MItem home;
    ArrSt(MItem) *items;
};

DeclSt(MItem);

/*---------------------------------------------------------------------------*/

static const real32_t i_LINE_HEIGHT = 24.f;
static const real32_t i_TOP_PADDING = 10.f;
static const real32_t i_BOT_PADDING = 9.f;
static const real32_t i_HOR_PADDING = 15.f;
static const real32_t i_LETTER_SPACING = 1.f;
static const real32_t i_ICON_SIZE = 20;
static const real32_t i_OPACITY = .8f;
static const real32_t i_GLYPH_MEDIUM = 0.55f;

/*---------------------------------------------------------------------------*/

static void i_remove_item(MItem *item)
{
    cassert_no_null(item);
    str_destroy(&item->text);
    str_destroy(&item->url);
    str_destroy(&item->hover);
}

/*---------------------------------------------------------------------------*/

static void i_remove_item_opt(MItem *item)
{
    cassert_no_null(item);
    str_destopt(&item->text);
    str_destopt(&item->url);
}

/*---------------------------------------------------------------------------*/

Nav *nav_create(
    const char_t *home_icon,
    const char_t *menu_icon,
    const color_t bkcolor,
    const color_t txcolor,
    const color_t hvcolor,
    const color_t secolor,
    const real32_t fsize)
{
    Nav *nav = heap_new0(Nav);
    nav->home_icon = str_c(home_icon);
    nav->menu_icon = str_c(menu_icon);
    nav->bkcolor = bkcolor;
    nav->txcolor = txcolor;
    nav->hvcolor = hvcolor;
    nav->secolor = secolor;
    nav->fsize = fsize;
    nav->items = arrst_create(MItem);
    return nav;
}

/*---------------------------------------------------------------------------*/

void nav_destroy(Nav **nav)
{
    cassert_no_null(nav);
    cassert_no_null(*nav);
    str_destroy(&(*nav)->home_icon);
    str_destroy(&(*nav)->menu_icon);
    i_remove_item_opt(&(*nav)->home);
    arrst_destroy(&(*nav)->items, i_remove_item, MItem);
    heap_delete(nav, Nav);
}

/*---------------------------------------------------------------------------*/

void nav_clear_items(Nav *nav)
{
    cassert_no_null(nav);
    i_remove_item_opt(&nav->home);
    arrst_clear(nav->items, i_remove_item, MItem);
}

/*---------------------------------------------------------------------------*/

void nav_set_home(Nav *nav, const char_t *url)
{
    cassert(nav->home.text == NULL);
    cassert(nav->home.url == NULL);
    nav->home.text = str_c("");
    nav->home.url = str_c(url);
}

/*---------------------------------------------------------------------------*/

void nav_add_item(Nav *nav, const char_t *text, const char_t *url, const char_t *hover)
{
    MItem *item;
    cassert_no_null(nav);
    item = arrst_new(nav->items, MItem);
    item->text = str_c(text);
    item->url = str_c(url);
    item->hover = str_c(hover);
}

/*---------------------------------------------------------------------------*/

static void i_process_icon(const char_t *icon_name, const Nav *nav, WSite *site, Html5Status *status)
{
    String *basename;
    String *normal_icon;
    String *hover_icon;
    String *sel_icon;
    str_split_pathext(icon_name, NULL, &basename, NULL);
    normal_icon = str_printf("%s.png", tc(basename));
    hover_icon = str_printf("%s_hover.png", tc(basename));
    sel_icon = str_printf("%s_sel.png", tc(basename));
    wsite_add_icon(site, icon_name, tc(normal_icon), nav->txcolor, (uint32_t)i_ICON_SIZE, status);
    wsite_add_icon(site, icon_name, tc(hover_icon), nav->hvcolor, (uint32_t)i_ICON_SIZE, status);
    wsite_add_icon(site, icon_name, tc(sel_icon), nav->secolor, (uint32_t)i_ICON_SIZE, status);
    str_destroy(&basename);
    str_destroy(&normal_icon);
    str_destroy(&hover_icon);
    str_destroy(&sel_icon);
}

/*---------------------------------------------------------------------------*/

void nav_site(Nav *nav, WSite *site, Html5Status *status)
{
    cassert_no_null(nav);
    cassert(nav->site == NULL);
    nav->site = site;
    i_process_icon(tc(nav->home_icon), nav, site, status);
    i_process_icon(tc(nav->menu_icon), nav, site, status);
}

/*---------------------------------------------------------------------------*/

static real32_t i_item_width(const real32_t fsize, const char_t *text)
{
    real32_t n = (real32_t)unicode_nchars(text, ekUTF8) + 1;
    real32_t w = (n * fsize * i_GLYPH_MEDIUM) + ((n - 1.f) * i_LETTER_SPACING);
    w += 2.f * i_HOR_PADDING;
    return w;
}

/*---------------------------------------------------------------------------*/

real32_t nav_height(const Nav *nav)
{
    unref(nav);
    return i_LINE_HEIGHT + i_TOP_PADDING + i_BOT_PADDING;
}

/*---------------------------------------------------------------------------*/

void nav_css(const Nav *nav, CSS *css, Html5Status *status)
{
    String *home_icon;
    String *menu_icon;
    real32_t nav_size;
    real32_t sizes[32];
    uint32_t si = 0, i;

    cassert_no_null(nav);
    if (arrst_size(nav->items, MItem) > 32)
    {
        String *err = str_printf("[html5-nav]-More menu items");
        html5_status_err(status, &err);
        return;
    }

    str_split_pathext(tc(nav->home_icon), NULL, &home_icon, NULL);
    str_split_pathext(tc(nav->menu_icon), NULL, &menu_icon, NULL);

    css_sel(css, ".nav-div");
    css_int(css, "z-index", 10);
    css_sele(css);

    css_sel(css, ".nav-sticky");
    css_str(css, "position", "fixed");
    css_px(css, "top", 0.f);
    css_str(css, "width", "100%");
    css_sele(css);

    css_sel(css, ".navbar");
    css_str(css, "box-shadow", "0 3px 4px 0 rgba(0,0,0,.2), 0 3px 3px -2px rgba(0,0,0,.14), 0 1px 8px 0 rgba(0,0,0,.12)");
    css_str(css, "width", "auto");
    css_px(css, "height", nav_height(nav));
    css_rgba(css, "background-color", nav->bkcolor);
    css_rgba(css, "color", nav->txcolor);
    css_str(css, "position", "relative");
    css_real(css, "opacity", i_OPACITY);
    css_int(css, "z-index", 10);
    css_sele(css);

    css_sel(css, ".navbar a");
    css_str(css, "float", "left");
    css_str(css, "width", "auto");
    css_px4(css, "padding", i_TOP_PADDING, i_HOR_PADDING, i_BOT_PADDING, i_HOR_PADDING);
    css_rgba(css, "color", nav->txcolor);
    css_px(css, "font-size", nav->fsize);
    css_px(css, "letter-spacing", i_LETTER_SPACING);
    css_str(css, "text-decoration", "none");
    css_str(css, "vertical-align", "middle");
    css_px(css, "line-height", i_LINE_HEIGHT);
    css_str(css, "background-repeat", "no-repeat");
    css_str(css, "background-position", "center");
    css_sele(css);

    css_sel(css, ".navbar a:hover");
    css_rgba(css, "color", nav->hvcolor);
    css_str(css, "transition", "color 300ms linear");
    css_sele(css);

    css_sel(css, ".navbar-sel a");
    css_rgba(css, "color", nav->secolor);
    css_sele(css);

    css_sel(css, ".navbar-img");
    css_px(css, "width", i_ICON_SIZE);
    css_px(css, "height", i_ICON_SIZE);
    css_sele(css);

    if (nav->home.text != NULL)
    {
        {
            String *icon = str_printf("url(\"%s.png\")", tc(home_icon));
            css_sel(css, ".navbar-home a");
            css_str(css, "background-image", tc(icon));
            css_sele(css);
            str_destroy(&icon);
        }

        {
            String *icon = str_printf("url(\"%s_sel.png\")", tc(home_icon));
            css_sel(css, ".navbar-home-sel a");
            css_str(css, "background-image", tc(icon));
            css_sele(css);
            str_destroy(&icon);
        }

        {
            String *icon = str_printf("url(\"%s_hover.png\")", tc(home_icon));
            css_sel(css, ".navbar-home a:hover");
            css_str(css, "background-image", tc(icon));
            css_str(css, "transition", "color 300ms linear");
            css_sele(css);
            str_destroy(&icon);
        }
    }

    {
        String *icon = str_printf("url(\"%s.png\")", tc(menu_icon));
        css_sel(css, ".navbar-menu a");
        css_str(css, "background-image", tc(icon));
        css_sele(css);
        str_destroy(&icon);
    }

    {
        String *icon = str_printf("url(\"%s_hover.png\")", tc(menu_icon));
        css_sel(css, ".navbar-menu a:hover");
        css_str(css, "background-image", tc(icon));
        css_str(css, "transition", "color 300ms linear");
        css_sele(css);
        str_destroy(&icon);
    }

    {
        String *icon = str_printf("url(\"%s_sel.png\")", tc(menu_icon));
        css_sel(css, ".navbar-menu-sel a");
        css_str(css, "background-image", tc(icon));
        css_sele(css);
        str_destroy(&icon);
    }

    css_sel(css, ".navdrop");
    css_str(css, "float", "left");
    css_str(css, "overflow", "hidden");
    css_sele(css);

    css_sel(css, ".navdrop:hover .navdrop-panel");
    css_str(css, "display", "block");
    css_real(css, "opacity", .9f);
    css_str(css, "transition", "opacity 300ms linear");
    css_sele(css);

    css_sel(css, ".navdrop-panel");
    css_str(css, "display", "none");
    css_px(css, "margin-top", nav_height(nav));
    css_str(css, "box-shadow", "0 2px 5px 0 rgba(0,0,0,0.16),0 2px 10px 0 rgba(0,0,0,0.12)");
    css_str(css, "position", "absolute");
    css_rgba(css, "background-color", nav->bkcolor);
    css_px(css, "min-width", 160.f);
    css_real(css, "opacity", .0f);
    css_int(css, "z-index", 1);
    css_sele(css);

    css_sel(css, ".navdrop-panel a");
    css_str(css, "display", "block");
    css_str(css, "float", "none");
    css_px4(css, "padding", i_TOP_PADDING, i_HOR_PADDING, i_BOT_PADDING, i_HOR_PADDING);
    css_rgba(css, "color", nav->txcolor);
    css_px(css, "font-size", nav->fsize);
    css_px(css, "letter-spacing", i_LETTER_SPACING);
    css_str(css, "text-decoration", "none");
    css_str(css, "text-align", "left");
    css_sele(css);

    nav_size = i_ICON_SIZE + 2.f * i_HOR_PADDING;

    if (wsite_lnav_width(nav->site) > 0.f)
    {
        real32_t w1, w2;
        nav_size *= 2.f;
        css_sel(css, ".navbar-menu");
        css_str(css, "display", "none");
        css_sele(css);

        w1 = wsite_post_width(nav->site);
        w2 = wsite_lnav_width(nav->site);

        css_media_max(css, w1 + w2);
        css_sel(css, ".navbar-menu");
        css_str(css, "display", "block");
        css_sele(css);
        css_mediae(css);
    }

    css_sel(css, "#navbar-more");
    css_str(css, "display", "none");
    css_sele(css);

    if (str_equ_c(wsite_lang(nav->site), "es") == TRUE)
        nav_size += i_item_width(nav->fsize, "Más ▼");
    else
        nav_size += i_item_width(nav->fsize, "More ▼");
    nav_size += 10.f;

    arrst_foreach(item, nav->items, MItem)
        real32_t width;
        String *ditem;
        ditem = str_printf("#navdrop-item-%d", item_i);
        width = i_item_width(nav->fsize, tc(item->text));
        nav_size += width;
        css_sel(css, tc(ditem));
        css_str(css, "display", "none");
        css_sele(css);
        sizes[si++] = nav_size;
        str_destroy(&ditem);
    arrst_end()

    for (i = 0; i < si; ++i)
    {
        String *ditem = str_printf("#navdrop-item-%d", i);
        String *bitem = str_printf("#navbar-item-%d", i);
        css_media_max(css, sizes[i]);
        css_sel(css, tc(ditem));
        css_str(css, "display", "block");
        css_sele(css);

        css_sel(css, tc(bitem));
        css_str(css, "display", "none");
        css_sele(css);

        if (i == si - 1)
        {
            css_sel(css, "#navbar-more");
            css_str(css, "display", "block");
            css_sele(css);
        }

        css_mediae(css);
        str_destroy(&bitem);
        str_destroy(&ditem);
    }

    str_destroy(&home_icon);
    str_destroy(&menu_icon);
}

/*---------------------------------------------------------------------------*/

void nav_js(const Nav *nav, const jssec_t sec, Stream *js)
{
    cassert_no_null(nav);
    switch (sec)
    {
    case ekJS_CODE:
        stm_writef(js, "\nvar mitem = null;\n");
        stm_writef(js, "var lnav_show = false;\n");
        stm_writef(js, "function on_click_menu() {\n");
        stm_writef(js, "if (lnav_show == false) {\n");
        stm_writef(js, "mitem.classList.add(\"navbar-menu-sel\");\n");
        stm_writef(js, "mitem.classList.remove(\"navbar-menu\");\n");
        stm_writef(js, "lnav.classList.add(\"lnav-visible\");\n");
        stm_writef(js, "lnav_show = true;\n");
        stm_writef(js, "}\n");
        stm_writef(js, "else {\n");
        stm_writef(js, "mitem.classList.remove(\"navbar-menu-sel\");\n");
        stm_writef(js, "mitem.classList.add(\"navbar-menu\");\n");
        stm_writef(js, "lnav.classList.remove(\"lnav-visible\");\n");
        stm_writef(js, "lnav_show = false;\n");
        stm_writef(js, "}}\n");
        stm_writef(js, "\nvar nav = document.getElementsByClassName(\"nav-div\")[0];\n");
        stm_writef(js, "function on_nav_wscroll() {\n");
        stm_printf(js, "if (window.pageYOffset >= %.0f)\n", wsite_header_height(nav->site));
        stm_writef(js, "    nav.classList.add(\"nav-sticky\");\n");
        stm_writef(js, "else\n");
        stm_writef(js, "    nav.classList.remove(\"nav-sticky\");\n");
        stm_writef(js, "}\n");
        break;

    case ekJS_ONSCROLL:
        stm_writef(js, "on_nav_wscroll();\n");
        break;

    case ekJS_ONRESIZE:
        break;

    case ekJS_ONLOAD:
        stm_writef(js, "mitem = document.getElementById(\"mitem-hamburger\");\n");
        break;

    default:
        cassert_default(sec);
    }
}

/*---------------------------------------------------------------------------*/

void nav_html(const Nav *nav, const uint32_t menu_id, Stream *html)
{
    cassert_no_null(nav);

    stm_writef(html, "\n<div class='nav-div'>\n");
    stm_writef(html, "<div class='navbar'>\n");
    stm_writef(html, "<script></script>\n");

    /* Hamburguer Item */
    if (wsite_lnav_width(nav->site) > 0.f)
    {
        stm_writef(html, "\n<div class='navbar-menu' id='mitem-hamburger'>\n");
        stm_writef(html, "<a onclick='on_click_menu();'>\n");
        stm_writef(html, "<div class='navbar-img'>\n");
        stm_writef(html, "</div> <!-- navbar-img -->\n");
        stm_writef(html, "</a>\n");
        stm_writef(html, "</div> <!-- navbar-menu -->\n");
    }

    if (nav->home.text != NULL)
    {
        const char_t *divclass = menu_id == UINT32_MAX ? "navbar-home-sel" : "navbar-home";
        stm_printf(html, "\n<div class='%s'>\n", divclass);
        stm_printf(html, "<a href='%s'>\n", tc(nav->home.url));
        stm_writef(html, "<div class='navbar-img'>\n");
        stm_writef(html, "</div> <!-- navbar-img -->\n");
        stm_writef(html, "</a>\n");
        stm_printf(html, "</div> <!-- %s -->\n", divclass);
    }

    /* Menu items */
    arrst_foreach(item, nav->items, MItem)

        if (item_i == menu_id)
            stm_writef(html, "\n<div class='navbar-sel'>\n");
        else
            stm_writef(html, "\n<div>\n");

        stm_printf(html, "<a href='%s' title='%s' id='navbar-item-%d'>", tc(item->url), tc(item->hover), item_i);
        stm_writef(html, tc(item->text));
        stm_writef(html, "</a>\n");

        if (item_i == menu_id)
            stm_writef(html, "</div> <!-- navbar-sel -->\n");
        else
            stm_writef(html, "</div>\n");

    arrst_end()

    /* More... item */
    stm_writef(html, "\n<div class='navdrop'>\n");
    stm_writef(html, "<div>\n");
    stm_writef(html, "<a href='#' id='navbar-more'>");
    if (str_equ_c(wsite_lang(nav->site), "es") == TRUE)
        stm_writef(html, "Más ▼");
    else
        stm_writef(html, "More ▼");
    stm_writef(html, "</a>\n");
    stm_writef(html, "</div>\n");

    /* More... panel */
    stm_writef(html, "\n<div class='navdrop-panel'>\n");

    arrst_foreach(item, nav->items, MItem)

        if (item_i == menu_id)
            stm_writef(html, "\n<div class='navbar-sel'>\n");
        else
            stm_writef(html, "\n<div>\n");

        stm_printf(html, "<a href='%s' title='%s' id='navdrop-item-%d'>", tc(item->url), tc(item->hover), item_i);
        stm_writef(html, tc(item->text));
        stm_writef(html, "</a>\n");

        if (item_i == menu_id)
            stm_writef(html, "</div> <!-- navbar-sel -->\n");
        else
            stm_writef(html, "</div>\n");

    arrst_end()

    stm_writef(html, "\n</div> <!-- navdrop-panel -->\n");
    stm_writef(html, "</div> <!-- navdrop -->\n");
    stm_writef(html, "</div> <!-- navbar -->\n");
    stm_writef(html, "</div> <!-- nav-div -->\n");
}
