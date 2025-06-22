/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: wsite.c
 *
 */

/* Html5 WebSite */

#include "wsite.h"
#include "wsite.inl"
#include "css.inl"
#include "css.h"
#include "head.inl"
#include "header.inl"
#include "html5.h"
#include "nav.inl"
#include "lnav.inl"
#include "post.inl"
#include "rcol.inl"
#include "scode.inl"
#include <draw2d/color.h>
#include <draw2d/image.h>
#include <draw2d/pixbuf.h>
#include <core/arrpt.h>
#include <core/buffer.h>
#include <core/date.h>
#include <core/event.h>
#include <core/heap.h>
#include <core/hfile.h>
#include <core/stream.h>
#include <core/strings.h>
#include <osbs/bproc.h>
#include <osbs/btime.h>
#include <sewer/bstd.h>
#include <sewer/cassert.h>
#include <sewer/ptr.h>

struct _wsite_t
{
    ArrPt(String) *comments;
    String *site_fontfile;
    String *head_fontfile;
    String *mono_fontfile;
    String *prjurl;
    String *prjtitle;
    String *respath;
    String *webpath;
    String *tempath;
    String *webres;
    String *curpath;
    String *curfolder;
    String *webimg;
    ArrPt(String) *langs;
    uint32_t langid;
    String *footer;
    Stream *sitemap;
    color_t bkcolor;
    uint32_t level;
    Head *head;
    Header *header;
    Nav *nav;
    LNav *lnav;
    Post *post;
    RCol *rcol;
    SCode *scode;
};

/*---------------------------------------------------------------------------*/

WSite *wsite_create(const char_t *project_url, const char_t *respath, const char_t *webpath, const char_t *tempath, const color_t bkcolor, Html5Status *status)
{
    WSite *site = heap_new0(WSite);
    site->comments = arrpt_create(String);
    site->site_fontfile = str_c("");
    site->head_fontfile = str_c("");
    site->mono_fontfile = str_c("");
    site->prjurl = str_c(project_url);
    site->respath = str_cpath("%s", respath);
    site->webpath = str_cpath("%s", webpath);
    site->tempath = str_cpath("%s", tempath);
    site->webres = str_cpath("%s/res", webpath);
    site->curpath = str_cpath("%s", webpath);
    site->curfolder = str_c("");
    site->webimg = str_cpath("%s/img", webpath);
    site->langs = arrpt_create(String);
    site->langid = UINT32_MAX;
    site->footer = str_c("");
    site->bkcolor = bkcolor;
    site->level = 0;

    if (hfile_dir_create(tc(site->webres), NULL) == FALSE)
    {
        String *err = str_printf("[wsite]-Error creating '%s' dir", tc(site->webres));
        html5_status_err(status, &err);
    }

    if (hfile_dir_create(tc(site->webres), NULL) == FALSE)
    {
        String *err = str_printf("[wsite]-Error creating '%s' dir", tc(site->webres));
        html5_status_err(status, &err);
    }

    return site;
}

/*---------------------------------------------------------------------------*/

void wsite_destroy(WSite **site)
{
    cassert_no_null(site);
    cassert_no_null(*site);
    arrpt_destroy(&(*site)->comments, str_destroy, String);
    str_destroy(&(*site)->site_fontfile);
    str_destroy(&(*site)->head_fontfile);
    str_destroy(&(*site)->mono_fontfile);
    str_destroy(&(*site)->prjurl);
    str_destroy(&(*site)->prjtitle);
    str_destroy(&(*site)->respath);
    str_destroy(&(*site)->webpath);
    str_destroy(&(*site)->tempath);
    str_destroy(&(*site)->webres);
    str_destroy(&(*site)->curpath);
    str_destroy(&(*site)->curfolder);
    str_destroy(&(*site)->webimg);
    arrpt_destroy(&(*site)->langs, str_destroy, String);
    str_destroy(&(*site)->footer);
    ptr_destopt(head_destroy, &(*site)->head, Head);
    ptr_destopt(header_destroy, &(*site)->header, Header);
    ptr_destopt(nav_destroy, &(*site)->nav, Nav);
    ptr_destopt(lnav_destroy, &(*site)->lnav, LNav);
    ptr_destopt(post_destroy, &(*site)->post, Post);
    ptr_destopt(rcol_destroy, &(*site)->rcol, RCol);
    ptr_destopt(scode_destroy, &(*site)->scode, SCode);
    heap_delete(site, WSite);
}

/*---------------------------------------------------------------------------*/

void wsite_site_font(WSite *site, const char_t *font_file, Html5Status *status)
{
    cassert_no_null(site);
    cassert(str_empty(site->site_fontfile) == TRUE);
    str_upd(&site->site_fontfile, font_file);
    wsite_add_res(site, font_file, status);
}

/*---------------------------------------------------------------------------*/

void wsite_head_font(WSite *site, const char_t *font_file, Html5Status *status)
{
    cassert_no_null(site);
    cassert(str_empty(site->head_fontfile) == TRUE);
    str_upd(&site->head_fontfile, font_file);
    wsite_add_res(site, font_file, status);
}

/*---------------------------------------------------------------------------*/

void wsite_mono_font(WSite *site, const char_t *font_file, Html5Status *status)
{
    cassert_no_null(site);
    cassert(str_empty(site->mono_fontfile) == TRUE);
    str_upd(&site->mono_fontfile, font_file);
    wsite_add_res(site, font_file, status);
}

/*---------------------------------------------------------------------------*/

void wsite_clear_comments(WSite *site)
{
    cassert_no_null(site);
    arrpt_clear(site->comments, str_destroy, String);
}

/*---------------------------------------------------------------------------*/

void wsite_comment_c(WSite *site, const char_t *comment)
{
    String *com = str_c(comment);
    cassert_no_null(site);
    arrpt_append(site->comments, com, String);
}

/*---------------------------------------------------------------------------*/

void wsite_comment(WSite *site, String **comment)
{
    cassert_no_null(site);
    cassert_no_null(comment);
    arrpt_append(site->comments, *comment, String);
    *comment = NULL;
}

/*---------------------------------------------------------------------------*/

void wsite_head(WSite *site, Head *head, Html5Status *status)
{
    cassert_no_null(site);
    cassert(site->head == NULL);
    site->head = head;
    head_site(head, site, status);
}

/*---------------------------------------------------------------------------*/

void wsite_header(WSite *site, Header *header, Html5Status *status)
{
    cassert_no_null(site);
    cassert(site->header == NULL);
    site->header = header;
    header_site(header, site, status);
}

/*---------------------------------------------------------------------------*/

void wsite_nav(WSite *site, Nav *nav, Html5Status *status)
{
    cassert_no_null(site);
    cassert(site->nav == NULL);
    site->nav = nav;
    nav_site(nav, site, status);
}

/*---------------------------------------------------------------------------*/

void wsite_lnav(WSite *site, LNav *lnav)
{
    cassert_no_null(site);
    cassert(site->lnav == NULL);
    site->lnav = lnav;
    lnav_site(lnav, site);
}

/*---------------------------------------------------------------------------*/

void wsite_post(WSite *site, Post *post)
{
    cassert_no_null(site);
    cassert(site->post == NULL);
    site->post = post;
    post_site(post, site);
}

/*---------------------------------------------------------------------------*/

void wsite_rcol(WSite *site, RCol *rcol)
{
    cassert_no_null(site);
    cassert(site->rcol == NULL);
    site->rcol = rcol;
    rcol_site(rcol, site);
}

/*---------------------------------------------------------------------------*/

void wsite_scode(WSite *site, SCode *code)
{
    cassert_no_null(site);
    cassert(site->scode == NULL);
    site->scode = code;
    scode_site(code, site);
}

/*---------------------------------------------------------------------------*/

void wsite_footer(WSite *site, String **footer)
{
    cassert_no_null(site);
    str_destroy(&site->footer);
    site->footer = *footer;
    *footer = NULL;
}

/*---------------------------------------------------------------------------*/

static const char_t *i_font_format(const char_t *ext)
{
    if (str_equ_c(ext, "ttf") == TRUE)
        return "truetype";

    /* TODO: Update with other font types */
    cassert(FALSE);
    return ext;
}

/*---------------------------------------------------------------------------*/

static void i_css(WSite *site, CSS *css, Html5Status *status)
{
    Date date;
    String *sdate;
    cassert_no_null(site);

    arrpt_foreach(comment, site->comments, String)
        css_comment(css, tc(comment));
    arrpt_end()

    btime_date(&date);
    sdate = date_format(&date, "%d/%m/%Y-%H:%M:%S");
    css_comment(css, tc(sdate));
    str_destroy(&sdate);

    if (!str_empty(site->site_fontfile))
    {
        const char_t *ext = str_filext(tc(site->site_fontfile));
        String *fprop = str_printf("url(%s) format('%s')", tc(site->site_fontfile), i_font_format(ext));
        css_sel(css, "@font-face");
        css_str(css, "font-family", "SiteFont");
        css_str(css, "font-display", "auto");
        css_str(css, "font-style", "normal");
        css_str(css, "font-weight", "400");
        css_str(css, "src", tc(fprop));
        css_sele(css);
        str_destroy(&fprop);
    }

    if (!str_empty(site->head_fontfile))
    {
        const char_t *ext = str_filext(tc(site->head_fontfile));
        String *fprop = str_printf("url(%s) format('%s')", tc(site->head_fontfile), i_font_format(ext));
        css_sel(css, "@font-face");
        css_str(css, "font-family", "HeadFont");
        css_str(css, "font-display", "auto");
        css_str(css, "font-style", "normal");
        css_str(css, "font-weight", "400");
        css_str(css, "src", tc(fprop));
        css_sele(css);
        str_destroy(&fprop);
    }

    if (!str_empty(site->mono_fontfile))
    {
        const char_t *ext = str_filext(tc(site->mono_fontfile));
        String *fprop = str_printf("url(%s) format('%s')", tc(site->mono_fontfile), i_font_format(ext));
        css_sel(css, "@font-face");
        css_str(css, "font-family", "MonoFont");
        css_str(css, "font-display", "auto");
        css_str(css, "font-style", "normal");
        css_str(css, "font-weight", "400");
        css_str(css, "src", tc(fprop));
        css_sele(css);
        str_destroy(&fprop);
    }

    /* CSS Reset */
    {
        css_sel(css, "body, header, nav, div, section, h1, h2, h3, p, pre, code, blockquote, ul, li, figure, figcaption, img");
        /* css_rgba(css, "background-color", site->bkcolor); */
        css_px(css, "margin", 0.f);
        css_px(css, "padding", 0.f);
        css_px(css, "border", 0.f);
        css_str(css, "width", "auto");
        css_str(css, "vertical-align", "baseline");
        if (!str_empty(site->site_fontfile))
            css_str(css, "font-family", "SiteFont,Arial,sans-serif");
        else
            css_str(css, "font-family", "Arial,sans-serif");
        css_px(css, "font-size", 16.f);
        css_int(css, "font-weight", 400);
        css_sele(css);
    }

    css_sel(css, ".body-div");
    css_str(css, "width", "100%");
    css_sele(css);

    css_sel(css, ".nav-sticky + .body-div");
    css_px(css, "padding-top", wsite_nav_height(site));
    css_sele(css);

    if (site->header != NULL)
        header_css(site->header, css, status);

    if (site->nav != NULL)
        nav_css(site->nav, css, status);

    if (site->lnav != NULL)
        lnav_css(site->lnav, css);

    if (site->post != NULL)
        post_css(site->post, css);

    if (site->rcol != NULL)
        rcol_css(site->rcol, css);

    if (site->scode != NULL)
        scode_css(site->scode, css);
}

/*---------------------------------------------------------------------------*/

static void i_js_sec(WSite *site, const jssec_t sec, Stream *js)
{
    if (site->header != NULL)
        header_js(site->header, sec, js);

    if (site->nav != NULL)
        nav_js(site->nav, sec, js);

    if (site->lnav != NULL)
        lnav_js(site->lnav, sec, js);

    if (site->post != NULL)
        post_js(site->post, sec, js);

    if (site->rcol != NULL)
        rcol_js(site->rcol, sec, js);

    if (site->scode != NULL)
        scode_js(site->scode, sec, js);
}

/*---------------------------------------------------------------------------*/

static void i_js(WSite *site, Stream *js)
{
    Date date;
    String *sdate;
    cassert_no_null(site);

    arrpt_foreach(comment, site->comments, String)
        stm_writef(js, "// ");
        stm_writef(js, tc(comment));
        stm_writef(js, "\n");
    arrpt_end()

    btime_date(&date);
    sdate = date_format(&date, "%d/%m/%Y-%H:%M:%S");
    stm_writef(js, "// ");
    stm_writef(js, tc(sdate));
    stm_writef(js, "\n");
    str_destroy(&sdate);

    i_js_sec(site, ekJS_CODE, js);

    stm_writef(js, "\nwindow.onscroll = function() { on_wscroll() };\n");
    /* stm_writef(js, "window.onhashchange = function() { on_window_scroll() }; */
    stm_writef(js, "window.onresize = function () { on_wresize() };\n");
    stm_writef(js, "window.onload = function () { on_load() };\n");

    stm_writef(js, "\nfunction on_wscroll() {\n");
    i_js_sec(site, ekJS_ONSCROLL, js);

    stm_writef(js, "}\n");
    stm_writef(js, "\nfunction on_wresize() {\n");
    i_js_sec(site, ekJS_ONRESIZE, js);
    stm_writef(js, "}\n");
    stm_writef(js, "\nfunction on_load() {\n");
    i_js_sec(site, ekJS_ONLOAD, js);
    stm_writef(js, "}\n");
}

/*---------------------------------------------------------------------------*/

void wsite_add_lang(WSite *site, const char_t *lang, Html5Status *status)
{
    String *l = str_c(lang);
    String *langicon = str_printf("%s.png", lang);
    cassert_no_null(site);
    arrpt_append(site->langs, l, String);
    wsite_add_res(site, tc(langicon), status);
    str_destroy(&langicon);
}

/*---------------------------------------------------------------------------*/

static const char_t *i_lang(const WSite *site)
{
    const String *l;
    cassert_no_null(site);
    l = arrpt_get(site->langs, site->langid, String);
    return tc(l);
}

/*---------------------------------------------------------------------------*/

void wsite_begin_lang(WSite *site, const char_t *lang, const char_t *title, Listener *listener, Html5Status *status)
{
    String *str = NULL;
    String *sitemap = NULL;
    String *cssfile = NULL;
    String *jsfile = NULL;
    CSS *css = NULL;
    Stream *js = NULL;
    cassert_no_null(site);
    cassert(site->sitemap == NULL);
    str_destroy(&site->curpath);
    str = arrpt_search(site->langs, str_cmp, lang, &site->langid, String, char_t);
    unref(str);
    cassert(site->langid < arrpt_size(site->langs, String));
    str_upd(&site->prjtitle, title);

    site->curpath = str_cpath("%s/%s", tc(site->webpath), lang);
    if (hfile_exists(tc(site->curpath), NULL) == FALSE)
        hfile_dir_create(tc(site->curpath), NULL);

    sitemap = str_cpath("%s/sitemap.txt", tc(site->curpath));
    site->sitemap = stm_to_file(tc(sitemap), NULL);
    site->level = 1;

    if (hfile_dir_create(tc(site->curpath), NULL) == FALSE)
    {
        String *err = str_printf("[wsite]-Error creating '%s' dir", tc(site->curpath));
        html5_status_err(status, &err);
    }

    cssfile = str_cpath("%s/style_%s.css", tc(site->webres), lang);
    css = css_create(tc(cssfile), NULL);
    if (css != NULL)
    {
        i_css(site, css, status);
        if (listener != NULL)
            listener_event(listener, ekCSS_CONTENT, site, css, NULL, WSite, CSS, void);
        css_destroy(&css);
    }
    else
    {
        String *err = str_printf("[wsite]-Error creating '%s' dir", tc(cssfile));
        html5_status_err(status, &err);
    }

    jsfile = str_cpath("%s/code_%s.js", tc(site->webres), lang);
    js = stm_to_file(tc(jsfile), NULL);
    if (js != NULL)
    {
        i_js(site, js);
        if (listener != NULL)
            listener_event(listener, ekJS_CONTENT, site, js, NULL, WSite, Stream, void);
        stm_close(&js);
    }
    else
    {
        String *err = str_printf("[wsite]-Error creating '%s' dir", tc(jsfile));
        html5_status_err(status, &err);
    }

    str_destroy(&cssfile);
    str_destroy(&jsfile);
    str_destroy(&sitemap);

    if (site->head == NULL)
    {
        String *err = str_printf("[wsite]-'head' object not defined.");
        html5_status_err(status, &err);
    }
}

/*---------------------------------------------------------------------------*/

void wsite_end_lang(WSite *site)
{
    cassert_no_null(site);
    stm_close(&site->sitemap);
}

/*---------------------------------------------------------------------------*/

void wsite_begin_pack(WSite *site, const char_t *folder, Html5Status *status)
{
    String *curpath;
    cassert_no_null(site);
    cassert(site->level > 0);
    curpath = str_cpath("%s/%s", tc(site->curpath), folder);
    str_destroy(&site->curpath);
    str_destroy(&site->curfolder);
    str_destroy(&site->webimg);
    site->curpath = curpath;
    site->curfolder = str_c(folder);
    site->webimg = str_cpath("%s/img/%s", tc(site->webpath), folder);
    site->level += 1;

    if (hfile_dir_create(tc(site->curpath), NULL) == FALSE)
    {
        String *err = str_printf("[wsite]-Error creating '%s' dir", tc(site->curpath));
        html5_status_err(status, &err);
    }

    if (hfile_dir_create(tc(site->webimg), NULL) == FALSE)
    {
        String *err = str_printf("[wsite]-Error creating '%s' dir", tc(site->webimg));
        html5_status_err(status, &err);
    }
}

/*---------------------------------------------------------------------------*/

void wsite_end_pack(WSite *site)
{
    String *curpath;
    cassert_no_null(site);
    cassert(site->level > 1);
    str_split_pathname(tc(site->curpath), &curpath, NULL);
    str_destroy(&site->curpath);
    site->curpath = curpath;
    site->level -= 1;
}

/*---------------------------------------------------------------------------*/

void wsite_copy_landing(WSite *site, const char_t *landing_src, const char_t **except, const uint32_t except_size)
{
    cassert_no_null(site);

    /* Synchro landing content (by lang) */
    {
        const String *lang = arrpt_get(site->langs, site->langid, String);
        String *src = str_cpath("%s/%s", landing_src, tc(lang));
        String *dest = str_cpath("%s/%s", tc(site->curpath), "web");
        hfile_dir_sync(tc(src), tc(dest), TRUE, TRUE, except, except_size, NULL);
        str_destroy(&src);
        str_destroy(&dest);
    }

    /* Synchro landing resources */
    {
        String *src = str_cpath("%s/%s", landing_src, "res");
        String *dest = str_cpath("%s/%s", tc(site->curpath), "res");
        hfile_dir_sync(tc(src), tc(dest), TRUE, TRUE, except, except_size, NULL);
        str_destroy(&src);
        str_destroy(&dest);
    }
}

/*---------------------------------------------------------------------------*/

void wsite_begin_page(WSite *site, const char_t *name, const uint32_t menu_id, Listener *listener, Html5Status *status)
{
    String *html_file;
    String *url;
    Stream *html;
    cassert_no_null(site);
    html_file = str_cpath("%s/%s.html", tc(site->curpath), name);
    url = str_printf("%s/%s/%s/%s.html", tc(site->prjurl), i_lang(site), tc(site->curfolder), name);
    str_writef(site->sitemap, url);
    stm_writef(site->sitemap, "\n");

    html = stm_to_file(tc(html_file), NULL);
    if (html != NULL)
    {
        stm_writef(html, "<html>\n");

        {
            uint32_t l;
            String *ppath;
            l = str_len_c(tc(site->webpath));
            ppath = str_c(tc(html_file) + l);
            str_subs(ppath, '\\', '/');
            stm_printf(html, "<!-- File: %s -->\n", tc(ppath));
            str_destroy(&ppath);
        }

        arrpt_foreach(comment, site->comments, String)
            stm_printf(html, "<!-- %s -->\n", tc(comment));
        arrpt_end()

        {
            Date date;
            String *sdate;
            btime_date(&date);
            sdate = date_format(&date, "%d/%m/%Y-%H:%M:%S");
            stm_printf(html, "<!-- %s -->\n", tc(sdate));
            str_destroy(&sdate);
        }

        if (site->head != NULL)
            head_html(site->head, html, listener);

        /* cassert_no_null(body); */
        stm_writef(html, "\n<body>\n");

        if (site->header != NULL)
            header_html(site->header, site->langs, html, listener);

        if (site->nav != NULL)
            nav_html(site->nav, menu_id, html);

        stm_writef(html, "\n<div class='body-div'>\n");

        if (site->lnav != NULL)
            lnav_html(site->lnav, html, listener);

        if (site->post != NULL)
            post_html(site->post, html, listener);

        if (site->rcol != NULL)
            rcol_html(site->rcol, html, listener);

        stm_writef(html, "</div> <!-- body-div -->\n");
        stm_writef(html, "\n<script src='");
        wsite_stm_res(site, html);
        stm_printf(html, "code_%s.js'></script>\n", wsite_lang(site));
        stm_writef(html, "\n</body>\n");
        /* body_html(site->body, site->header, site->nav, html, listener); */
        stm_writef(html, "\n</html>\n");
        stm_close(&html);
    }
    else
    {
        String *err = str_printf("[wsite]-Error creating '%s'.", tc(html_file));
        html5_status_err(status, &err);
    }

    str_destroy(&html_file);
    str_destroy(&url);
}

/*---------------------------------------------------------------------------*/

Header *wsite_gheader(WSite *site)
{
    cassert_no_null(site);
    return site->header;
}

/*---------------------------------------------------------------------------*/

Nav *wsite_get_nav(WSite *site)
{
    cassert_no_null(site);
    return site->nav;
}

/*---------------------------------------------------------------------------*/

Post *wsite_get_post(WSite *site)
{
    cassert_no_null(site);
    return site->post;
}

/*---------------------------------------------------------------------------*/

RCol *wsite_get_rcol(WSite *site)
{
    cassert_no_null(site);
    return site->rcol;
}

/*---------------------------------------------------------------------------*/

SCode *wsite_get_scode(WSite *site)
{
    cassert_no_null(site);
    return site->scode;
}

/*---------------------------------------------------------------------------*/

void wsite_add_res(WSite *site, const char_t *resname, Html5Status *status)
{
    String *src_res;
    String *des_res;
    cassert_no_null(site);
    src_res = str_cpath("%s/%s", tc(site->respath), resname);
    des_res = str_cpath("%s/%s", tc(site->webres), resname);
    if (hfile_exists(tc(src_res), NULL) == TRUE)
    {
        if (hfile_is_uptodate(tc(src_res), tc(des_res)) == FALSE)
        {
            if (hfile_copy(tc(src_res), tc(des_res), NULL) == FALSE)
            {
                String *err = str_printf("[wsite]-Error copying resource '%s'", resname);
                html5_status_err(status, &err);
            }
        }
    }
    else
    {
        String *err = str_printf("[wsite]-Resource doesn't exists '%s'", resname);
        html5_status_err(status, &err);
    }

    str_destroy(&src_res);
    str_destroy(&des_res);
}

/*---------------------------------------------------------------------------*/

static String *i_load_res(const WSite *site, const char_t *resname)
{
    String *src_res;
    String *buffer;
    cassert_no_null(site);
    src_res = str_cpath("%s/%s", tc(site->respath), resname);
    buffer = hfile_string(tc(src_res), NULL);
    str_destroy(&src_res);
    return buffer;
}

/*---------------------------------------------------------------------------*/

static bool_t i_optimize_png(const char_t *src)
{
    String *str = NULL;
    Proc *proc = NULL;
    bool_t ok = FALSE;
    str = str_printf("pngquant %s --force --ext .png", src);
    proc = bproc_exec(tc(str), NULL);
    if (proc != NULL)
    {
        bproc_wait(proc);
        bproc_close(&proc);
        ok = TRUE;
    }
    str_destroy(&str);
    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_clip_png(const char_t *src)
{
    Image *image = image_from_file(src, NULL);
    if (image != NULL)
    {
        Pixbuf *pixels = image_pixels(image, ekFIMAGE);
        uint32_t w = pixbuf_width(pixels);
        uint32_t h = pixbuf_height(pixels);
        uint32_t i, j;
        uint32_t minx = UINT32_MAX;
        uint32_t miny = UINT32_MAX;
        uint32_t maxx = 0;
        uint32_t maxy = 0;
        cassert(pixbuf_format(pixels) == ekRGBA32);

        for (j = 0; j < h; ++j)
            for (i = 0; i < w; ++i)
            {
                uint32_t v = pixbuf_get(pixels, i, j);

                if ((uint8_t)(v >> 24) != 0)
                {
                    if (i < minx)
                        minx = i;
                    if (j < miny)
                        miny = j;
                    if (i > maxx)
                        maxx = i;
                    if (j > maxy)
                        maxy = j;
                }
            }

        if (minx != UINT32_MAX)
        {
            Pixbuf *clip = pixbuf_trim(pixels, 0, miny, w, maxy - miny + 1);
            Image *nimage = image_from_pixbuf(clip, NULL);
            image_to_file(nimage, src, NULL);
            image_destroy(&nimage);
            pixbuf_destroy(&clip);
        }

        pixbuf_destroy(&pixels);
        image_destroy(&image);
        return TRUE;
    }

    return FALSE;
}

/*---------------------------------------------------------------------------*/

static bool_t i_svg_to_png(const char_t *src, const char_t *dest, const uint32_t width, const uint32_t dpi)
{
    String *str = NULL;
    Proc *proc = NULL;
    bool_t ok = FALSE;

    if (dpi == UINT32_MAX)
        str = str_printf("inkscape %s --export-png=%s --export-width=%d --without-gui", src, dest, width);
    else
        str = str_printf("inkscape %s --export-png=%s --export-dpi=%d --without-gui", src, dest, dpi);

    proc = bproc_exec(tc(str), NULL);
    if (proc != NULL)
    {
        bproc_wait(proc);
        bproc_close(&proc);
        ok = TRUE;
    }
    str_destroy(&str);
    return ok;
}

/*---------------------------------------------------------------------------*/

void wsite_add_icon(WSite *site, const char_t *resname, const char_t *outname, const color_t color, const uint32_t width, Html5Status *status)
{
    uint8_t r, g, b;
    String *data = i_load_res(site, resname);
    String *temp = hfile_appdata("temp.svg");
    String *des_res = str_cpath("%s/%s", tc(site->webres), outname);
    String *col;
    bool_t ok = TRUE;
    color_get_rgb(color, &r, &g, &b);
    col = str_printf("#%02X%02X%02X", r, g, b);
    str_repl_c((char_t *)tc(data), "#FFFFFF", tc(col));
    ok &= hfile_from_string(tc(temp), data, NULL);
    ok &= i_svg_to_png(tc(temp), tc(des_res), width, UINT32_MAX);

    if (ok == FALSE)
    {
        String *err = str_printf("[wsite]-Error processing icon '%s'", resname);
        html5_status_err(status, &err);
    }

    str_destroy(&data);
    str_destroy(&temp);
    str_destroy(&des_res);
    str_destroy(&col);
}

/*---------------------------------------------------------------------------*/

void wsite_stm_res(const WSite *site, Stream *html)
{
    uint32_t i = 0;
    cassert_no_null(site);
    for (i = 0; i < site->level; ++i)
        stm_writef(html, "../");
    stm_writef(html, "res/");
}

/*---------------------------------------------------------------------------*/

uint32_t wsite_level(const WSite *site)
{
    cassert_no_null(site);
    return site->level;
}

/*---------------------------------------------------------------------------*/

real32_t wsite_header_height(const WSite *site)
{
    cassert_no_null(site);
    if (site->header != NULL)
        return header_height(site->header);
    else
        return 0.f;
}

/*---------------------------------------------------------------------------*/

real32_t wsite_nav_height(const WSite *site)
{
    cassert_no_null(site);
    if (site->nav != NULL)
        return nav_height(site->nav);
    else
        return 0.f;
}

/*---------------------------------------------------------------------------*/

real32_t wsite_lnav_width(const WSite *site)
{
    cassert_no_null(site);
    if (site->lnav != NULL)
        return lnav_width(site->lnav);
    return 0.f;
}

/*---------------------------------------------------------------------------*/

real32_t wsite_post_width(const WSite *site)
{
    cassert_no_null(site);
    if (site->post != NULL)
        return post_width(site->post);
    return 0.f;
}

/*---------------------------------------------------------------------------*/

real32_t wsite_rcol_width(const WSite *site)
{
    cassert_no_null(site);
    if (site->rcol != NULL)
        return rcol_width(site->rcol);
    return 0.f;
}

/*---------------------------------------------------------------------------*/

Image *wsite_image(const WSite *site, const char_t *resname, Html5Status *status)
{
    String *respath;
    Image *img;
    cassert_no_null(site);
    respath = str_cpath("%s/%s", tc(site->respath), resname);
    img = image_from_file(tc(respath), NULL);

    if (img == NULL)
    {
        String *err = str_printf("[wsite]-Error loading resource '%s'", resname);
        html5_status_err(status, &err);
    }

    str_destroy(&respath);
    return img;
}

/*---------------------------------------------------------------------------*/

static uint32_t i_img_width(const char_t *pathname)
{
    /* OPTIMIZAR: Obtener el tama�o de imagen si cargarla */
    Image *img = image_from_file(pathname, NULL);
    if (img != NULL)
    {
        uint32_t width = image_width(img);
        image_destroy(&img);
        return width;
    }
    return UINT32_MAX;
}

/*---------------------------------------------------------------------------*/

static bool_t i_img_is_uptodate(const char_t *src, const char_t *dest, const uint32_t width)
{
    if (hfile_is_uptodate(src, dest) == FALSE)
        return FALSE;
    return (bool_t)(i_img_width(dest) == width);
}

/*---------------------------------------------------------------------------*/

static bool_t i_copy_bitmap(const char_t *src, const char_t *dest, const char_t *imgext, const uint32_t max_width, uint32_t *width)
{
    bool_t ok = TRUE;
    Image *imgfrom = image_from_file(src, NULL);
    if (imgfrom)
    {
        uint32_t req_width, img_width = image_width(imgfrom);
        req_width = img_width;
        if (req_width > max_width)
            req_width = max_width;

        *width = req_width;
        if (i_img_is_uptodate(src, dest, req_width) == FALSE)
        {
            Image *imgto;
            if (img_width == req_width)
                imgto = image_copy(imgfrom);
            else
                imgto = image_scale(imgfrom, req_width, UINT32_MAX);

            ok = image_to_file(imgto, dest, NULL);

            if (ok && str_equ_c(imgext, "png") == TRUE)
                ok = i_optimize_png(dest);

            image_destroy(&imgto);
        }

        image_destroy(&imgfrom);
    }
    else
    {
        ok = FALSE;
    }

    return ok;
}

/*---------------------------------------------------------------------------*/

String *wsite_optimize_img(const WSite *site, const char_t *imgpath, const uint32_t max_width, uint32_t *width, Html5Status *status)
{
    String *imgname;
    String *imgext;
    String *imgbase = NULL;
    cassert_no_null(site);
    cassert_no_null(width);
    str_split_pathext(imgpath, NULL, &imgname, &imgext);
    str_lower(imgext);

    /* Convert svg to png for the web */
    if (str_equ_c(tc(imgext), "svg") == TRUE)
    {
        String *dest = str_cpath("%s/%s.png", tc(site->webimg), tc(imgname));
        bool_t ok = TRUE;

        if (i_img_is_uptodate(imgpath, tc(dest), max_width) == FALSE)
        {
            i_svg_to_png(imgpath, tc(dest), max_width, UINT32_MAX);
            if (ok)
                ok = i_optimize_png(tc(dest));
        }

        str_destroy(&dest);
        imgbase = ok ? str_printf("%s.png", tc(imgname)) : NULL;
        *width = max_width;
    }

    /* png / jpg */
    else if (str_equ_c(tc(imgext), "png") == TRUE || str_equ_c(tc(imgext), "jpg") == TRUE)
    {
        String *dest = str_cpath("%s/%s.%s", tc(site->webimg), tc(imgname), tc(imgext));
        bool_t ok = i_copy_bitmap(imgpath, tc(dest), tc(imgext), max_width, width);
        str_destroy(&dest);
        imgbase = ok ? str_printf("%s.%s", tc(imgname), tc(imgext)) : NULL;
    }

    /* gif */
    else if (str_equ_c(tc(imgext), "gif") == TRUE)
    {
        String *dest = str_cpath("%s/%s.%s", tc(site->webimg), tc(imgname), tc(imgext));
        Image *image = image_from_file(imgpath, NULL);
        if (hfile_is_uptodate(imgpath, tc(dest)) == FALSE)
            hfile_copy(imgpath, tc(dest), NULL);
        str_destroy(&dest);
        imgbase = str_printf("%s.gif", tc(imgname));
        if (image != NULL)
        {
            *width = image_width(image);
            image_destroy(&image);
        }
        else
        {
            *width = 200;
        }
    }

    else
    {
        String *err = str_printf("[wsite]-Unsupported image type '%s'", imgpath);
        html5_status_err(status, &err);
    }

    str_destroy(&imgname);
    str_destroy(&imgext);
    return imgbase;
}

/*---------------------------------------------------------------------------*/

static bool_t i_latex_to_png(const char_t *tempath, const char_t *webimg, const char_t *math_data, const uint32_t math_size, const char_t *name, const bool_t clip, Html5Status *status)
{
    String *texfile = str_cpath("%s/formula.tex", tempath);
    Stream *formula = stm_to_file(tc(texfile), NULL);
    bool_t ok = TRUE;

    if (formula != NULL)
    {
        stm_writef(formula, "\\documentclass[border=2pt]{standalone}\n");
        stm_writef(formula, "\\usepackage{amsmath}\n");
        stm_writef(formula, "\\usepackage{varwidth}\n");
        stm_writef(formula, "\\begin{document}\n");
        stm_writef(formula, "\\begin{varwidth}{\\linewidth}\n");
        /* stm_writef(formula, "\\begin{equation*}\n"); */
        stm_write(formula, (const byte_t *)math_data, math_size);
        /* stm_writef(formula, "\\end{equation*}\n"); */
        stm_writef(formula, "\\end{varwidth}\n");
        stm_writef(formula, "\\end{document}\n");
        stm_close(&formula);
    }
    else
    {
        String *err = str_printf("[wsite]-Error creating 'formula.tex' for '%s::%s'", webimg, name);
        html5_status_err(status, &err);
        ok = FALSE;
    }

    /* We have a 'formula.tex' file */
    if (ok == TRUE)
    {
        String *str = str_cpath("pdflatex -interaction=nonstopmode -output-directory=\"%s\" \"%s\"", tempath, tc(texfile));
        Proc *proc = bproc_exec(tc(str), NULL);
        if (proc != NULL)
        {
            uint32_t retcode = bproc_wait(proc);
            bproc_close(&proc);
            if (retcode != 0)
                ok = FALSE;
        }
        else
        {
            ok = FALSE;
        }

        str_destroy(&str);
    }

    /* We have a .pdf with the formula */
    if (ok == TRUE)
    {
        String *str = str_cpath("pdf2svg \"%s\\formula.pdf\" \"%s\\formula.svg\"", tempath, tempath);
        Proc *proc = bproc_exec(tc(str), NULL);
        if (proc != NULL)
        {
            byte_t ret[512];
            uint32_t retcode;
            while (bproc_eread(proc, ret, 512, NULL, NULL))
                bstd_write(ret, 512, NULL);

            retcode = bproc_wait(proc);
            bproc_close(&proc);
            if (retcode != 0)
                ok = FALSE;
        }
        else
        {
            ok = FALSE;
        }

        str_destroy(&str);
    }

    /* We have a .svg with the formula */
    if (ok == TRUE)
    {
        String *svgfile = str_cpath("%s/formula.svg", tempath);
        String *pngfile = str_cpath("%s/%s.png", webimg, name);

        ok = i_svg_to_png(tc(svgfile), tc(pngfile), UINT32_MAX, 144);
        if (ok == TRUE)
            ok = i_optimize_png(tc(pngfile));

        if (ok == TRUE && clip == TRUE)
            ok = i_clip_png(tc(pngfile));

        str_destroy(&svgfile);
        str_destroy(&pngfile);
    }

    str_destroy(&texfile);
    return ok;
}

/*---------------------------------------------------------------------------*/

String *wsite_math_img(const WSite *site, const char_t *math_data, const uint32_t math_size, const char_t *name, const bool_t clip_borders, uint32_t *width, Html5Status *status)
{
    String *mathimg = NULL;
    String *mathtxt = NULL;
    String *imgbase = NULL;
    bool_t update_formula = FALSE;

    cassert_no_null(site);
    mathimg = str_cpath("%s/%s.png", tc(site->webimg), name);
    mathtxt = str_cpath("%s/%s.txt", tc(site->webimg), name);

    /* Check if .png with formula already exists */
    if (hfile_exists(tc(mathimg), NULL) == FALSE)
    {
        update_formula = TRUE;
    }
    else
    {
        /* Check if .txt with formula already exists */
        Buffer *txt = hfile_buffer(tc(mathtxt), NULL);
        if (txt != NULL)
        {
            /* Check if formula has changed */
            if (buffer_size(txt) != math_size)
            {
                update_formula = TRUE;
            }
            else
            {
                const byte_t *txtdata = buffer_data(txt);
                if (str_equ_cn(math_data, (const char_t *)txtdata, math_size) == FALSE)
                    update_formula = TRUE;
            }

            buffer_destroy(&txt);
        }
        else
        {
            update_formula = TRUE;
        }
    }

    /* If no changes 'formula' is not recomputed again */
    if (update_formula == TRUE)
    {
        if (i_latex_to_png(tc(site->tempath), tc(site->webimg), math_data, math_size, name, clip_borders, status) == TRUE)
        {
            /* Save the .txt with the new formula */
            if (hfile_from_data(tc(mathtxt), (const byte_t *)math_data, math_size, NULL) == TRUE)
                imgbase = str_printf("%s.png", name);
        }
    }
    else
    {
        imgbase = str_printf("%s.png", name);
    }

    if (imgbase != NULL)
    {
        Image *image = image_from_file(tc(mathimg), NULL);
        *width = image_width(image);
        image_destroy(&image);
    }

    str_destroy(&mathimg);
    str_destroy(&mathtxt);
    return imgbase;
}

/*---------------------------------------------------------------------------*/

String *wsite_math_txt(const WSite *site, const char_t *name)
{
    cassert_no_null(site);
    return str_cpath("%s/%s.txt", tc(site->webimg), name);
}

/*---------------------------------------------------------------------------*/

const char_t *wsite_lang(const WSite *site)
{
    cassert_no_null(site);
    return i_lang(site);
}

/*---------------------------------------------------------------------------*/

const char_t *wsite_folder(const WSite *site)
{
    cassert_no_null(site);
    return tc(site->curfolder);
}

/*---------------------------------------------------------------------------*/

const char_t *wsite_prjname(const WSite *site)
{
    cassert_no_null(site);
    return tc(site->prjtitle);
}

/*---------------------------------------------------------------------------*/

const String *wsite_get_footer(const WSite *site)
{
    cassert_no_null(site);
    return site->footer;
}
