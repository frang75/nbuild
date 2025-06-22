/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: web.c
 *
 */

/* Web generator */

#include "web.h"
#include "loader.h"
#include "doc.h"
#include "nlog.h"
#include "res_ndoc.h"
#include <html5/css.h>
#include <html5/head.h>
#include <html5/header.h>
#include <html5/html5.h>
#include <html5/lnav.h>
#include <html5/nav.h>
#include <html5/post.h>
#include <html5/rcol.h>
#include <html5/scode.h>
#include <html5/wsite.h>
#include <draw2d/color.h>
#include <core/arrpt.h>
#include <core/arrst.h>
#include <core/hfile.h>
#include <core/respack.h>
#include <core/stream.h>
#include <core/strings.h>
#include <osbs/bfile.h>
#include <sewer/cassert.h>
#include <sewer/unicode.h>

/*---------------------------------------------------------------------------*/

static ___INLINE const char_t *i_lang(const Config *config, const uint32_t lang_id)
{
    const Lang *lang = NULL;
    cassert_no_null(config);
    lang = arrst_get(config->langs, lang_id, Lang);
    return tc(lang->lang);
}

/*---------------------------------------------------------------------------*/

static void i_web_warn(String **str, DocParser *parser)
{
    String *msg = NULL;
    cassert_no_null(str);
    cassert_no_null(parser);
    msg = str_printf("[web:%s:%s-%d]-%s.", i_lang(parser->config, parser->lang_id), doc_name(parser->doc), parser->line, tc(*str));
    nlog_warn(&msg);
    str_destroy(str);
}

/*---------------------------------------------------------------------------*/

static void i_html_errors(Html5Status *status, DocParser *parser)
{
    String *prefix = NULL;

    if (parser != NULL)
        prefix = str_printf("[web:%s:%s-%d]", i_lang(parser->config, parser->lang_id), doc_name(parser->doc), parser->line);
    else
        prefix = str_printf("[web]");

    nlog_html5(status, tc(prefix));
    str_destroy(&prefix);
}

/*---------------------------------------------------------------------------*/

WSite *web_start(const char_t *docpath, const char_t *webpath, const char_t *tmppath, const Config *config)
{
    String *url = NULL;
    String *respath = NULL;
    WSite *site = NULL;
    Head *head = NULL;
    Header *header = NULL;
    Nav *nav = NULL;
    LNav *lnav = NULL;
    Post *post = NULL;
    RCol *rcol = NULL;
    SCode *code = NULL;
    Html5Status *status = NULL;
    cassert_no_null(docpath);
    cassert_no_null(webpath);
    cassert_no_null(tmppath);
    cassert_no_null(config);
    url = str_printf("http://www.%s", tc(config->project_url));
    respath = str_cpath("%s/config", docpath);
    status = html5_status_create();
    site = wsite_create(tc(url), tc(respath), webpath, tmppath, color_html(tc(config->web_colback_color)), status);
    wsite_site_font(site, tc(config->web_site_font), status);
    wsite_head_font(site, tc(config->web_head_font), status);
    wsite_mono_font(site, tc(config->web_mono_font), status);

    arrst_foreach(lang, config->langs, Lang)
        wsite_add_lang(site, tc(lang->lang), status);
    arrst_end()

    head = head_create("favicon.png", tc(config->web_analytics));
    header = header_create("header.png", color_html(tc(config->web_sec_color)), color_html(tc(config->web_back_color)), 16.f);
    nav = nav_create("home.svg", "menu.svg", color_html(tc(config->web_navback_color)), color_html(tc(config->web_navtext_color)), color_html(tc(config->web_navhover_color)), color_html(tc(config->web_sec_color)), 14.f);
    lnav = lnav_create(config->web_lnav_width, color_html(tc(config->web_colback_color)), color_html(tc(config->web_over_color)), color_html(tc(config->web_sec_color)), color_html(tc(config->web_text_color)), color_html(tc(config->web_navtext_color)), color_html(tc(config->web_navtext_color)), 20.f, 16.f);
    post = post_create(config->web_post_width, color_html(tc(config->web_back_color)), color_html(tc(config->web_text_color)), color_html(tc(config->web_colback_color)), color_html(tc(config->web_title_color)), color_html(tc(config->web_sec_color)), color_html(tc(config->web_secback_color)), color_html(tc(config->web_sec_color)), color_html(tc(config->web_ter_color)), 26.f, 17.f, 32.f, 24.f, 22.f);
    rcol = rcol_create(config->web_rcol_width, color_html(tc(config->web_colback_color)));
    code = scode_create(color_html(tc(config->web_colback_color)), color_html(tc(config->web_text_color)), color_html(tc(config->web_ter_color)), color_html(tc(config->web_funcs_color)), color_html(tc(config->web_types_color)), color_html(tc(config->web_constants_color)));
    wsite_head(site, head, status);
    wsite_header(site, header, status);
    wsite_nav(site, nav, status);
    wsite_lnav(site, lnav);
    wsite_post(site, post);
    wsite_rcol(site, rcol);
    wsite_scode(site, code);
    str_destroy(&url);
    str_destroy(&respath);
    i_html_errors(status, NULL);
    html5_status_destroy(&status);
    return site;
}

/*---------------------------------------------------------------------------*/

void web_css(CSS *css, const Config *config)
{
    cassert_no_null(config);

    css_sel(css, ".divtable");
    css_str(css, "text-align", "center");
    css_str(css, "overflow-x", "auto");
    css_sele(css);

    css_sel(css, ".vtable");
    css_str(css, "text-align", "center");
    css_str(css, "border-collapse", "collapse");
    css_rgba(css, "color", color_html(tc(config->web_text_color)));
    css_str(css, "margin", "0 auto");
    css_sele(css);

    css_sel(css, ".vtable tbody td");
    css_str(css, "border", "1px solid #808080");
    css_str(css, "padding", "5px");
    css_sele(css);

    css_sel(css, ".vtable tbody td a");
    css_rgba(css, "color", color_html(tc(config->web_sec_color)));
    css_str(css, "text-decoration", "none");
    css_sele(css);

    css_sel(css, ".vtable tbody td a:hover");
    css_str(css, "text-decoration", "underline");
    css_sele(css);

    css_sel(css, ".vhead");
    css_rgba(css, "background-color", color_html(tc(config->web_colback_color)));
    css_str(css, "font-weight", "bold");
    css_str(css, "text-align", "center");
    css_sele(css);

    css_sel(css, ".ftable");
    css_str(css, "display", "block");
    css_str(css, "max-width", "100%");
    css_str(css, "width", "auto");
    css_str(css, "border-collapse", "separate");
    css_str(css, "border-spacing", "0 0");
    css_str(css, "font-family", "MonoFont,monospace");
    css_px4(css, "margin", 15.f, 0.f, 15.f, 0.f);
    css_px(css, "font-size", 18.f);
    css_sele(css);

    css_sel(css, ".ftable tbody tr:first-child td");
    css_px(css, "padding-top", 0.f);
    css_sele(css);

    css_sel(css, ".ftable tbody tr:last-child td");
    css_px(css, "padding-bottom", 0.f);
    css_sele(css);

    css_sel(css, ".ftable tbody td");
    css_px4(css, "padding", 7.f, 0.f, 7.f, 15.f);
    css_sele(css);

    css_sel(css, "td.fret");
    css_px(css, "width", 150.f);
    css_str(css, "text-align", "right");
    css_rgba(css, "color", color_rgb(64, 64, 64));
    css_sele(css);

    css_sel(css, "td.fret a");
    css_rgba(css, "color", color_html(tc(config->web_over_color)));
    css_str(css, "text-decoration", "none");
    css_sele(css);

    css_sel(css, "td.fret a:hover");
    css_str(css, "text-decoration", "underline");

    /* css_str(css, "font-weight", "bold"); */
    css_sele(css);

    css_sel(css, "td.fname a");
    css_rgba(css, "color", color_html(tc(config->web_over_color)));
    css_str(css, "text-decoration", "none");
    css_sele(css);

    css_sel(css, "td.fname a:hover");
    css_str(css, "text-decoration", "underline");

    /* css_str(css, "font-weight", "bold"); */
    css_sele(css);

    css_sel(css, "td.fname p");
    css_px(css, "text-indent", 0.f);
    css_sele(css);

    css_sel(css, "td.fenum");
    css_px(css, "width", 150.f);
    css_str(css, "text-align", "left");
    css_sele(css);

    css_sel(css, "td.fdesc a");
    css_rgba(css, "color", color_html(tc(config->web_sec_color)));
    css_sele(css);

    css_sel(css, "td.fdesc a:hover");
    css_str(css, "text-decoration", "underline");
    css_sele(css);

    css_sel(css, "td.fdesc p");
    css_px(css, "text-indent", 0.f);
    css_px(css, "margin", 0.f);
    css_rgba(css, "color", color_html(tc(config->web_text_color)));
    css_sele(css);

    css_sel(css, "pre.fdesc");
    css_px4(css, "margin", 15.f, 0.f, 15.f, 0.f);
    css_px(css, "font-size", 16.f);
    css_px(css, "border-width", 1.f);
    css_str(css, "line-height", "normal");
    css_str(css, "border-style", "solid");
    css_rgba(css, "color", color_rgb(64, 64, 64));
    css_rgba(css, "border-color", color_html(tc(config->web_ter_color)));
    css_sele(css);

    css_media_max(css, 550.f);
    css_sel(css, "td.fret");
    css_str(css, "width", "auto");
    css_sele(css);
    css_sel(css, "td.fenum");
    css_str(css, "width", "auto");
    css_sele(css);
    css_mediae(css);

    css_media_max(css, 400.f);
    css_sel(css, "td.fdesc p");
    css_str(css, "text-align", "left");
    css_sele(css);
    css_mediae(css);

    /* css_sel(css, "pre.fdesc a");
    css_rgba(css, "color", color_html(tc(config->web_over_color)));
    css_str(css, "text-decoration", "none");
    css_sele(css);

    css_sel(css, "pre.fdesc a:hover");
    css_str(css, "font-weight", "bold");
    css_sele(css); */

    css_sel(css, "p.noindent");
    css_px(css, "text-indent", 0.f);
    css_str(css, "line-height", "normal");
    css_rgba(css, "color", color_html(tc(config->web_ter_color)));
    css_px(css, "margin-bottom", 5.f);
    css_sele(css);

    css_sel(css, ".ftitle");
    css_str(css, "font-family", "SiteFont !important");
    css_sele(css);
}

/*---------------------------------------------------------------------------*/

static void i_hwrite(Stream *html, const char_t *text)
{
    uint32_t code = unicode_to_u32(text, ekUTF8);
    while (code != 0)
    {
        switch (code)
        {
        case '<':
            stm_writef(html, "&lt;");
            break;
        case '>':
            stm_writef(html, "&gt;");
            break;
        default:
            stm_write_char(html, code);
        }

        text = unicode_next(text, ekUTF8);
        code = unicode_to_u32(text, ekUTF8);
    }
}

/*---------------------------------------------------------------------------*/

static void i_text(Stream *html, const ArrSt(Tag) *tags, DocParser *parser)
{
    arrst_foreach_const(tag, tags, Tag)
        switch (tag->type)
        {
        case ekPLAINTEXT:
        case ekLFUNC_OPEN:
        case ekLTYPE_OPEN:
        case ekLHEAD_OPEN:
            str_writef(html, tag->text);
            break;

        case ekLPAGE_OPEN:
        {
            const Doc *doc = loader_doc(parser->loader, tc(tag->text));
            if (doc != NULL)
            {
                const Block *h1 = doc_h1(doc);
                i_text(html, h1->tags, parser);
            }
            break;
        }

        case ekLESS:
        case ekGREATER:
        case ekBOLD_OPEN:
        case ekBOLD_CLOSE:
        case ekITALIC_OPEN:
        case ekITALIC_CLOSE:
        case ekCODE_OPEN:
        case ekCODE_CLOSE:
        case ekSUB_OPEN:
        case ekSUB_CLOSE:
        case ekSUP_OPEN:
        case ekSUP_CLOSE:
        case ekUNDER_OPEN:
        case ekUNDER_CLOSE:
        case ekSTRIKE_OPEN:
        case ekSTRIKE_CLOSE:
        case ekREF_OPEN:
        case ekREF_CLOSE:
        case ekMATH_OPEN:
        case ekMATH_CLOSE:
        case ekLINK_OPEN:
        case ekLINK_CLOSE:
        case ekLFUNC_CLOSE:
        case ekLTYPE_CLOSE:
        case ekLPAGE_CLOSE:
        case ekLHEAD_CLOSE:
        default:
            break;
        }
    arrst_end()
}

/*---------------------------------------------------------------------------*/

static bool_t i_with_text(const ArrSt(Tag) *tags)
{
    if (tags == NULL)
        return FALSE;

    arrst_foreach_const(tag, tags, Tag)
        if (tag->type == ekPLAINTEXT)
        {
            if (str_len(tag->text) > 0)
                return TRUE;
        }
    arrst_end()
    return FALSE;
}

/*---------------------------------------------------------------------------*/

static String *i_plain_str(const ArrSt(Tag) *tags)
{
    String *str = str_c("");
    arrst_foreach_const(tag, tags, Tag)
        if (tag->type == ekPLAINTEXT)
            str_cat(&str, tc(tag->text));
    arrst_end()
    return str;
}

/*---------------------------------------------------------------------------*/

static void i_html_tags(Stream *html, const ArrSt(Tag) *tags, DocParser *parser)
{
    bool_t inmath = FALSE;
    cassert_no_null(parser);

    arrst_foreach_const(tag, tags, Tag)
        switch (tag->type)
        {
        case ekPLAINTEXT:
            if (inmath == TRUE)
                post_math_inline(parser->post, tag->text, html, parser->status);
            else
                stm_writef(html, tc(tag->text));
            break;

        case ekLESS:
            stm_writef(html, "&lt;");
            break;

        case ekGREATER:
            stm_writef(html, "&gt;");
            break;

        case ekBOLD_OPEN:
            stm_writef(html, "<b>");
            break;

        case ekBOLD_CLOSE:
            stm_writef(html, "</b>");
            break;

        case ekITALIC_OPEN:
            stm_writef(html, "<i>");
            break;

        case ekITALIC_CLOSE:
            stm_writef(html, "</i>");
            break;

        case ekCODE_OPEN:
            stm_writef(html, "<code>");
            break;

        case ekCODE_CLOSE:
            stm_writef(html, "</code>");
            break;

        case ekUNDER_OPEN:
            stm_writef(html, "<u>");
            break;

        case ekUNDER_CLOSE:
            stm_writef(html, "</u>");
            break;

        case ekSTRIKE_OPEN:
            stm_writef(html, "<del>");
            break;

        case ekSTRIKE_CLOSE:
            stm_writef(html, "</del>");
            break;

        case ekMATH_OPEN:
            inmath = TRUE;
            break;

        case ekMATH_CLOSE:
            inmath = FALSE;
            break;

        case ekSUB_OPEN:
        case ekSUB_CLOSE:
        case ekSUP_OPEN:
        case ekSUP_CLOSE:
            cassert(FALSE);
            break;

        case ekREF_OPEN:
        {
            btype_t reftype;
            const char_t *refid = doc_refid(parser->doc, tc(tag->text), &reftype);
            stm_writef(html, " (");
            if (refid != NULL)
            {
                if (reftype == ekIMG || reftype == ekIMG2)
                {
                    post_img_ref(parser->post, refid, html);
                }
                else if (reftype == ekMATH)
                {
                    post_img_math_ref(parser->post, refid, html);
                }
                else if (reftype == ekCODE)
                {
                    scode_ref(parser->scode, refid, html);
                }
                else if (reftype == ekTABLE)
                {
                    stm_writef(html, "<span class='post-dec'>");
                    stm_printf(html, "%s %s", respack_text(parser->respack, TEXT_20), refid);
                    stm_writef(html, "</span>");
                }
            }
            else
            {
                String *warn = str_printf("Reference '%s::%s' not found", doc_name(parser->doc), tc(tag->text));
                post_img_ref(parser->post, "XX", html);
                i_web_warn(&warn, parser);
            }
            stm_writef(html, ")");
            break;
        }

        case ekREF_CLOSE:
            break;

        case ekLINK_OPEN:
            post_a(parser->post, tc(tag->text), html);
            break;

        case ekLINK_CLOSE:
            post_a_end(parser->post, html);
            break;

        case ekLHEAD_OPEN:
        {
            const Doc *hddoc;
            const Block *hdblock;
            hddoc = loader_section(parser->loader, tc(tag->text), parser->doc, &hdblock);
            if (hddoc != NULL)
            {
                String *title = str_printf("%s::%s", doc_name(hddoc), tc(tag->text));

                if (str_equ_c(doc_packname(hddoc), doc_packname(parser->doc)) == TRUE)
                    stm_printf(html, "<a href='%s.html#%s' title='%s'>", doc_name(hddoc), tc(hdblock->ref), tc(title));
                else
                    stm_printf(html, "<a href='../%s/%s.html#%s' title='%s'>", doc_packname(hddoc), doc_name(hddoc), tc(hdblock->ref), tc(title));

                str_destroy(&title);
            }
            else
            {
                String *warn = str_printf("Referenced section '%s' not found", tc(tag->text));
                i_web_warn(&warn, parser);
                stm_writef(html, "<a>");
            }

            break;
        }

        case ekLPAGE_OPEN:
        {
            const Doc *doc = loader_doc(parser->loader, tc(tag->text));
            if (doc != NULL)
            {
                const Block *h1 = doc_h1(doc);
                if (str_equ_c(doc_packname(doc), doc_packname(parser->doc)) == TRUE)
                    stm_printf(html, "<a href='%s.html'>", doc_name(doc));
                else
                    stm_printf(html, "<a href='../%s/%s.html'>", doc_packname(doc), doc_name(doc));

                if (h1->type == ekH1)
                    i_html_tags(html, h1->tags, parser);
            }
            else
            {
                String *warn = str_printf("Referenced document '%s' not found", tc(tag->text));
                i_web_warn(&warn, parser);
                stm_printf(html, "<a>%s</a>", tc(tag->text));
            }
            break;
        }

        case ekLHEAD_CLOSE:
        case ekLPAGE_CLOSE:
            stm_writef(html, "</a>");
            break;

        case ekLFUNC_OPEN:
        {
            const Block *block;
            const Doc *doc = loader_func(parser->loader, tc(tag->text), &block);
            if (doc != NULL)
            {
                if (str_equ_c(doc_packname(doc), doc_packname(parser->doc)) == TRUE)
                    stm_printf(html, "<a href='%s.html#", doc_name(doc));
                else
                    stm_printf(html, "<a href='../%s/%s.html#", doc_packname(doc), doc_name(doc));

                stm_writef(html, tc(block->ref));
                stm_writef(html, "' title='");
                i_text(html, block->tags, parser);
                stm_writef(html, "'>");
                stm_writef(html, tc(tag->text));
                stm_writef(html, "</a>");
            }
            else
            {
                String *warn = str_printf("Referenced function '%s' not found", tc(tag->text));
                i_web_warn(&warn, parser);
                stm_writef(html, "<a>");
                stm_writef(html, tc(tag->text));
                stm_writef(html, "</a>");
            }
            break;
        }

        case ekLFUNC_CLOSE:
            break;

        case ekLTYPE_OPEN:
        {
            const Block *block;
            const Doc *doc = loader_type(parser->loader, tc(tag->text), &block);
            if (doc != NULL)
            {
                if (str_equ_c(doc_packname(doc), doc_packname(parser->doc)) == TRUE)
                    stm_printf(html, "<a href='%s.html#", doc_name(doc));
                else
                    stm_printf(html, "<a href='../%s/%s.html#", doc_packname(doc), doc_name(doc));

                stm_writef(html, tc(block->ref));
                stm_writef(html, "' title='");
                i_text(html, block->tags, parser);
                stm_writef(html, "'>");
                stm_writef(html, tc(tag->text));
                stm_writef(html, "</a>");
            }
            else
            {
                String *warn = str_printf("Referenced type '%s' not found", tc(tag->text));
                i_web_warn(&warn, parser);
                stm_writef(html, "<a>");
                stm_writef(html, tc(tag->text));
                stm_writef(html, "</a>");
            }
            break;
        }

        case ekLTYPE_CLOSE:
            break;

            cassert_default();
        }

    arrst_end()

    i_html_errors(parser->status, parser);
}

/*---------------------------------------------------------------------------*/

void web_part(Stream *html, const char_t *title, const char_t *packname, const Doc *doc, const Doc *packdoc, LNav *lnav)
{
    unref(packname);
    unref(doc);
    unref(packdoc);
    /* Web part title is not clickable */
    lnav_write_l1(lnav, title, NULL, FALSE, html);

    /*
    const char_t *docpack = doc_packname(doc);
    if (str_equ_c(packname, docpack) == TRUE)
    {
        lnav_write_l1(lnav, title, NULL, FALSE, html);
    }
    else
    {
        String *url = str_printf("../%s/%s.html", packname, doc_name(packdoc));
        lnav_write_l1(lnav, title, tc(url), FALSE, html);
        str_destroy(&url);
    }
    */
}

/*---------------------------------------------------------------------------*/

void web_doc_lnav(Stream *html, const Doc *curdoc, const Doc *doc, LNav *lnav)
{
    const char_t *cpack = doc_packname(curdoc);
    const char_t *pack = doc_packname(doc);
    String *url = NULL;
    bool_t selected = curdoc == doc ? TRUE : FALSE;
    const Block *h1 = doc_h1(doc);
    String *text = i_plain_str(h1->tags);

    if (str_equ_c(cpack, pack) == TRUE)
        url = str_printf("%s.html", doc_name(doc));
    else
        url = str_printf("../%s/%s.html", pack, doc_name(doc));

    lnav_write_l2(lnav, tc(text), tc(url), selected, html);
    str_destroy(&text);
    str_destopt(&url);
}

/*---------------------------------------------------------------------------*/

void web_nav_buttons(Stream *html, DocParser *parser)
{
    const Doc *prevdoc;
    const Doc *nextdoc;
    String *prev_url = NULL;
    String *prev_title = NULL;
    String *next_url = NULL;
    String *next_title = NULL;
    const char_t *prev_url_c = NULL;
    const char_t *prev_title_c = NULL;
    const char_t *next_url_c = NULL;
    const char_t *next_title_c = NULL;
    cassert_no_null(parser);
    prevdoc = loader_prev_doc(parser->loader, parser->doc);
    nextdoc = loader_next_doc(parser->loader, parser->doc);

    if (prevdoc != NULL)
    {
        const Block *h1 = doc_h1(prevdoc);

        if (str_equ_c(doc_packname(parser->doc), doc_packname(prevdoc)) == TRUE)
            prev_url = str_printf("%s.html", doc_name(prevdoc));
        else
            prev_url = str_printf("../%s/%s.html", doc_packname(prevdoc), doc_name(prevdoc));

        prev_title = i_plain_str(h1->tags);
        prev_url_c = tc(prev_url);
        prev_title_c = tc(prev_title);
    }

    if (nextdoc != NULL)
    {
        const Block *h1 = doc_h1(nextdoc);

        if (str_equ_c(doc_packname(parser->doc), doc_packname(nextdoc)) == TRUE)
            next_url = str_printf("%s.html", doc_name(nextdoc));
        else
            next_url = str_printf("../%s/%s.html", doc_packname(nextdoc), doc_name(nextdoc));

        next_title = i_plain_str(h1->tags);
        next_url_c = tc(next_url);
        next_title_c = tc(next_title);
    }

    post_nav_buttons(parser->post, prev_url_c, prev_title_c, next_url_c, next_title_c, html);

    str_destopt(&prev_url);
    str_destopt(&prev_title);
    str_destopt(&next_url);
    str_destopt(&next_title);
}

/*---------------------------------------------------------------------------*/

void web_tags(Stream *html, const Block *block, DocParser *parser)
{
    cassert_no_null(block);
    i_text(html, block->tags, parser);
}

/*---------------------------------------------------------------------------*/

void web_epig(Stream *html, const Block *block, DocParser *parser)
{
    cassert_no_null(block);
    cassert(block->type == ekEPIG);
    cassert_no_null(parser);
    post_epig(parser->post, html);
    i_html_tags(html, block->tags, parser);
    if (block->alt1 != NULL)
    {
        stm_writef(html, " <u>");
        i_html_tags(html, block->alt1, parser);
        stm_writef(html, "</u>");
    }
    post_epig_end(parser->post, html);
}

/*---------------------------------------------------------------------------*/

void web_meta_desc(Stream *html, const Block *block, DocParser *parser)
{
    cassert_no_null(block);
    if (block->type == ekDESC)
    {
        stm_writef(html, "<meta name='description' content='");
        i_text(html, block->tags, parser);
        stm_writef(html, "'>\n");
    }
}

/*---------------------------------------------------------------------------*/

void web_h1(Stream *html, const Block *block, DocParser *parser)
{
    cassert_no_null(block);
    cassert(block->type == ekH1);
    i_text(html, block->tags, parser);
}

/*---------------------------------------------------------------------------*/

void web_h2_plain(Stream *html, const Block *block, DocParser *parser)
{
    cassert_no_null(block);
    cassert(block->type == ekH2);
    cassert_no_null(parser);
    i_html_tags(html, block->tags, parser);
}

/*---------------------------------------------------------------------------*/

void web_h2(Stream *html, const Block *block, DocParser *parser)
{
    cassert_no_null(block);
    cassert(block->type == ekH2);
    cassert_no_null(parser);
    post_line(parser->post, html);
    post_h2(parser->post, tc(block->ref), NULL, doc_secnums(parser->doc), html);
    i_html_tags(html, block->tags, parser);
    post_h2_end(parser->post, html);
}

/*---------------------------------------------------------------------------*/

void web_h3_plain(Stream *html, const Block *block, DocParser *parser)
{
    cassert_no_null(block);
    cassert(block->type == ekH3);
    cassert_no_null(parser);
    i_html_tags(html, block->tags, parser);
}

/*---------------------------------------------------------------------------*/

void web_h3(Stream *html, const Block *block, DocParser *parser)
{
    cassert_no_null(block);
    cassert(block->type == ekH3);
    cassert_no_null(parser);
    post_h3(parser->post, tc(block->ref), NULL, doc_secnums(parser->doc), html);
    i_html_tags(html, block->tags, parser);
    post_h3_end(parser->post, html);
}

/*---------------------------------------------------------------------------*/

void web_parag(Stream *html, const Block *block, DocParser *parser)
{
    const char_t *param = doc_block_param(block, 0);
    bool_t indent = TRUE;
    cassert_no_null(block);
    cassert(block->type == ekPARAG);
    cassert_no_null(parser);

    if (param && str_equ_c(param, "noindent"))
        indent = FALSE;

    post_p(parser->post, indent, html);
    i_html_tags(html, block->tags, parser);
    post_p_end(parser->post, html);
}

/*---------------------------------------------------------------------------*/

void web_bq(Stream *html, const Block *block, DocParser *parser)
{
    cassert_no_null(block);
    cassert(block->type == ekBQ);
    cassert_no_null(parser);
    post_bq(parser->post, html);
    i_html_tags(html, block->tags, parser);
    post_bq_end(parser->post, html);
}

/*---------------------------------------------------------------------------*/

void web_li(Stream *html, const Block *block, DocParser *parser)
{
    const char_t *icon;
    cassert_no_null(block);
    cassert(block->type == ekLI);
    cassert_no_null(parser);
    icon = doc_block_param(block, 0);
    if (icon != NULL)
    {
        String *imgpath = str_cpath("%s/img/%s", parser->docpath, icon);
        uint32_t width = doc_int_param(block, 1);
        post_li(parser->post, tc(imgpath), width, html, parser->status);
        str_destroy(&imgpath);
    }
    else
    {
        post_li(parser->post, NULL, UINT32_MAX, html, parser->status);
    }

    i_html_tags(html, block->tags, parser);
    post_li_end(parser->post, html);
    i_html_errors(parser->status, parser);
}

/*---------------------------------------------------------------------------*/

void web_lili(Stream *html, const Block *block, DocParser *parser)
{
    cassert_no_null(block);
    cassert(block->type == ekLILI);
    cassert_no_null(parser);
    post_lili(parser->post, html);
    i_html_tags(html, block->tags, parser);
    post_lili_end(parser->post, html);
}

/*---------------------------------------------------------------------------*/

void web_img(Stream *html, const Block *block, DocParser *parser)
{
    const char_t *imgname = doc_block_param(block, 0);
    real32_t width = doc_real_param(block, 1);
    String *imgpath = NULL;
    String *alt = NULL;
    cassert_no_null(parser);
    imgpath = str_cpath("%s/img/%s", parser->docpath, imgname);
    if (width < -.5f)
        width = parser->config->web_post_def_imgwidth;
    else if (width < 1.f)
        width = 16384;

    if (i_with_text(block->alt1) == TRUE)
    {
        alt = i_plain_str(block->alt1);
    }
    else
    {
        String *warn = str_printf("'%s' image without alt text", imgname);
        i_web_warn(&warn, parser);
    }

    post_img(parser->post, tc(imgpath), tc(alt), (uint32_t)width, html, parser->status);

    if (i_with_text(block->tags) == TRUE)
    {
        post_img_caption(parser->post, tc(block->ref), html);
        i_html_tags(html, block->tags, parser);
        post_img_caption_end(parser->post, html);
    }

    post_img_end(parser->post, html);
    str_destroy(&imgpath);
    str_destopt(&alt);
    i_html_errors(parser->status, parser);
}

/*---------------------------------------------------------------------------*/

void web_img2(Stream *html, const Block *block, DocParser *parser)
{
    const char_t *imgname1 = doc_block_param(block, 0);
    const char_t *imgname2 = doc_block_param(block, 1);
    real32_t width = doc_real_param(block, 2);
    String *imgpath1 = NULL;
    String *imgpath2 = NULL;
    String *alt1 = NULL;
    String *alt2 = NULL;
    cassert_no_null(parser);
    imgpath1 = str_cpath("%s/img/%s", parser->docpath, imgname1);
    imgpath2 = str_cpath("%s/img/%s", parser->docpath, imgname2);
    if (width < 1.f)
        width = parser->config->web_post_def_imgwidth / 2.f;

    if (i_with_text(block->alt1) == TRUE)
    {
        alt1 = i_plain_str(block->alt1);
    }
    else
    {
        String *warn = str_printf("'%s' image without alt text", imgname1);
        i_web_warn(&warn, parser);
    }

    if (i_with_text(block->alt2) == TRUE)
    {
        alt2 = i_plain_str(block->alt2);
    }
    else
    {
        String *warn = str_printf("'%s' image without alt text", imgname2);
        i_web_warn(&warn, parser);
    }

    post_img2(parser->post, tc(imgpath1), tc(imgpath2), tc(alt1), tc(alt2), (uint32_t)width, html, parser->status);

    if (i_with_text(block->tags) == TRUE)
    {
        post_img_caption(parser->post, tc(block->ref), html);
        i_html_tags(html, block->tags, parser);
        post_img_caption_end(parser->post, html);
    }

    post_img_end(parser->post, html);
    str_destroy(&imgpath1);
    str_destroy(&imgpath2);
    str_destopt(&alt1);
    str_destopt(&alt2);
    i_html_errors(parser->status, parser);
}

/*---------------------------------------------------------------------------*/

void web_table(Stream *html, const Block *block, DocParser *parser)
{
    const char_t *expand = doc_block_param(block, 2);
    const char_t *wrap = doc_block_param(block, 3);
    caption_t caption = ekCAPTION_NO;
    cassert_no_null(block);
    cassert_no_null(parser);

    if (expand != NULL)
    {
        if (str_equ_c(expand, "open") == TRUE)
        {
            caption = ekCAPTION_OPEN;
        }
        else if (str_equ_c(expand, "close") == TRUE)
        {
            caption = ekCAPTION_CLOSED;
        }
        else if (str_equ_c(expand, "no") == TRUE)
        {
            caption = ekCAPTION_NO;
        }
        else
        {
            String *warn = str_printf("Table caption unknown '%s'", expand);
            i_web_warn(&warn, parser);
        }
    }

    if (caption == ekCAPTION_NO && i_with_text(block->tags) == TRUE)
        caption = ekCAPTION_FIXED;

    switch (caption)
    {
    case ekCAPTION_NO:
        stm_writef(html, "\n<figure>\n");
        break;

    case ekCAPTION_FIXED:
        stm_writef(html, "\n<figure>\n");
        stm_writef(html, "<figcaption class='top'>\n");
        break;

    case ekCAPTION_OPEN:
        stm_writef(html, "\n<details open>\n");
        stm_writef(html, "<summary class='top center'>\n");
        break;

    case ekCAPTION_CLOSED:
        stm_writef(html, "\n<details>\n");
        stm_writef(html, "<summary class='top center'>\n");
        break;
        cassert_default();
    }

    if (i_with_text(block->tags) == TRUE)
    {
        stm_writef(html, "<span class='post-dec'>");
        stm_printf(html, "%s %s: ", respack_text(parser->respack, TEXT_20), tc(block->ref));
        stm_writef(html, "</span>");
        i_html_tags(html, block->tags, parser);
    }

    switch (caption)
    {
    case ekCAPTION_NO:
        break;
    case ekCAPTION_FIXED:
        stm_writef(html, "\n</figcaption>\n");
        break;
    case ekCAPTION_OPEN:
    case ekCAPTION_CLOSED:
        stm_writef(html, "\n</summary>\n");
        stm_writef(html, "<figure class='fdetails'>\n");
        break;
        cassert_default();
    }

    stm_writef(html, "<div class='divtable'>\n");
    stm_writef(html, "<table class='vtable'>\n");
    stm_writef(html, "<tbody>\n");

    arrst_foreach(child, block->children, Block)
        switch (child->type)
        {
        case ekROW:
            if (child_i == 0)
            {
                stm_writef(html, "<tr class='vhead'>\n");
            }
            else
            {
                const char_t *color = doc_block_param(child, 0);
                stm_writef(html, "</tr>\n");
                if (color != NULL)
                    stm_printf(html, "<tr bgcolor='%s'>\n", color);
                else
                    stm_writef(html, "<tr>\n");
            }
            break;

        case ekIMG:
        {
            const char_t *imgname = doc_block_param(child, 0);
            real32_t width = doc_real_param(child, 1);
            String *imgpath = str_cpath("%s/img/%s", parser->docpath, imgname);
            if (width < 1.f)
                width = 16384;
            stm_writef(html, "<td>");
            post_img_nofigure(parser->post, tc(imgpath), (uint32_t)width, TRUE, html, parser->status);
            stm_writef(html, "</td>\n");
            str_destroy(&imgpath);
            break;
        }

        case ekPARAG:
        {
            const char_t *align = doc_block_param(child, 0);

            stm_writef(html, "<td");
            if (align != NULL && str_equ_c(align, "left") == TRUE)
                stm_writef(html, " style=\"text-align: left;\"");

            if (wrap != NULL && str_equ_c(wrap, "no") == TRUE)
                stm_writef(html, " class='nowrap'");

            stm_writef(html, ">");

            i_html_tags(html, child->tags, parser);
            stm_writef(html, "</td>\n");
            break;
        }

        case ekBDRAFT:
        case ekNOSECNUM:
        case ekBLANGREV:
        case ekBCOMPLETE:
        case ekEBOOK_ONLY:
        case ekWEB_ONLY:
        case ekEPIG:
        case ekDESC:
        case ekH1:
        case ekH2:
        case ekH3:
        case ekIMG2:
        case ekTABLE:
        case ekLI:
        case ekLILI:
        case ekBQ:
        case ekCODE:
        case ekCODEFILE:
        case ekMATH:
        case ekTYPE:
        case ekCONST:
        case ekENUM:
        case ekENUMV:
        case ekSTRUCT:
        case ekSMEMBER:
        case ekFUNC:
        case ekFPAR:
        case ekFRET:
        case ekFCODE:
        case ekFNOTE:
        case ekWARN:
        case ekNOTAG:
        case ekNOTOC:
            cassert_default();
        }

    arrst_end()

    stm_writef(html, "</tr>\n");
    stm_writef(html, "</tbody>\n");
    stm_writef(html, "</table>\n");
    stm_writef(html, "</div>\n");
    stm_writef(html, "</figure>\n");

    switch (caption)
    {
    case ekCAPTION_NO:
    case ekCAPTION_FIXED:
        break;
    case ekCAPTION_OPEN:
    case ekCAPTION_CLOSED:
        stm_writef(html, "</details>\n");
        break;
        cassert_default();
    }

    i_html_errors(parser->status, parser);
}

/*---------------------------------------------------------------------------*/

void web_code(Stream *html, const Block *block, const char_t *clang, DocParser *parser)
{
    const char_t *stline = doc_block_param(block, 2);
    const char_t *expand = doc_block_param(block, 3);
    caption_t caption = ekCAPTION_NO;
    const char_t *blockref = NULL;
    const byte_t *data = NULL;
    uint32_t size = 0;
    uint32_t line = 1;
    cassert_no_null(block);
    cassert_no_null(parser);

    if (block->code == NULL)
    {
        String *warn = str_c("No sources in code block");
        i_web_warn(&warn, parser);
        return;
    }

    data = stm_buffer(block->code);
    size = stm_buffer_size(block->code);

    if (expand != NULL)
    {
        if (str_equ_c(expand, "open") == TRUE)
        {
            caption = ekCAPTION_OPEN;
        }
        else if (str_equ_c(expand, "close") == TRUE)
        {
            caption = ekCAPTION_CLOSED;
        }
        else if (str_equ_c(expand, "no") == TRUE)
        {
            caption = ekCAPTION_NO;
        }
        else
        {
            String *warn = str_printf("Code caption unknown '%s'", expand);
            i_web_warn(&warn, parser);
        }
    }

    if (caption == ekCAPTION_NO && i_with_text(block->tags) == TRUE)
        caption = ekCAPTION_FIXED;

    blockref = block->ref ? tc(block->ref) : NULL;
    scode_begin(parser->scode, caption, blockref, html);

    if (i_with_text(block->tags) == TRUE)
        i_html_tags(html, block->tags, parser);

    if (stline != NULL)
    {
        if (str_equ_c(stline, "no") == TRUE)
            line = UINT32_MAX;
        else
            line = str_to_u32(stline, 10, NULL);
    }
    else
    {
        line = 1;
    }

    scode_html(parser->scode, html, clang ? clang : "cpp", line, data, size, parser->listener);
}

/*---------------------------------------------------------------------------*/

void web_math(Stream *html, const Block *block, DocParser *parser)
{
    const char_t *name = doc_block_param(block, 0);
    String *desc = NULL;

    cassert_no_null(parser);

    if (i_with_text(block->tags) == TRUE)
    {
        desc = i_plain_str(block->tags);
    }
    else
    {
        String *warn = str_printf("'%s' formula without description", name);
        i_web_warn(&warn, parser);
    }

    post_math_img(parser->post, block->code, name, tc(desc), html, parser->status);

    if (i_with_text(block->tags) == TRUE)
    {
        post_math_img_caption(parser->post, tc(block->ref), name, html);
        i_html_tags(html, block->tags, parser);
        post_img_caption_end(parser->post, html);
    }

    post_img_end(parser->post, html);
    str_destopt(&desc);
    i_html_errors(parser->status, parser);
}

/*---------------------------------------------------------------------------*/

static void i_type_header(Stream *html, const Block *type, DocParser *parser)
{
    const char_t *name = doc_block_param(type, 0);
    cassert_no_null(type);
    stm_writef(html, "<tr>");
    stm_writef(html, "<td class='fret'>");
    stm_writef(html, "</td>");
    stm_writef(html, "<td class='fname'>");
    stm_printf(html, "<a href='#%s' title='", tc(type->ref));
    i_text(html, type->tags, parser);
    stm_writef(html, "'>");
    stm_writef(html, name ? name : "");
    stm_writef(html, "</a>");
    stm_writef(html, "</td>");
    stm_writef(html, "</tr>\n");
}

/*---------------------------------------------------------------------------*/

static void i_html_type(Stream *html, const char_t *type, const ptype_t ptype, const bool_t colorize, DocParser *parser)
{
    uint32_t i, n;
    const Block *block;
    const Doc *doc;

    if (type == NULL)
        return;

    switch (ptype)
    {
    case ekFLOAT:
        type = doc_float_type(type, parser->loader);
        break;
    case ekDOUBLE:
        type = doc_double_type(type, parser->loader);
        break;
    case ekREAL:
        type = doc_real_type(type, parser->loader);
        break;
    case ekOTHER:
        break;
        cassert_default();
    }

    n = str_len_c(type);
    for (i = 0; i < n; ++i)
    {
        if (type[i] == '(')
        {
            ((char_t *)type)[i] = '\0';
            break;
        }
    }

    doc = loader_type(parser->loader, type, &block);
    if (doc == NULL)
        doc = loader_func(parser->loader, type, &block);

    if (colorize == TRUE)
        stm_printf(html, "<span class='%s'>", scode_type_class(parser->scode));

    if (doc != NULL)
    {
        if (str_equ_c(doc_packname(doc), doc_packname(parser->doc)) == TRUE)
            stm_printf(html, "<a href='%s.html#", doc_name(doc));
        else
            stm_printf(html, "<a href='../%s/%s.html#", doc_packname(doc), doc_name(doc));

        stm_writef(html, tc(block->ref));
        stm_writef(html, "' title='");
        i_text(html, block->tags, parser);
        stm_writef(html, "'>");
        stm_writef(html, type);
        stm_writef(html, "</a>");
    }
    else
    {
        stm_writef(html, type);
    }

    if (i < n)
    {
        stm_printf(html, "(%s", type + i + 1);
        ((char_t *)type)[i] = '(';
    }

    if (colorize == TRUE)
        stm_writef(html, "</span>");
}

/*---------------------------------------------------------------------------*/

static void i_constant_header(Stream *html, const Block *ctype, DocParser *parser)
{
    const char_t *type = doc_block_param(ctype, 0);
    const char_t *name = doc_block_param(ctype, 1);
    cassert_no_null(ctype);
    stm_writef(html, "<tr>");
    stm_writef(html, "<td class='fret'>");
    i_html_type(html, type, doc_is_real_type(type, parser->loader) ? ekREAL : ekOTHER, FALSE, parser);
    stm_writef(html, "</td>");
    stm_writef(html, "<td class='fname'>");
    stm_printf(html, "<a href='#%s' title='", tc(ctype->ref));
    i_text(html, ctype->tags, parser);
    stm_writef(html, "'>");

    if (type != NULL && name != NULL)
    {
        if (doc_is_real_type(type, parser->loader))
        {
            String *cname = doc_real_constant(name);
            str_writef(html, cname);
            str_destroy(&cname);
        }
        else
        {
            stm_writef(html, name);
        }
    }

    stm_writef(html, "</a>");
    stm_writef(html, "</td>");
    stm_writef(html, "</tr>\n");
}

/*---------------------------------------------------------------------------*/

static void i_enum_header(Stream *html, const Block *etype, DocParser *parser)
{
    const char_t *type = doc_block_param(etype, 0);
    cassert_no_null(etype);
    stm_writef(html, "<tr>");
    stm_writef(html, "<td class='fret'>");
    stm_writef(html, "enum");
    stm_writef(html, "</td>");
    stm_writef(html, "<td class='fname'>");
    stm_printf(html, "<a href='#%s' title='", tc(etype->ref));
    i_text(html, etype->tags, parser);
    stm_writef(html, "'>");
    stm_writef(html, type ? type : "");
    stm_writef(html, "</a>");
    stm_writef(html, "</td>");
    stm_writef(html, "</tr>\n");
}

/*---------------------------------------------------------------------------*/

static void i_struct_header(Stream *html, const Block *stype, DocParser *parser)
{
    const char_t *name = doc_block_param(stype, 0);
    cassert_no_null(stype);
    stm_writef(html, "<tr>");
    stm_writef(html, "<td class='fret'>");
    stm_writef(html, "struct");
    stm_writef(html, "</td>");
    stm_writef(html, "<td class='fname'>");
    stm_printf(html, "<a href='#%s' title='", tc(stype->ref));
    i_text(html, stype->tags, parser);
    stm_writef(html, "'>");
    stm_writef(html, name ? name : "");
    stm_writef(html, "</a>");
    stm_writef(html, "</td>");
    stm_writef(html, "</tr>\n");
}

/*---------------------------------------------------------------------------*/

void web_types_header(Stream *html, const ArrSt(Block) *types, DocParser *parser)
{
    stm_writef(html, "\n<table class='ftable'>\n");
    stm_writef(html, "<tbody>\n");

    arrst_foreach_const(type, types, Block)
        if (type->type == ekTYPE)
            i_type_header(html, type, parser);
    arrst_end()

    arrst_foreach_const(type, types, Block)
        if (type->type == ekCONST)
            i_constant_header(html, type, parser);
    arrst_end()

    arrst_foreach_const(type, types, Block)
        if (type->type == ekENUM)
            i_enum_header(html, type, parser);
    arrst_end()

    arrst_foreach_const(type, types, Block)
        if (type->type == ekSTRUCT)
            i_struct_header(html, type, parser);
    arrst_end()

    stm_writef(html, "</tbody>\n");
    stm_writef(html, "</table>\n");
}

/*---------------------------------------------------------------------------*/

static void i_type_body(Stream *html, const Block *type, DocParser *parser)
{
    const char_t *name = doc_block_param(type, 0);
    cassert_no_null(parser);
    post_line(parser->post, html);
    post_h2(parser->post, tc(type->ref), "ftitle", FALSE, html);
    stm_writef(html, name ? name : "");
    post_h2_end(parser->post, html);
    stm_writef(html, "<p>");
    i_html_tags(html, type->tags, parser);
    stm_writef(html, "</p>");
}

/*---------------------------------------------------------------------------*/

static void i_write_constant(Stream *html, const char_t *type, const ptype_t ptype, const char_t *name, const char_t *noconst, const char_t *val, const char_t *valsufix, DocParser *parser)
{
    if (!noconst || !str_equ_c(noconst, "noconst"))
        stm_writef(html, "const ");

    i_html_type(html, type, ptype, TRUE, parser);
    stm_writef(html, " ");
    i_hwrite(html, name ? name : "");

    if (str_empty_c(val))
        stm_writef(html, ";");
    else
        stm_printf(html, " = %s%s;", val, valsufix ? valsufix : "");
}

/*---------------------------------------------------------------------------*/

static void i_constant_body(Stream *html, const Block *ctype, DocParser *parser)
{
    const char_t *type = doc_block_param(ctype, 0);
    const char_t *name = doc_block_param(ctype, 1);
    const char_t *val = doc_block_param(ctype, 2);
    const char_t *noconst = doc_block_param(ctype, 3);
    cassert_no_null(parser);
    post_line(parser->post, html);
    post_h2(parser->post, tc(ctype->ref), "ftitle", FALSE, html);

    if (type != NULL && name != NULL)
    {
        if (doc_is_real_type(type, parser->loader))
        {
            String *cname = doc_real_constant(name);
            str_writef(html, cname);
            str_destroy(&cname);
        }
        else
        {
            stm_writef(html, name);
        }
    }

    post_h2_end(parser->post, html);

    stm_writef(html, "\n<pre class='fdesc'>\n");

    if (type != NULL && name != NULL)
    {
        if (doc_is_real_type(type, parser->loader))
        {
            String *cfloat = doc_float_constant(type, name);
            String *cdouble = doc_double_constant(type, name);
            String *ctempl = doc_template_constant(type, name);
            i_write_constant(html, type, ekFLOAT, tc(cfloat), noconst, val, "f", parser);
            stm_writef(html, "\n\n");
            i_write_constant(html, type, ekDOUBLE, tc(cdouble), noconst, val, "", parser);
            stm_writef(html, "\n\n");
            i_write_constant(html, type, ekREAL, tc(ctempl), noconst, NULL, NULL, parser);
            str_destroy(&cfloat);
            str_destroy(&cdouble);
            str_destroy(&ctempl);
        }
        else
        {
            i_write_constant(html, type, ekOTHER, name, noconst, val, "", parser);
        }
    }

    stm_writef(html, "</pre>\n");
    stm_writef(html, "<p>");
    i_html_tags(html, ctype->tags, parser);
    stm_writef(html, "</p>");
}

/*---------------------------------------------------------------------------*/

static void i_enum_body(Stream *html, const Block *etype, DocParser *parser)
{
    const char_t *type = doc_block_param(etype, 0);
    cassert_no_null(parser);
    post_line(parser->post, html);
    post_h2(parser->post, tc(etype->ref), "ftitle", FALSE, html);
    stm_printf(html, "enum %s\n", type ? type : "");
    post_h2_end(parser->post, html);
    stm_writef(html, "<p>");
    i_html_tags(html, etype->tags, parser);
    stm_writef(html, "</p>");

    stm_writef(html, "\n<pre class='fdesc'>\n");
    stm_printf(html, "enum <span class='%s'>%s</span>\n", scode_type_class(parser->scode), type ? type : "");
    stm_writef(html, "{\n");
    arrst_foreach(value, etype->children, Block)
        const char_t *val0 = doc_block_param(value, 0);
        stm_writef(html, "    ");
        stm_writef(html, val0 ? val0 : "NOVALUE");
        if (value_i < value_total - 1)
            stm_writef(html, ",\n");
    arrst_end()

    stm_writef(html, "\n};\n");
    stm_writef(html, "</pre>\n");

    stm_writef(html, "\n<table class='ftable'>\n");
    stm_writef(html, "<tbody>\n");
    arrst_foreach(value, etype->children, Block)
        if (i_with_text(value->tags))
        {
            const char_t *val0 = doc_block_param(value, 0);
            stm_writef(html, "<tr>");
            stm_writef(html, "<td class='fenum'>");
            stm_printf(html, "<span class='%s'>%s</span>", scode_const_class(parser->scode), val0 ? val0 : "NOVALUE");
            stm_writef(html, "</td>");
            stm_writef(html, "<td class='fdesc'>");
            stm_writef(html, "<p>");
            i_html_tags(html, value->tags, parser);
            stm_writef(html, "</p>");
            stm_writef(html, "</td>");
            stm_writef(html, "</tr>\n");
        }
    arrst_end()

    stm_writef(html, "</tbody>\n");
    stm_writef(html, "</table>\n");
}

/*---------------------------------------------------------------------------*/

static void i_write_struct(Stream *html, const char_t *name, const ptype_t ptype, const Block *sblock, DocParser *parser)
{
    if (ptype == ekREAL)
        i_hwrite(html, "template<typename real>\n");

    stm_printf(html, "struct <span class='%s'>%s</span>", scode_type_class(parser->scode), name ? name : "");
    if (arrst_size(sblock->children, Block) > 0)
    {
        stm_writef(html, "\n{\n");
        arrst_foreach(member, sblock->children, Block)
            const Block *block;
            const char_t *mtype = doc_block_param(member, 0);
            const char_t *mname = doc_block_param(member, 1);
            const Doc *doc;

            if (doc_is_real_type(mtype, parser->loader) == TRUE)
            {
                switch (ptype)
                {
                case ekFLOAT:
                    mtype = doc_float_type(mtype, parser->loader);
                    break;
                case ekDOUBLE:
                    mtype = doc_double_type(mtype, parser->loader);
                    break;
                case ekREAL:
                    mtype = doc_real_type(mtype, parser->loader);
                    break;
                case ekOTHER:
                    break;
                    cassert_default();
                }
            }

            doc = loader_type(parser->loader, mtype ? mtype : "", &block);

            stm_printf(html, "    <span class='%s'>", scode_type_class(parser->scode));
            if (doc != NULL)
            {
                if (str_equ_c(doc_packname(doc), doc_packname(parser->doc)) == TRUE)
                    stm_printf(html, "<a href='%s.html#", doc_name(doc));
                else
                    stm_printf(html, "<a href='../%s/%s.html#", doc_packname(doc), doc_name(doc));

                stm_writef(html, tc(block->ref));
                stm_writef(html, "' title='");
                i_text(html, block->tags, parser);
                stm_writef(html, "'>");
                stm_writef(html, mtype);
                stm_writef(html, "</a>");
            }
            else
            {
                stm_writef(html, mtype);
            }

            stm_writef(html, "</span>");
            stm_printf(html, " %s;\n", mname ? mname : "");
        arrst_end()
        stm_writef(html, "};\n");
    }
    else
    {
        stm_writef(html, ";\n");
    }
}

/*---------------------------------------------------------------------------*/

static void i_struct_body(Stream *html, const Block *sblock, DocParser *parser)
{
    const char_t *name = doc_block_param(sblock, 0);
    const char_t *type = doc_block_param(sblock, 1);
    cassert_no_null(parser);
    post_line(parser->post, html);
    post_h2(parser->post, tc(sblock->ref), "ftitle", FALSE, html);
    stm_printf(html, "struct %s", name ? name : "");
    post_h2_end(parser->post, html);
    stm_writef(html, "<p>");
    i_html_tags(html, sblock->tags, parser);
    stm_writef(html, "</p>");

    stm_writef(html, "\n<pre class='fdesc'>\n");

    if (type && str_equ_c(type, "real"))
    {
        String *fstruct = doc_float_struct(name);
        String *dstruct = doc_double_struct(name);
        String *tstruct = doc_template_struct(name);
        i_write_struct(html, tc(fstruct), ekFLOAT, sblock, parser);
        stm_writef(html, "\n");
        i_write_struct(html, tc(dstruct), ekDOUBLE, sblock, parser);
        stm_writef(html, "\n");
        i_write_struct(html, tc(tstruct), ekREAL, sblock, parser);
        str_destroy(&fstruct);
        str_destroy(&dstruct);
        str_destroy(&tstruct);
    }
    else
    {
        i_write_struct(html, name, ekOTHER, sblock, parser);
    }

    stm_writef(html, "</pre>\n");

    if (arrst_size(sblock->children, Block) > 0)
    {
        stm_writef(html, "\n<table class='ftable'>\n");
        stm_writef(html, "<tbody>\n");
        arrst_foreach(member, sblock->children, Block)
            const char_t *mname = doc_block_param(member, 1);
            stm_writef(html, "<tr>");
            stm_writef(html, "<td class='fenum'>");
            stm_writef(html, mname ? mname : "");
            stm_writef(html, "</td>");
            stm_writef(html, "<td class='fdesc'>");
            stm_writef(html, "<p>");
            i_html_tags(html, member->tags, parser);
            stm_writef(html, "</p>");
            stm_writef(html, "</td>");
            stm_writef(html, "</tr>\n");
        arrst_end()

        stm_writef(html, "</tbody>\n");
        stm_writef(html, "</table>\n");
    }
}

/*---------------------------------------------------------------------------*/

void web_types_body(Stream *html, const ArrSt(Block) *types, DocParser *parser)
{
    arrst_foreach_const(type, types, Block)
        if (type->type == ekTYPE)
        {
            parser->line = type->line;
            i_type_body(html, type, parser);
        }
    arrst_end()

    arrst_foreach_const(type, types, Block)
        if (type->type == ekCONST)
        {
            parser->line = type->line;
            i_constant_body(html, type, parser);
        }
    arrst_end()

    arrst_foreach_const(type, types, Block)
        if (type->type == ekENUM)
        {
            parser->line = type->line;
            i_enum_body(html, type, parser);
        }
    arrst_end()

    arrst_foreach_const(type, types, Block)
        if (type->type == ekSTRUCT)
        {
            parser->line = type->line;
            i_struct_body(html, type, parser);
        }
    arrst_end()
}

/*---------------------------------------------------------------------------*/

static void i_html_param(Stream *html, const Block *param, const ptype_t ptype, const bool_t colorize, DocParser *parser)
{
    const char_t *name = NULL;
    if (param == NULL)
    {
        stm_writef(html, "void");
        return;
    }

    name = doc_block_param(param, 1);
    if (str_empty(param->ptype) == FALSE)
    {
        if (param->is_const == TRUE)
            stm_writef(html, "const ");

        i_html_type(html, tc(param->ptype), ptype, colorize, parser);

        if (name != NULL)
            stm_writef(html, " ");

        if (param->is_dptr == TRUE)
            stm_writef(html, "**");
        else if (param->is_ptr == TRUE)
            stm_writef(html, "*");
    }

    if (name != NULL)
        stm_writef(html, name);
}

/*---------------------------------------------------------------------------*/

void web_funcs_header(Stream *html, const ArrSt(Block) *funcs, DocParser *parser)
{
    stm_writef(html, "\n<table class='ftable'>\n");
    stm_writef(html, "<tbody>\n");
    arrst_foreach_const(func, funcs, Block)
        const char_t *fname = doc_block_param(func, 0);
        const char_t *alias = doc_block_param(func, 1);
        const Block *fret = NULL;
        const Block *fparams = NULL;
        uint32_t nparams = 0;

        doc_func_params(func, &fret, &fparams, &nparams);

        stm_writef(html, "<tr>");
        stm_writef(html, "<td class='fret'>");
        i_html_param(html, fret, ekOTHER, FALSE, parser);
        stm_writef(html, "</td>");
        stm_writef(html, "<td class='fname'>");
        if (func->is_ptr)
            stm_writef(html, "(*");
        stm_printf(html, "<a href='#%s' title='", tc(func->ref));
        i_text(html, func->tags, parser);
        stm_writef(html, "'>");

        if (alias != NULL && !func->is_ptr)
        {
            String *rfunc = doc_real_func(fname, alias);
            stm_writef(html, tc(rfunc));
            str_destroy(&rfunc);
        }
        else
        {
            stm_writef(html, fname ? fname : "");
        }

        stm_writef(html, "</a>");

        if (nparams == 0)
            stm_writef(html, " (void)");
        else
            stm_writef(html, " (...)");

        if (func->is_ptr)
            stm_writef(html, ")");

        stm_writef(html, "</td>");
        stm_writef(html, "</tr>\n");
    arrst_end()

    stm_writef(html, "</tbody>\n");
    stm_writef(html, "</table>\n");
}

/*---------------------------------------------------------------------------*/

static void i_write_func(Stream *html, const Block *func, const char_t *fname, const ptype_t ptype, DocParser *parser)
{
    const Block *fret = NULL;
    const Block *fparams = NULL;
    uint32_t i, nparams = 0;
    uint32_t nsize = 0;
    String *blanks = NULL;

    doc_func_params(func, &fret, &fparams, &nparams);
    i_html_param(html, fret, ptype, TRUE, parser);
    stm_writef(html, "\n");

    nsize = fname ? str_len_c(fname) : 0;
    nsize += 1; /* "(" */

    if (func->is_ptr == TRUE)
    {
        stm_writef(html, "(*");
        nsize += 3;
    }

    blanks = str_fill(nsize, ' ');
    stm_printf(html, "<span class='%s'>", scode_func_class(parser->scode));
    i_hwrite(html, fname ? fname : "");
    stm_writef(html, "</span>");

    if (func->is_ptr == TRUE)
        stm_writef(html, ")");

    stm_writef(html, "(");
    if (nparams == 0)
        stm_writef(html, "void");

    for (i = 0; i < nparams; ++i)
    {
        i_html_param(html, fparams + i, ptype, TRUE, parser);
        if (i < nparams - 1)
        {
            stm_writef(html, ",\n");
            str_writef(html, blanks);
        }
    }

    stm_writef(html, ");\n");
    str_destroy(&blanks);
}

/*---------------------------------------------------------------------------*/

void web_funcs_body(Stream *html, const ArrSt(Block) *funcs, DocParser *parser)
{
    arrst_foreach_const(func, funcs, Block)
        if (func->type == ekFUNC)
        {
            const char_t *fname = doc_block_param(func, 0);
            const char_t *alias = doc_block_param(func, 1);
            const Block *fret = NULL;
            const Block *fparams = NULL;
            uint32_t i, nparams = 0;
            parser->line = func->line;

            doc_func_params(func, &fret, &fparams, &nparams);

            post_line(parser->post, html);
            post_h2(parser->post, tc(func->ref), "ftitle", FALSE, html);

            if (alias != NULL && !func->is_ptr)
            {
                String *rfunc = doc_real_func(fname, alias);
                stm_writef(html, tc(rfunc));
                str_destroy(&rfunc);
            }
            else
            {
                stm_writef(html, fname ? fname : "");
            }

            if (func->is_ptr == FALSE)
                stm_writef(html, " ()");

            post_h2_end(parser->post, html);

            stm_writef(html, "\n<p>");
            i_html_tags(html, func->tags, parser);
            stm_writef(html, "</p>\n");

            stm_writef(html, "\n<pre class='fdesc'>\n");

            if (alias != NULL && !func->is_ptr)
            {
                String *namef = doc_float_func(fname, alias);
                String *named = doc_double_func(fname, alias);
                String *namet = doc_template_func(fname, alias);
                i_write_func(html, func, tc(namef), ekFLOAT, parser);
                stm_writef(html, "\n");
                i_write_func(html, func, tc(named), ekDOUBLE, parser);
                stm_writef(html, "\n");
                i_write_func(html, func, tc(namet), ekREAL, parser);
                str_destroy(&namef);
                str_destroy(&named);
                str_destroy(&namet);
            }
            else
            {
                i_write_func(html, func, fname, ekOTHER, parser);
            }

            stm_writef(html, "</pre>\n");

            if (func->code != NULL)
            {
                const byte_t *data = NULL;
                uint32_t size = 0;
                data = stm_buffer(func->code);
                size = stm_buffer_size(func->code);
                scode_begin(parser->scode, ekCAPTION_NO, NULL, html);
                scode_html(parser->scode, html, "cpp", 1, data, size, parser->listener);
            }

            if (nparams > 0)
            {
                stm_writef(html, "\n<table class='ftable'>\n");
                stm_writef(html, "<tbody>\n");

                for (i = 0; i < nparams; ++i)
                {
                    const char_t *name = doc_block_param(fparams + i, 1);
                    stm_writef(html, "<tr>");
                    stm_writef(html, "<td class='fret'>");
                    stm_writef(html, name ? name : "");
                    stm_writef(html, "</td>");
                    stm_writef(html, "<td class='fdesc'>");
                    stm_writef(html, "<p>");
                    i_html_tags(html, fparams[i].tags, parser);
                    stm_writef(html, "</p>");
                    stm_writef(html, "</td>");
                    stm_writef(html, "</tr>\n");
                }

                stm_writef(html, "</tbody>\n");
                stm_writef(html, "</table>\n");
            }

            if (fret != NULL)
            {
                stm_writef(html, "\n<p class='noindent'>");
                stm_writef(html, "<b>");
                stm_writef(html, respack_text(parser->respack, TEXT_24));
                stm_writef(html, "</b>");
                stm_writef(html, "</p>");

                stm_writef(html, "<p>");
                i_html_tags(html, fret->tags, parser);
                stm_writef(html, "</p>");
                stm_writef(html, "\n");
            }

            if (func->alt1 != NULL)
            {
                stm_writef(html, "\n<p class='noindent'>");
                stm_writef(html, "<b>");
                stm_writef(html, respack_text(parser->respack, TEXT_25));
                stm_writef(html, "</b>");
                stm_writef(html, "</p>");

                stm_writef(html, "<p>");
                i_html_tags(html, func->alt1, parser);
                stm_writef(html, "</p>");
                stm_writef(html, "\n");
            }
        }
    arrst_end()
}
