/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: scode.c
 *
 */

/* Html5 Code Syntax Highlighting */

#include "scode.h"
#include "scode.inl"
#include "css.h"
#include "wsite.inl"
#include <draw2d/color.h>
#include <core/event.h>
#include <core/heap.h>
#include <core/stream.h>
#include <core/strings.h>
#include <osbs/bproc.h>
#include <sewer/bmem.h>
#include <sewer/cassert.h>
#include <sewer/unicode.h>

struct _scode_t
{
    WSite *site;
    color_t clbkgnd;
    color_t cltext;
    color_t clline;
    color_t clfuncs;
    color_t cltypes;
    color_t clconsts;
    caption_t caption;
};

/*---------------------------------------------------------------------------*/

SCode *scode_create(
    const color_t clbkgnd,
    const color_t cltext,
    const color_t clline,
    const color_t clfuncs,
    const color_t cltypes,
    const color_t clconsts)
{
    SCode *code = heap_new0(SCode);
    code->clbkgnd = clbkgnd;
    code->cltext = cltext;
    code->clline = clline;
    code->clfuncs = clfuncs;
    code->cltypes = cltypes;
    code->clconsts = clconsts;
    return code;
}

/*---------------------------------------------------------------------------*/

void scode_destroy(SCode **code)
{
    cassert_no_null(code);
    cassert_no_null(*code);
    heap_delete(code, SCode);
}

/*---------------------------------------------------------------------------*/

void scode_site(SCode *code, WSite *site)
{
    cassert_no_null(code);
    cassert(code->site == NULL);
    code->site = site;
}

/*---------------------------------------------------------------------------*/

void scode_css(const SCode *code, CSS *css)
{
    cassert_no_null(code);

    css_sel(css, ".code");
    css_str(css, "display", "block");
    /* css_px4(css, "margin", 15.f, 0.f, 15.f, 0.f); */
    css_px(css, "margin", 0.f);
    css_rgba(css, "border-color", code->clline);
    css_rgba(css, "color", color_rgb(64, 64, 64));
    css_px4(css, "border-width", 6.f, 1.f, 1.f, 1.f);
    css_str(css, "border-style", "solid");
    css_str(css, "border-collapse", "collapse");
    css_str(css, "max-width", "100%");
    css_str(css, "width", "auto");
    css_str(css, "overflow", "auto");
    css_sele(css);

    css_sel(css, ".code td");
    css_px(css, "padding", 0.f);
    css_rgba(css, "background-color", code->clbkgnd);
    css_sele(css);

    css_sel(css, ".code td:last-child");
    css_str(css, "width", "100%");
    css_sele(css);

    css_sel(css, ".code-lineno pre");
    css_px(css, "padding", 10.f);
    css_px(css, "margin-top", 0.f);
    css_str(css, "border-right", "1px solid");
    css_rgba(css, "border-right-color", code->clline);
    css_rgba(css, "background-color", code->clbkgnd);
    css_rgba(css, "color", code->clline);
    css_sele(css);

    css_sel(css, ".code-noline pre");
    css_px(css, "padding", 0.f);
    css_px(css, "margin", 0.f);
    css_sele(css);

    css_sel(css, ".k, .k a");
    css_rgba(css, "color", code->clline);
    css_str(css, "text-decoration", "none");
    css_sele(css);

    css_sel(css, ".cm");
    css_rgba(css, "color", color_rgb(0, 100, 0));
    css_sele(css);

    css_sel(css, ".f, .f a");
    css_rgba(css, "color", code->clfuncs);
    css_str(css, "text-decoration", "none");
    css_sele(css);

    css_sel(css, ".f a:hover");
    css_str(css, "text-decoration", "underline");
    /* css_str(css, "font-weight", "bold"); */
    css_sele(css);

    css_sel(css, ".t, .t a");
    css_rgba(css, "color", code->cltypes);
    css_str(css, "text-decoration", "none");
    css_sele(css);

    css_sel(css, ".t a:hover");
    css_str(css, "text-decoration", "underline");
    /* css_str(css, "font-weight", "bold"); */
    css_sele(css);

    css_sel(css, ".c, .c a");
    css_rgba(css, "color", code->clconsts);
    css_str(css, "text-decoration", "none");
    css_sele(css);

    css_sel(css, ".c a:hover");
    css_str(css, "text-decoration", "underline");
    /* css_str(css, "font-weight", "bold"); */
    css_sele(css);

    /* This style is for ANSI-to_HTML conversion with 'aha' tool */
    /* cat term_data | aha -s > ls-output.htm */
    css_direct(css, ".reset       {color: black;}");
    css_direct(css, ".bg-reset    {background-color: white;}");
    css_direct(css, ".inverted    {color: white;}");
    css_direct(css, ".bg-inverted {background-color: black;}");
    css_direct(css, ".dimgray     {color: dimgray;}");
    css_direct(css, ".red         {color: red;}");
    css_direct(css, ".green       {color: green;}");
    css_direct(css, ".yellow      {color: darkgoldenrod;}");
    css_direct(css, ".blue        {color: blue;}");
    css_direct(css, ".purple      {color: purple;}");
    css_direct(css, ".cyan        {color: steelblue;}");
    css_direct(css, ".white       {color: gray;}");
    css_direct(css, ".bg-black    {background-color: black;}");
    css_direct(css, ".bg-red      {background-color: red;}");
    css_direct(css, ".bg-green    {background-color: green;}");
    css_direct(css, ".bg-yellow   {background-color: darkgoldenrod;}");
    css_direct(css, ".bg-blue     {background-color: blue;}");
    css_direct(css, ".bg-purple   {background-color: purple;}");
    css_direct(css, ".bg-cyan     {background-color: steelblue;}");
    css_direct(css, ".bg-white    {background-color: gray;}");
    css_direct(css, ".underline   {text-decoration: underline;}");
    css_direct(css, ".bold        {font-weight: bold;}");
    css_direct(css, ".italic      {font-style: italic;}");
    css_direct(css, ".blink       {text-decoration: blink;}");
    css_direct(css, ".crossed-out {text-decoration: line-through;}");
    css_direct(css, ".highlighted {filter: brightness(100%);}");
}

/*---------------------------------------------------------------------------*/

void scode_js(const SCode *code, const jssec_t sec, Stream *js)
{
    unref(code);
    unref(sec);
    unref(js);
}

/*---------------------------------------------------------------------------*/

const char_t *scode_func_class(const SCode *code)
{
    unref(code);
    return "f";
}

/*---------------------------------------------------------------------------*/

const char_t *scode_type_class(const SCode *code)
{
    unref(code);
    return "t";
}

/*---------------------------------------------------------------------------*/

const char_t *scode_const_class(const SCode *code)
{
    unref(code);
    return "c";
}

/*---------------------------------------------------------------------------*/

void scode_ref(const SCode *code, const char_t *list_id, Stream *html)
{
    cassert_no_null(code);
    stm_writef(html, "<span class='post-dec'>");
    if (str_equ_c(wsite_lang(code->site), "es") == TRUE)
        stm_printf(html, "Listado %s", list_id);
    else
        stm_printf(html, "Listing %s", list_id);
    stm_writef(html, "</span>");
}

/*---------------------------------------------------------------------------*/

void scode_begin(const SCode *code, const caption_t caption, const char_t *list_id, Stream *html)
{
    cassert_no_null(code);
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
        stm_writef(html, "<summary class='top'>\n");
        break;

    case ekCAPTION_CLOSED:
        stm_writef(html, "\n<details>\n");
        stm_writef(html, "<summary class='top'>\n");
        break;
        cassert_default();
    }

    if (list_id != NULL)
    {
        stm_writef(html, "<span class='post-dec'>");
        if (str_equ_c(wsite_lang(code->site), "es") == TRUE)
            stm_printf(html, "Listado %s: ", list_id);
        else
            stm_printf(html, "Listing %s: ", list_id);
        stm_writef(html, "</span>");
    }

    cast(code, SCode)->caption = caption;
}

/*---------------------------------------------------------------------------*/

static void i_parse_cpp(Stream *html, const byte_t *data, const uint32_t size, Listener *listener)
{
    Stream *stm = stm_from_block(data, size);
    ltoken_t token = ekTUNDEF;
    const char_t *lexeme = NULL;

    stm_token_comments(stm, TRUE);
    stm_token_spaces(stm, TRUE);
    token = stm_read_token(stm);
    lexeme = stm_token_lexeme(stm, NULL);
    while (token != ekTEOF)
    {
        if (token == ekTIDENT)
        {
            Identifier result;
            bmem_zero(&result, Identifier);
            listener_event(listener, ekCODE_IDENTIFIER, lexeme, html, &result, char_t, Stream, Identifier);
            if (result.type != ekID_OTHER)
            {
                const char_t *idcl = "";
                switch (result.type)
                {
                case ekID_KEYWORD:
                    idcl = "k";
                    break;
                case ekID_TYPE:
                    idcl = "t";
                    break;
                case ekID_CONSTANT:
                    idcl = "c";
                    break;
                case ekID_FUNCTION:
                    idcl = "f";
                    break;
                case ekID_OTHER:
                    cassert_default();
                }

                stm_printf(html, "<span class='%s'>", idcl);
                if (result.has_link == TRUE)
                {
                    stm_writef(html, "<a href='");
                    listener_event(listener, ekCODE_LINK, lexeme, html, NULL, char_t, Stream, void);
                    stm_writef(html, "'");

                    if (result.has_title == TRUE)
                    {
                        stm_writef(html, " title='");
                        listener_event(listener, ekCODE_TITLE, lexeme, html, NULL, char_t, Stream, void);
                        stm_writef(html, "'");
                    }
                    stm_writef(html, ">");
                }
                stm_writef(html, lexeme);
                if (result.has_link == TRUE)
                    stm_writef(html, "</a>");
                stm_writef(html, "</span>");
            }
            else
            {
                stm_writef(html, lexeme);
            }
        }
        else if (token == ekTSLCOM || token == ekTMLCOM)
        {
            stm_printf(html, "<span class='cm'>%s</span>", lexeme);
        }
        else if (token == ekTPOUND)
        {
            stm_printf(html, "<span class='k'>%s</span>", lexeme);
        }
        else if (token == ekTSPACE)
        {
            stm_writef(html, lexeme);
        }
        else if (token == ekTLESS)
        {
            stm_writef(html, "&lt;");
        }
        else if (token == ekTGREAT)
        {
            stm_writef(html, "&gt;");
        }
        else
        {
            stm_writef(html, lexeme);
        }

        token = stm_read_token(stm);
        lexeme = stm_token_lexeme(stm, NULL);
    }

    stm_close(&stm);
}

/*---------------------------------------------------------------------------*/

static void i_parse_ansi(Stream *html, const byte_t *data, const uint32_t size)
{
    const char_t *cmd = "aha -s";
    Proc *proc = bproc_exec(cmd, NULL);
    if (proc != NULL)
    {
        uint32_t wsize = 0, rsize = 0;
        byte_t buffer[1024];
        Stream *stm = stm_memory(1024);
        const char_t *html_ansi = NULL;
        uint32_t html_size = UINT32_MAX;

        if (bproc_write(proc, data, size, &wsize, NULL) == TRUE)
        {
            cassert_unref(size == wsize, wsize);
            bproc_write_close(proc);
        }

        bproc_eread_close(proc);

        while (bproc_read(proc, buffer, sizeof(buffer), &rsize, NULL) == TRUE)
            stm_write(stm, buffer, rsize);

        /* Post-process 'aha' result stripping all 'html' headers */
        {
            const char_t *code = cast_const(stm_buffer(stm), char_t);
            html_ansi = str_str(code, "<pre>");
            if (html_ansi != NULL)
            {
                const char_t *end = NULL;
                html_ansi += str_len_c("<pre>") + 1;
                end = str_str(html_ansi, "</pre>");
                if (end != NULL)
                    html_size = (uint32_t)(end - html_ansi);
                else
                    html_ansi = NULL;
            }
        }

        if (html_ansi != NULL)
            stm_write(html, cast_const(html_ansi, byte_t), html_size);

        stm_close(&stm);
        bproc_close(&proc);
    }
}

/*---------------------------------------------------------------------------*/

static void i_parse_text(Stream *html, const byte_t *data, const uint32_t size)
{
    uint32_t i = 0, n, c;
    const char_t *str = (const char_t *)data;
    while (i < size)
    {
        c = unicode_to_u32b(str, ekUTF8, &n);
        i += n;
        if (c == '<')
            stm_writef(html, "&lt;");
        else if (c == '>')
            stm_writef(html, "&gt;");
        else
            stm_write_char(html, c);
        str = unicode_next(str, ekUTF8);
    }
}

/*---------------------------------------------------------------------------*/

void scode_html(const SCode *code, Stream *html, const char_t *lang, const uint32_t start_line, const byte_t *data, const uint32_t size, Listener *listener)
{
    cassert_no_null(code);

    switch (code->caption)
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

    stm_writef(html, "<table class='code'>\n");
    stm_writef(html, "<tr>\n");

    if (start_line != UINT32_MAX)
    {
        uint32_t i = 0, n = 0;
        const char_t *lformat;
        const char_t *tcode = (const char_t *)data;

        for (i = 0; i < size; ++i, ++tcode)
            if (*tcode == '\n')
                n++;

        if (n < 10)
            lformat = "%d\n";
        else if (n < 100)
            lformat = "%2d\n";
        else if (n < 1000)
            lformat = "%3d\n";
        else if (n < 10000)
            lformat = "%4d\n";
        else
            lformat = "%d\n";

        stm_writef(html, "<td>\n");
        stm_writef(html, "<div class='code-lineno'>\n");
        stm_writef(html, "<pre>\n");
        for (i = 0; i < n; ++i)
            stm_printf(html, lformat, i + start_line);
        stm_writef(html, "</pre>\n");
        stm_writef(html, "</div> <!-- code-lineno -->\n");
        stm_writef(html, "</td>\n");
    }
    else
    {
        stm_writef(html, "<td>\n");
        stm_writef(html, "<div class='code-noline'>\n");
        stm_writef(html, "<pre>\n");
        stm_writef(html, " \n");
        stm_writef(html, "</pre>\n");
        stm_writef(html, "</div> <!-- code-noline -->\n");
        stm_writef(html, "</td>\n");
    }

    stm_writef(html, "<td>\n");
    stm_writef(html, "<div class='code-hl'>\n");
    stm_writef(html, "<pre>\n");

    if (str_equ_c(lang, "cpp") == TRUE)
    {
        i_parse_cpp(html, data, size, listener);
    }
    else if (str_equ_c(lang, "ansi") == TRUE)
    {
        i_parse_ansi(html, data, size);
    }
    else
    {
        i_parse_text(html, data, size);
    }

    /*     if (tcode[size - 1] != '\n')
       stm_writef(html, "\n"); */

    stm_writef(html, "</pre>\n");
    stm_writef(html, "</div> <!-- code-hl -->\n");
    stm_writef(html, "</td>\n");
    stm_writef(html, "</tr>\n");
    stm_writef(html, "</table>\n");
    stm_writef(html, "</figure>\n");

    switch (code->caption)
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

    cast(code, SCode)->caption = ENUM_MAX(caption_t);
}
