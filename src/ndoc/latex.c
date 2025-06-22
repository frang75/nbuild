/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: latex.c
 *
 */

/* LaTeX generator */

#include "latex.h"
#include "doc.h"
#include "loader.h"
#include "nlog.h"
#include "res_ndoc.h"
#include <nlib/ssh.h>
#include <draw2d/image.h>
#include <core/arrpt.h>
#include <core/arrst.h>
#include <core/date.h>
#include <core/hfile.h>
#include <core/respack.h>
#include <core/stream.h>
#include <core/strings.h>
#include <osbs/bproc.h>
#include <osbs/btime.h>
#include <sewer/cassert.h>
#include <sewer/sewer.h>
#include <sewer/unicode.h>

/*---------------------------------------------------------------------------*/

static void i_copy_cfg_file(const char_t *docpath, const char_t *res_dir, const char_t *filename)
{
    String *str = str_cpath("%s/config/%s", docpath, filename);

    if (hfile_copy(tc(str), res_dir, NULL) == FALSE)
    {
        String *msg = str_printf("Copying '%s' file.", tc(str));
        nlog_error(&msg);
    }

    str_destroy(&str);
}

/*---------------------------------------------------------------------------*/

static ___INLINE const char_t *i_lang(const Config *config, const uint32_t lang_id)
{
    const Lang *lang = NULL;
    cassert_no_null(config);
    lang = arrst_get(config->langs, lang_id, Lang);
    return tc(lang->lang);
}

/*---------------------------------------------------------------------------*/

static ___INLINE const char_t *i_str(const ArrPt(String) *strs, const uint32_t lang_id)
{
    const String *str = arrpt_get_const(strs, lang_id, String);
    return tc(str);
}

/*---------------------------------------------------------------------------*/

void latex_tokens(const char_t *output_dir, const Config *config, const uint32_t lang_id, const Loader *loader)
{
    String *str = str_cpath("%s/ndoc_tokens_%s.tex", output_dir, i_lang(config, lang_id));
    Stream *latex = stm_to_file(tc(str), NULL);
    if (latex != NULL)
    {
        stm_writef(latex, "% .tex tokens\n");
        stm_printf(latex, "%% %s %s\n", tc(config->project_name), i_str(config->project_brief, lang_id));
        stm_writef(latex, "\n");
        stm_writef(latex, "\\lstset{\n");
        stm_writef(latex, "    basicstyle=\\footnotesize\\ttfamily,\n");
        stm_writef(latex, "    showstringspaces=false,\n");
        stm_writef(latex, "    backgroundcolor=\\color[rgb]{0.99,0.99,0.99},\n");
        stm_writef(latex, "    language=C++,\n");
        stm_writef(latex, "    numbers=none,\n");
        /* stm_writef(latex, "    numbers=left,\n");
        stm_writef(latex, "    numberstyle=\\scriptsize\\color{TITLE_COLOR},\n"); */
        stm_writef(latex, "    rulecolor=\\color{TITLE_COLOR},\n");
        stm_writef(latex, "    frame=tb,\n");
        stm_writef(latex, "    inputencoding=utf8,\n");
        stm_writef(latex, "    extendedchars=true,\n");
        stm_writef(latex, "    literate={á}{{\\'a}}1 {é}{{\\'e}}1 {í}{{\\'i}}1 {ó}{{\\'o}}1 {ú}{{\\'u}}1{Á}{{\\'A}}1 {É}{{\\'E}}1 {Í}{{\\'I}}1 {Ó}{{\\'O}}1 {Ú}{{\\'U}}1 {à}{{\\`a}}1 {è}{{\\`e}}1 {ì}{{\\`i}}1 {ò}{{\\`o}}1 {ù}{{\\`u}}1 {À}{{\\`A}}1 {È}{{\\'E}}1 {Ì}{{\\`I}}1 {Ò}{{\\`O}}1 {Ù}{{\\`U}}1 {ä}{{\\\"a}}1 {ë}{{\\\"e}}1 {ï}{{\\\"i}}1 {ö}{{\\\"o}}1 {ü}{{\\\"u}}1 {Ä}{{\\\"A}}1 {Ë}{{\\\"E}}1 {Ï}{{\\\"I}}1 {Ö}{{\\\"O}}1 {Ü}{{\\\"U}}1 {â}{{\\^a}}1 {ê}{{\\^e}}1 {î}{{\\^i}}1 {ô}{{\\^o}}1 {û}{{\\^u}}1 {Â}{{\\^A}}1 {Ê}{{\\^E}}1 {Î}{{\\^I}}1 {Ô}{{\\^O}}1 {Û}{{\\^U}}1 {œ}{{\\oe}}1 {Œ}{{\\OE}}1 {æ}{{\\ae}}1 {Æ}{{\\AE}}1 {ß}{{\\ss}}1 {ű}{{\\H{u}}}1 {Ű}{{\\H{U}}}1 {ő}{{\\H{o}}}1 {Ő}{{\\H{O}}}1 {ç}{{\\c c}}1 {Ç}{{\\c C}}1 {ø}{{\\o}}1 {å}{{\\r a}}1 {Å}{{\\r A}}1 {€}{\\euro}1 {£}{{\\pounds}}1 {«}{{\\guillemotleft}}1 {»}{{\\guillemotright}}1 {ñ}{{\\~n}}1 {Ñ}{{\\~N}}1 {¿}{{?`}}1,\n");
        stm_writef(latex, "    breaklines=true,\n");
        stm_writef(latex, "    postbreak=\\mbox{\\textcolor{red}{$\\hookrightarrow$}\\space},\n");
        stm_writef(latex, "    keywordstyle=\\color{TITLE_COLOR}\\bfseries,\n");
        stm_writef(latex, "    commentstyle=\\itshape\\color[rgb]{0,0.3,0},\n");
        stm_writef(latex, "    emphstyle=\\color{FUNCS_COLOR},\n");
        stm_writef(latex, "    emphstyle={[2]\\color{TYPES_COLOR}},\n");
        stm_writef(latex, "    emphstyle={[3]\\color{CONSTANT_COLOR}},\n");
        stm_writef(latex, "    emph={\n");
        loader_funtions_to_latex(loader, latex);
        stm_writef(latex, "         },\n");
        stm_writef(latex, "    emph={[2]\n");
        loader_types_to_latex(loader, latex);
        stm_writef(latex, "         },\n");
        stm_writef(latex, "    emph={[3]\n");
        loader_constants_to_latex(loader, latex);
        stm_writef(latex, "         }\n");
        stm_writef(latex, "}\n");
        stm_close(&latex);
    }
    else
    {
        String *msg = str_printf("Creating '%s' file.", tc(str));
        nlog_error(&msg);
    }

    str_destroy(&str);
}

/*---------------------------------------------------------------------------*/

static const char_t *i_month(const ResPack *pack, const uint8_t month)
{
    switch (month)
    {
    case ekJANUARY:
        return respack_text(pack, TEXT_04);
    case ekFEBRUARY:
        return respack_text(pack, TEXT_05);
    case ekMARCH:
        return respack_text(pack, TEXT_06);
    case ekAPRIL:
        return respack_text(pack, TEXT_07);
    case ekMAY:
        return respack_text(pack, TEXT_08);
    case ekJUNE:
        return respack_text(pack, TEXT_09);
    case ekJULY:
        return respack_text(pack, TEXT_10);
    case ekAUGUST:
        return respack_text(pack, TEXT_11);
    case ekSEPTEMBER:
        return respack_text(pack, TEXT_12);
    case ekOCTOBER:
        return respack_text(pack, TEXT_13);
    case ekNOVEMBER:
        return respack_text(pack, TEXT_14);
    case ekDECEMBER:
        return respack_text(pack, TEXT_15);
        cassert_default();
    }
    return "";
}

/*---------------------------------------------------------------------------*/

bool_t latex_preamble(Stream *latex, const char_t *project_vers, const Config *config, const uint32_t lang_id, const ResPack *pack, const char_t *docpath, const char_t *res_dir)
{
    Date date;
    String *sdate;
    const byte_t *pdata;
    uint32_t psize;
    const char_t *logo = "logo.png";
    cassert_no_null(config);
    i_copy_cfg_file(docpath, res_dir, logo);
    btime_date(&date);
    stm_writef(latex, "% .tex ");
    stm_printf(latex, "%s-v%s %s\n", tc(config->project_name), project_vers, i_str(config->project_brief, lang_id));
    stm_printf(latex, "%% %s\n", respack_text(pack, TEXT_01));
    stm_printf(latex, "%% © %d-%d %s (%s)\n", config->project_start_year, date.year, tc(config->project_author), tc(config->project_email));
    stm_writef(latex, "% ");
    sdate = date_format(&date, "%d/%m/%Y-%H:%M:%S");
    str_writef(latex, sdate);
    str_destroy(&sdate);
    stm_writef(latex, "\n");
    stm_writef(latex, "\n");
    pdata = respack_file(pack, PREAMBLE_TEX, &psize);
    stm_write(latex, pdata, psize);

    stm_writef(latex, "% Source Code & Captions\n");
    stm_writef(latex, "\\usepackage{listings}\n");
    stm_printf(latex, "\\input{ndoc_tokens_%s.tex}\n", i_lang(config, lang_id));
    stm_writef(latex, "\\renewcommand{\\lstlistingname}{");
    stm_writef(latex, respack_text(pack, TEXT_18));
    stm_writef(latex, "}\n");

    stm_writef(latex, "\\def\\codeline{\\lstinline[basicstyle=\\ttfamily\\small,postbreak={},keywordstyle={}]}\n");
    stm_writef(latex, "\\usepackage[footnotesize,labelfont={color=TITLE_COLOR,bf},figurename=");
    stm_writef(latex, respack_text(pack, TEXT_19));
    stm_writef(latex, ",tablename=");
    stm_writef(latex, respack_text(pack, TEXT_20));
    stm_writef(latex, "]{ caption }\n");
    stm_writef(latex, "\n");

    stm_writef(latex, "\\DeclareCaptionType{equcaption}[");
    stm_writef(latex, respack_text(pack, TEXT_21));
    stm_writef(latex, "]\n");

    stm_writef(latex, "% Custom colors\n");
    stm_printf(latex, "\\definecolor{TITLE_COLOR}{HTML}{%s}\n", tc(config->ebook_section_color));
    stm_printf(latex, "\\definecolor{BQBK_COLOR}{HTML}{%s}\n", tc(config->ebook_bqback_color));
    stm_printf(latex, "\\definecolor{BQLI_COLOR}{HTML}{%s}\n", tc(config->ebook_bqline_color));
    stm_printf(latex, "\\definecolor{FUNCS_COLOR}{HTML}{%s}\n", tc(config->ebook_funcs_color));
    stm_printf(latex, "\\definecolor{TYPES_COLOR}{HTML}{%s}\n", tc(config->ebook_types_color));
    stm_printf(latex, "\\definecolor{CONSTANT_COLOR}{HTML}{%s}\n", tc(config->ebook_constants_color));
    stm_printf(latex, "\\definecolor{GRAY_COLOR}{HTML}{%s}\n", "F1F1F1");
    stm_writef(latex, "\n");

    stm_writef(latex, "% Cover page\n");
    stm_writef(latex, "\\pretitle{\n");
    stm_writef(latex, "\\begin{center}\n");
    stm_writef(latex, "\\LARGE\n");
    stm_printf(latex, "\\includegraphics[width=0.3\\textwidth]{./res/%s}\n", logo);
    stm_writef(latex, "\\\\[\\medskipamount]}\n");
    stm_writef(latex, "\\posttitle{\\end{center}\n");
    stm_writef(latex, "}\n");

    stm_printf(latex, "\\title{%s\\\\\n", i_str(config->ebook_title, lang_id));
    stm_printf(latex, "\\large{%s", i_str(config->ebook_subtitle, lang_id));
    stm_printf(latex, "\\\\%s %s}\n", respack_text(pack, TEXT_23), project_vers);
    stm_writef(latex, "\\thanks{");
    stm_writef(latex, respack_text(pack, TEXT_22) /*, tc(config->project_url)*/);
    stm_writef(latex, "}}");

    stm_writef(latex, "\\author{© ");
    if (config->project_start_year != (uint32_t)date.year)
        stm_printf(latex, "%d-%d ", config->project_start_year, date.year);
    else
        stm_printf(latex, "%d ", date.year);

    stm_writef(latex, tc(config->project_author));
    stm_printf(latex, "\\thanks{\\small{%s %s}}", i_str(config->project_license, lang_id), i_str(config->project_license_ebook, lang_id));
    stm_writef(latex, "\\\\");
    stm_writef(latex, tc(config->project_email));
    if (config->project_url != NULL)
    {
        stm_writef(latex, "\\\\");
        stm_writef(latex, tc(config->project_url));
    }
    stm_writef(latex, "}\n");
    stm_printf(latex, "\\date{%d %s %d}\n", date.mday, i_month(pack, date.month), date.year);
    return TRUE;
}

/*---------------------------------------------------------------------------*/

bool_t latex_cover(Stream *latex, const Config *config, const uint32_t lang_id, const char_t *docpath, const char_t *res_dir)
{
    String *cover = str_printf("cover_%s.pdf", i_lang(config, lang_id));
    i_copy_cfg_file(docpath, res_dir, tc(cover));
    stm_printf(latex, "\\includepdf[pages={1}]{./res/%s}\n", tc(cover));
    str_destroy(&cover);
    return TRUE;
}

/*---------------------------------------------------------------------------*/

static void i_latex_warn(String **str, DocParser *parser)
{
    String *msg = NULL;
    cassert_no_null(str);
    cassert_no_null(parser);
    msg = str_printf("[latex:%s:%s-%d]-%s.", i_lang(parser->config, parser->lang_id), doc_name(parser->doc), parser->line, tc(*str));
    nlog_warn(&msg);
    str_destroy(str);
}

/*---------------------------------------------------------------------------*/

static void i_lcode(Stream *latex, const char_t *text, const bool_t open_codeline)
{
    /* stm_writef(latex, text); */
    uint32_t code = unicode_to_u32(text, ekUTF8);
    while (code != 0)
    {
        switch (code)
        {
        case '$':
            /* stm_writef(latex, "\\$"); */
            break;
            /*        case '%':
            stm_writef(latex, "\%");
            break;*/

        case '\"':
            if (open_codeline == TRUE)
                stm_writef(latex, "\\\"");
            else
                stm_writef(latex, "\"");
            break;

        case '#':
            if (open_codeline == TRUE)
                stm_writef(latex, "\\#");
            else
                stm_writef(latex, "#");
            break;

        case '{':
            if (open_codeline == TRUE)
                stm_writef(latex, "\\{");
            else
                stm_writef(latex, "{");
            break;

        case '}':
            if (open_codeline == TRUE)
                stm_writef(latex, "\\}");
            else
                stm_writef(latex, "}");
            break;

        default:
            stm_write_char(latex, code);
        }

        text = unicode_next(text, ekUTF8);
        code = unicode_to_u32(text, ekUTF8);
    }
}

/*---------------------------------------------------------------------------*/

static void i_lwrite(Stream *latex, const char_t *text)
{
    uint32_t code = unicode_to_u32(text, ekUTF8);
    bool_t open_quotes = FALSE;
    while (code != 0)
    {
        switch (code)
        {
        case '\\':
            stm_writef(latex, "\\textbackslash ");
            break;
        case '$':
            stm_writef(latex, "\\$");
            break;
        case '%':
            stm_writef(latex, "\\%");
            break;
        case '#':
            stm_writef(latex, "\\#");
            break;
        case '&':
            stm_writef(latex, "\\&");
            break;
        case '_':
            stm_writef(latex, "\\_");
            break;
        case '{':
            stm_writef(latex, "\\{");
            break;
        case '}':
            stm_writef(latex, "\\}");
            break;
        case '\"':
            if (open_quotes == FALSE)
                stm_writef(latex, "``");
            else
                stm_writef(latex, "''");
            open_quotes = !open_quotes;
            break;
        default:
            stm_write_char(latex, code);
        }

        text = unicode_next(text, ekUTF8);
        code = unicode_to_u32(text, ekUTF8);
    }
}

/*---------------------------------------------------------------------------*/

static void i_latex_tags(Stream *latex, const ArrSt(Tag) *tags, const bool_t intabular, const bool_t in_bq, DocParser *parser)
{
    bool_t inmath = FALSE;
    bool_t incode = FALSE;
    const Tag *open_link = NULL;
    cassert_no_null(parser);
    arrst_foreach_const(tag, tags, Tag)
        switch (tag->type)
        {

        case ekPLAINTEXT:
            if (inmath == TRUE)
            {
                str_writef(latex, tag->text);
            }
            else
            {
                if (incode == TRUE)
                    i_lcode(latex, tc(tag->text), in_bq);
                else
                    i_lwrite(latex, tc(tag->text));
            }
            break;

        case ekLESS:
            stm_writef(latex, incode ? "<" : "$<$");
            break;

        case ekGREATER:
            stm_writef(latex, incode ? ">" : "$>$");
            break;

        case ekBOLD_OPEN:
            stm_writef(latex, "\\textbf{");
            break;

        case ekITALIC_OPEN:
            stm_writef(latex, "\\textit{");
            break;

        case ekCODE_OPEN:
            incode = TRUE;
            if (intabular == TRUE)
                stm_writef(latex, "\\lstinline[basicstyle=\\ttfamily\\small,postbreak={}]$");
            else
                stm_writef(latex, "\\codeline!");
            break;

        case ekUNDER_OPEN:
            stm_writef(latex, "\\underline{");
            break;

        case ekSTRIKE_OPEN:
            stm_writef(latex, "\\st{");
            break;

        case ekMATH_OPEN:
            inmath = TRUE;
            stm_writef(latex, "$");
            break;

        case ekMATH_CLOSE:
            inmath = FALSE;
            stm_writef(latex, "$");
            break;

        case ekBOLD_CLOSE:
        case ekITALIC_CLOSE:
        case ekUNDER_CLOSE:
        case ekSTRIKE_CLOSE:
            stm_writef(latex, "}");
            break;

        case ekCODE_CLOSE:
            if (intabular == TRUE)
                stm_writef(latex, "$");
            else
                stm_writef(latex, "!");
            incode = FALSE;
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
            if (refid != NULL)
            {
                if (reftype == ekIMG || reftype == ekIMG2)
                    stm_printf(latex, " (\\autoref{%s::img%s})", doc_name(parser->doc), refid);
                else if (reftype == ekCODE)
                    stm_printf(latex, " (\\autoref{%s::list%s})", doc_name(parser->doc), refid);
                else if (reftype == ekTABLE)
                    stm_printf(latex, " (\\autoref{%s::table%s})", doc_name(parser->doc), refid);
                else if (reftype == ekMATH)
                    stm_printf(latex, " (\\autoref{%s::equ%s})", doc_name(parser->doc), refid);
            }
            else
            {
                String *warn = str_printf("Reference '%s::%s' not found", doc_name(parser->doc), tc(tag->text));
                stm_writef(latex, " (\\autoref{UNKNOWN})");
                i_latex_warn(&warn, parser);
            }
            break;
        }

        case ekREF_CLOSE:
            break;

        case ekLINK_OPEN:
            open_link = tag;
            break;

        case ekLINK_CLOSE:
            if (open_link != NULL)
            {
                stm_writef(latex, "\\footnote{\\url{");
                i_lwrite(latex, tc(open_link->text));
                stm_writef(latex, "}}");
                open_link = NULL;
            }
            break;

        case ekLFUNC_OPEN:
        {
            /* const Block *block;
            const Doc *doc = loader_func(parser->loader, tc(tag->text), &block); */
            stm_writef(latex, "\\lstinline[basicstyle=\\ttfamily\\small,postbreak={}]$");
            stm_writef(latex, tc(tag->text));
            stm_writef(latex, "$");
            /*if (doc != NULL)
            {
                stm_printf(latex, "$ (\\autopageref{%s::%s})", doc_name(doc), tc(block->ref));
            }
            else
            {
                String *warn = str_printf("Referenced function '%s' not found", tc(tag->text));
                i_latex_warn(&warn, parser);
                stm_writef(latex, "$ (\\autopageref{Unknown})");
            }*/
            break;
        }

        case ekLFUNC_CLOSE:
            break;

        case ekLTYPE_OPEN:
        {
            /* const Block *block;
            const Doc *doc = loader_type(parser->loader, tc(tag->text), &block); */
            stm_writef(latex, "\\lstinline[basicstyle=\\ttfamily\\small,postbreak={}]$");
            stm_writef(latex, tc(tag->text));
            stm_writef(latex, "$");
            /*if (doc != NULL)
            {
                stm_printf(latex, "$ (\\autopageref{%s::%s})", doc_name(doc), tc(block->ref));
            }
            else
            {
                String *warn = str_printf("Referenced type '%s' not found", tc(tag->text));
                i_latex_warn(&warn, parser);
                stm_writef(latex, "$ (\\autopageref{Unknown})");
            }*/
            break;
        }

        case ekLTYPE_CLOSE:
            break;

        case ekLPAGE_OPEN:
        {
            const Doc *doc = loader_doc(parser->loader, tc(tag->text));
            open_link = tag;
            stm_printf(latex, "\\textit{\\hyperref[%s]{``", tc(tag->text));
            if (doc != NULL)
            {
                const Block *h1 = doc_h1(doc);
                if (h1->type == ekH1)
                    i_latex_tags(latex, h1->tags, FALSE, FALSE, parser);
            }
            else
            {
                String *warn = str_printf("Referenced document '%s' not found", tc(tag->text));
                i_latex_warn(&warn, parser);
                i_lwrite(latex, tc(tag->text));
            }
            break;
        }

        case ekLPAGE_CLOSE:
            if (open_link != NULL)
            {
                stm_printf(latex, "''}} (\\autopageref{%s})", tc(open_link->text));
                /* stm_writef(latex, "''}}"); */
                open_link = NULL;
            }
            break;

        case ekLHEAD_OPEN:
        {
            const Doc *hddoc;
            const Block *hdblock;
            cassert(open_link == NULL);
            open_link = tag;
            hddoc = loader_section(parser->loader, tc(tag->text), parser->doc, &hdblock);
            if (hddoc != NULL)
            {
                stm_printf(latex, "\\textit{\\hyperref[%s::%s]{``", doc_name(hddoc), tc(hdblock->ref));
            }
            else
            {
                String *warn = str_printf("Referenced section '%s' not found", tc(tag->text));
                i_latex_warn(&warn, parser);
                stm_writef(latex, "\\textit{\\hyperref[Unknown]{``");
            }

            break;
        }

        case ekLHEAD_CLOSE:
            if (open_link != NULL)
            {
                const Doc *hddoc;
                const Block *hdblock;
                hddoc = loader_section(parser->loader, tc(open_link->text), parser->doc, &hdblock);
                if (hddoc != NULL)
                    stm_printf(latex, "''}} (\\autopageref{%s::%s})", doc_name(hddoc), tc(hdblock->ref));
                else
                    stm_writef(latex, "''}} (\\autopageref{Unknown})");
                /* stm_writef(latex, "''}}"); */
                open_link = NULL;
            }
            break;

            cassert_default();
        }

    arrst_end()
}

/*---------------------------------------------------------------------------*/

/*
static void i_tex_text(Stream *latex, const ArrSt(Tag) *tags)
{
   arrst_foreach(tag, tags, Tag)
      switch (tag->type) {
       case ekPLAINTEXT:
       case ekLFUNC_OPEN:
       case ekLTYPE_OPEN:
           str_writef(latex, tag->text);
           break;
       default:
           break;
       }
    arrst_end()
}
 */

/*---------------------------------------------------------------------------*/

static bool_t i_with_text(const ArrSt(Tag) *tags)
{
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

void latex_part(Stream *latex, const char_t *title)
{
    stm_printf(latex, "\\part{%s}\n", title);
    stm_writef(latex, "\\thispagestyle{empty}\n\n");
}

/*---------------------------------------------------------------------------*/

void latex_h1(Stream *latex, const Block *block, DocParser *parser)
{
    cassert_no_null(parser);
    cassert(parser->item_level == 0);
    i_latex_tags(latex, block->tags, FALSE, FALSE, parser);
}

/*---------------------------------------------------------------------------*/

void latex_epig(Stream *latex, const Block *block, DocParser *parser)
{
    cassert_no_null(parser);
    cassert(parser->item_level == 0);
    parser->line = block->line;
    stm_writef(latex, "\n");
    stm_writef(latex, "\\setlength\\epigraphwidth{\\textwidth}\n");
    stm_writef(latex, "\\epigraph{\\textit{");
    i_latex_tags(latex, block->tags, FALSE, FALSE, parser);
    stm_writef(latex, "}}");
    if (block->alt1 != NULL)
    {
        stm_writef(latex, "{\\textit{");
        i_latex_tags(latex, block->alt1, FALSE, FALSE, parser);
        stm_writef(latex, "}}");
    }
    stm_writef(latex, "\n");
}

/*---------------------------------------------------------------------------*/

static void i_close_item(DocParser *parser, Stream *latex)
{
    uint32_t i = 0;
    cassert_no_null(parser);
    for (i = 0; i < parser->item_level; ++i)
        stm_writef(latex, "\\end{itemize}\n");
    parser->item_level = 0;
}

/*---------------------------------------------------------------------------*/

void latex_section(Stream *latex, const Block *block, const char_t *secname, const char_t *secref, DocParser *parser)
{
    i_close_item(parser, latex);
    stm_writef(latex, "\n");
    stm_printf(latex, "\\%s{", secname);
    i_latex_tags(latex, block->tags, FALSE, FALSE, parser);
    stm_writef(latex, "}\n");
    if (secref != NULL)
        stm_printf(latex, "\\label{%s::%s}", doc_name(parser->doc), secref);
    else
        stm_printf(latex, "\\label{%s}", doc_name(parser->doc));
}

/*---------------------------------------------------------------------------*/

void latex_parag(Stream *latex, const Block *block, DocParser *parser)
{
    const char_t *param = doc_block_param(block, 0);
    i_close_item(parser, latex);
    stm_writef(latex, "\n");
    if (param != NULL)
    {
        if (str_equ_c(param, "noindent"))
        {
            stm_writef(latex, "\\noindent ");
        }
        else
        {
            String *warn = str_printf("Unknown paragraph parameter '%s'.", param);
            i_latex_warn(&warn, parser);
        }
    }
    else
    {
        /* stm_writef(latex, "\\paragraph{}\n"); */
    }

    i_latex_tags(latex, block->tags, FALSE, FALSE, parser);
    stm_writef(latex, "\n");
}

/*---------------------------------------------------------------------------*/

void latex_bq(Stream *latex, const Block *block, DocParser *parser)
{
    i_close_item(parser, latex);
    stm_writef(latex, "\n");
    stm_writef(latex, "\\begin{mdframed}[backgroundcolor=BQBK_COLOR, linecolor=BQLI_COLOR, nobreak=true]\n");
    stm_writef(latex, "\\textit{");
    i_latex_tags(latex, block->tags, FALSE, TRUE, parser);
    stm_writef(latex, "}\n");
    stm_writef(latex, "\\end{mdframed}\n");
}

/*---------------------------------------------------------------------------*/

static bool_t i_svg_to_pdf(const Config *config, const char_t *src, const char_t *dest)
{
    if (hfile_is_uptodate(src, dest) == FALSE)
    {
        String *str = NULL;
        Proc *proc = NULL;
        bool_t ok = FALSE;
        cassert_no_null(config);
        str = str_printf("inkscape %s --export-pdf=%s --without-gui", src, dest);
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

    return TRUE;
}

/*---------------------------------------------------------------------------*/

/*
static bool_t i_gif_to_png(const char_t *src, const char_t *dest)
{
    if (hfile_is_uptodate(src, dest) == FALSE)
    {
        Image *image = image_from_file(src, NULL);
        bool_t ok;
        if (!image)
            return FALSE;

        cassert(image_get_codec(image) == ekGIF);
        image_codec(image, ekPNG);
        ok = image_to_file(image, dest, NULL);
        image_destroy(&image);
        return ok;
    }

    return TRUE;
}*/

/*---------------------------------------------------------------------------*/

static String *i_latex_img(const char_t *imgname, const char_t *docpath)
{
    String *name = NULL, *ext = NULL;
    String *filename = NULL;
    str_split_pathext(imgname, NULL, &name, &ext);
    filename = str_cpath("%s/img/%s_book.%s", docpath, tc(name), tc(ext));
    str_destroy(&name);
    str_destroy(&ext);

    if (hfile_exists(tc(filename), NULL) == TRUE)
    {
        return filename;
    }
    else
    {
        str_destroy(&filename);
        return str_cpath("%s/img/%s", docpath, imgname);
    }
}

/*---------------------------------------------------------------------------*/

static String *i_optimize_img(const Config *config, const char_t *imgname, const char_t *docpath, const char_t *prespath, DocParser *parser)
{
    String *imgsrc = i_latex_img(imgname, docpath);
    String *imgdest = NULL;
    const char_t *ext = str_filext(tc(imgsrc));
    bool_t ok = TRUE;

    if (ext != NULL && str_equ_c(ext, "svg"))
    {
        String *name = NULL;
        str_split_pathext(imgname, NULL, &name, NULL);
        imgdest = str_cpath("%s/%s.pdf", prespath, tc(name));
        ok = i_svg_to_pdf(config, tc(imgsrc), tc(imgdest));
        str_destroy(&name);
    }
    else if (ext != NULL && str_equ_c(ext, "gif"))
    {
        String *name = NULL;
        String *imgsrc2;
        str_split_pathext(imgname, NULL, &name, NULL);
        imgsrc2 = str_cpath("%s/img/%s.png", docpath, tc(name));
        imgdest = str_cpath("%s/%s.png", prespath, tc(name));
        ok = hfile_copy(tc(imgsrc2), tc(imgdest), NULL);

        if (ok == FALSE)
        {
            String *warn = str_printf("No .png version of '%s' image", tc(imgsrc));
            i_latex_warn(&warn, parser);
        }

        /* ok = i_gif_to_png(tc(imgsrc), tc(imgdest)); */
        str_destroy(&name);
        str_destroy(&imgsrc2);
    }
    else
    {
        imgdest = str_cpath("%s/%s", prespath, imgname);
        ok = hfile_copy(tc(imgsrc), tc(imgdest), NULL);
    }

    str_destroy(&imgsrc);

    if (ok == TRUE)
    {
        str_subs(imgdest, '\\', '/');
        return imgdest;
    }
    else
    {
        str_destroy(&imgdest);
        return NULL;
    }
}

/*---------------------------------------------------------------------------*/

static void i_li(Stream *latex, const Block *block, const Config *config, const char_t *docpath, const char_t *prespath, DocParser *parser)
{
    const char_t *icon = doc_block_param(block, 0);
    stm_writef(latex, "\\item[\\textcolor{TITLE_COLOR}{\\textbullet}]");

    if (icon != NULL)
    {
        String *imgdest = i_optimize_img(config, icon, docpath, prespath, parser);
        if (imgdest != NULL)
        {
            stm_printf(latex, "\\licon{%s} ", tc(imgdest));
            str_destroy(&imgdest);
        }
        else
        {
            String *warn = str_printf("Error procesing li '%s' icon", icon);
            i_latex_warn(&warn, parser);
        }
    }

    i_latex_tags(latex, block->tags, FALSE, FALSE, parser);
    stm_writef(latex, "\n");
}

/*---------------------------------------------------------------------------*/

void latex_li(Stream *latex, const Block *block, const Config *config, const char_t *docpath, const char_t *prespath, DocParser *parser)
{
    cassert_no_null(parser);
    if (parser->item_level == 0)
    {
        stm_writef(latex, "\n");
        stm_writef(latex, "\\begin{itemize}\n");
    }
    else if (parser->item_level == 2)
    {
        stm_writef(latex, "\\end{itemize}\n");
    }
    else
    {
        cassert(parser->item_level == 1);
    }

    parser->item_level = 1;
    i_li(latex, block, config, docpath, prespath, parser);
}

/*---------------------------------------------------------------------------*/

void latex_lili(Stream *latex, const Block *block, const Config *config, const char_t *docpath, const char_t *prespath, DocParser *parser)
{
    cassert_no_null(parser);
    if (parser->item_level == 1)
    {
        stm_writef(latex, "\n");
        stm_writef(latex, "\\begin{itemize}\n");
    }
    else
    {
        cassert(parser->item_level == 2);
    }

    parser->item_level = 2;
    i_li(latex, block, config, docpath, prespath, parser);
}

/*---------------------------------------------------------------------------*/

void latex_liend(Stream *latex, DocParser *parser)
{
    i_close_item(parser, latex);
}

/*---------------------------------------------------------------------------*/

void latex_img(Stream *latex, const Block *block, const char_t *docpath, const char_t *prespath, DocParser *parser)
{
    const char_t *imgname = doc_block_param(block, 0);
    real32_t width = doc_real_param(block, 2);
    String *imgdest = NULL;
    cassert_no_null(parser);
    if (imgname != NULL)
        imgdest = i_optimize_img(parser->config, imgname, docpath, prespath, parser);

    if (imgdest == NULL)
    {
        String *warn = str_printf("Error procesing '%s' image", imgname != NULL ? imgname : "Unknown");
        i_latex_warn(&warn, parser);
        imgdest = str_c("UNKNOWN_IMAGE");
    }

    if (width < 0.f)
        width = .65f;

    stm_writef(latex, "\n");
    stm_writef(latex, "\\begin{figure}[!h]\n");
    if (i_with_text(block->tags) == TRUE)
    {
        /* Image caption at left-bottom */
        if (width < .65f)
        {
            real32_t caps_width = 1.f - width - .1f;
            if (caps_width > .35f)
                caps_width = .35f;
            stm_writef(latex, "\\floatbox[{\\capbeside\\thisfloatsetup{capbesideposition={left,bottom},capbesidesep=qquad,capbesidewidth=");
            stm_printf(latex, "%.2f\\textwidth}}]\n", caps_width);
            stm_writef(latex, "{figure}[\\FBwidth]\n");
            stm_writef(latex, "{\\caption{");
            i_latex_tags(latex, block->tags, FALSE, FALSE, parser);
            stm_writef(latex, "}\n");
            stm_printf(latex, "\\label{%s::img%s}}\n", doc_name(parser->doc), tc(block->ref));
            stm_printf(latex, "{\\includegraphics[width=%.2f\\textwidth]{%s}}\n", width, tc(imgdest));
        }
        /* Image caption at bottom */
        else
        {
            stm_writef(latex, "\\centering\n");
            stm_printf(latex, "\\captionsetup{width=%.2f\\textwidth}\n", width);
            stm_printf(latex, "\\includegraphics[width=%.2f\\textwidth]{%s}\n", width, tc(imgdest));
            stm_writef(latex, "{\\caption{");
            i_latex_tags(latex, block->tags, FALSE, FALSE, parser);
            stm_writef(latex, "}\n");
            stm_printf(latex, "\\label{%s::img%s}}\n", doc_name(parser->doc), tc(block->ref));
        }
    }
    /* Image without caption */
    else
    {
        stm_writef(latex, "\\centering\n");
        stm_printf(latex, "\\includegraphics[width=%.2f\\textwidth]{%s}\n", width, tc(imgdest));
        stm_printf(latex, "\\label{%s::img%s}\n", doc_name(parser->doc), tc(block->ref));
    }

    stm_writef(latex, "\\end{figure}\n");
    str_destroy(&imgdest);
}

/*---------------------------------------------------------------------------*/

void latex_img2(Stream *latex, const Block *block, const char_t *docpath, const char_t *prespath, DocParser *parser)
{
    const char_t *imgname1 = doc_block_param(block, 0);
    const char_t *imgname2 = doc_block_param(block, 1);
    real32_t width = doc_real_param(block, 3);
    String *imgbase = NULL;
    String *imgdest1 = NULL;
    String *imgdest2 = NULL;
    cassert_no_null(parser);

    if (imgname1 != NULL)
        imgdest1 = i_optimize_img(parser->config, imgname1, docpath, prespath, parser);

    if (imgname2 != NULL)
        imgdest2 = i_optimize_img(parser->config, imgname2, docpath, prespath, parser);

    if (imgdest1 == NULL)
    {
        String *warn = str_printf("Error procesing '%s' image", imgname1 != NULL ? imgname1 : "Unknown");
        i_latex_warn(&warn, parser);
        imgdest1 = str_c("UNKNOWN_IMAGE");
    }

    if (imgdest2 == NULL)
    {
        String *warn = str_printf("Error procesing '%s' image", imgname2 != NULL ? imgname2 : "Unknown");
        i_latex_warn(&warn, parser);
        imgdest2 = str_c("UNKNOWN_IMAGE");
    }

    if (imgname1 != NULL)
        str_split_pathext(imgname1, NULL, &imgbase, NULL);
    else
        imgbase = str_c("NOIMAGE");

    if (width < 0.f)
        width = .85f;

    stm_writef(latex, "\n");
    stm_writef(latex, "\\begin{figure}[!h]\n");
    stm_writef(latex, "\\begin{minipage}{0.48\\textwidth}\n");
    stm_printf(latex, "\\includegraphics[width=%.2f\\textwidth,right]{%s}\n", width, tc(imgdest1));
    stm_writef(latex, "\\end{minipage}\n");
    stm_writef(latex, "\\hspace{.02\\textwidth}\n");
    stm_writef(latex, "\\begin{minipage}{0.48\\textwidth}\n");
    stm_printf(latex, "\\includegraphics[width=%.2f\\textwidth,left]{%s}\n", width, tc(imgdest2));
    stm_writef(latex, "\\end{minipage}\n");
    if (i_with_text(block->tags) == TRUE)
    {
        stm_writef(latex, "\\caption{");
        i_latex_tags(latex, block->tags, FALSE, FALSE, parser);
        stm_writef(latex, "}\n");
    }

    stm_printf(latex, "\\label{%s::img%s}\n", doc_name(parser->doc), tc(block->ref));
    stm_writef(latex, "\\end{figure}\n");
    str_destroy(&imgbase);
    str_destroy(&imgdest1);
    str_destroy(&imgdest2);
}

/*---------------------------------------------------------------------------*/

static uint32_t i_num_cols(const ArrSt(Block) *blocks)
{
    uint32_t n = 0;
    arrst_foreach_const(child, blocks, Block)
        if (child->type == ekROW)
        {
            if (n > 0)
                return n;
        }
        else
        {
            n += 1;
        }
    arrst_end()
    return n;
}

/*---------------------------------------------------------------------------*/

void latex_table(Stream *latex, const Block *block, const char_t *docpath, const char_t *prespath, DocParser *parser)
{
    uint32_t i, ncols = 0, nrow = UINT32_MAX, ncol = 0;
    Stream *head, *body, *current;
    String *shead, *sbody;
    const char_t *pcolumns = doc_block_param(block, 1);

    cassert_no_null(block);
    unref(docpath);
    unref(prespath);
    unref(parser);

    ncols = i_num_cols(block->children);
    head = stm_memory(512);
    body = stm_memory(1024);
    current = head;

    arrst_foreach(child, block->children, Block)
        switch (child->type)
        {
        case ekROW:
            if (nrow == UINT32_MAX)
            {
                current = head;
                nrow = 0;
            }
            else
            {
                if (current == body)
                    stm_writef(current, "\\\\\n\\hline\n");

                nrow += 1;
                current = body;
            }

            ncol = 0;
            break;

        case ekIMG:
        {
            const char_t *imgname = doc_block_param(child, 0);
            real32_t width = doc_real_param(child, 2);
            String *imgdest = NULL;

            if (imgname != NULL)
                imgdest = i_optimize_img(parser->config, imgname, docpath, prespath, parser);

            if (imgdest == NULL)
            {
                String *warn = str_printf("Error procesing table '%s' image", imgname != NULL ? imgname : "Unknown");
                i_latex_warn(&warn, parser);
                imgdest = str_c("UNKNOWN_IMAGE");
            }

            stm_printf(current, "\\includegraphics[width=%.2f\\textwidth,align=c]{%s}", width, tc(imgdest));

            if (ncol < ncols - 1)
                stm_writef(current, " & ");

            str_destroy(&imgdest);
            ncol += 1;
            break;
        }

        case ekPARAG:
        {
            i_latex_tags(current, child->tags, TRUE, FALSE, parser);

            if (ncol < ncols - 1)
                stm_writef(current, " & ");

            ncol += 1;
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

    shead = stm_str(head);
    sbody = stm_str(body);

    i_close_item(parser, latex);

    stm_writef(latex, "\n");
    stm_writef(latex, "\\renewcommand{\\arraystretch}{1.4}\n");
    stm_writef(latex, "\\begin{center}\n");

    /* stm_writef(latex, "\\tablefirsthead{\\hline \\rowcolor{GRAY_COLOR} "); */
    stm_writef(latex, "\\tablefirsthead{\\hline ");
    stm_writef(latex, tc(shead));
    stm_writef(latex, "\\\\}\n");

    /* stm_writef(latex, "\\tablehead{\\hline \\rowcolor{GRAY_COLOR} "); */
    stm_writef(latex, "\\tablehead{\\hline ");
    stm_writef(latex, tc(shead));
    stm_writef(latex, "\\\\}\n");

    stm_writef(latex, "\\begin{supertabular}");
    if (pcolumns != NULL)
    {
        stm_printf(latex, "{%s}\n", pcolumns);
    }
    else
    {
        stm_writef(latex, "{|");
        for (i = 0; i < ncols; ++i)
            stm_writef(latex, " c |");
        stm_writef(latex, "}\n");
    }

    stm_writef(latex, "\\hline\n");
    stm_writef(latex, tc(sbody));
    stm_writef(latex, "\\\\\n\\hline\n");
    stm_writef(latex, "\\end{supertabular}\n");

    if (i_with_text(block->tags) == TRUE)
    {
        stm_writef(latex, "\\captionof{table}{");
        i_latex_tags(latex, block->tags, FALSE, FALSE, parser);
        stm_writef(latex, "}\n");
    }

    stm_printf(latex, "\\label{%s::table%s}\n", doc_name(parser->doc), tc(block->ref));
    stm_writef(latex, "\\end{center}\n");
    stm_writef(latex, "\\renewcommand{\\arraystretch}{1}\n");

    stm_close(&head);
    stm_close(&body);
    str_destroy(&shead);
    str_destroy(&sbody);
}

/*---------------------------------------------------------------------------*/

void latex_code(Stream *latex, const Block *block, DocParser *parser)
{
    const char_t *clang = NULL;
    const char_t *clabel = NULL;

    cassert_no_null(block);
    cassert_no_null(parser);
    /* i_close_item(parser, latex); */
    clang = doc_block_param(block, 0);
    clabel = doc_block_param(block, 1);

    stm_writef(latex, "\n");
    stm_writef(latex, "\\begin{lstlisting}[firstnumber=1");

    if (clabel != NULL)
        stm_printf(latex, ", label={%s::list%s}", doc_name(parser->doc), tc(block->ref));

    if (clang != NULL && str_equ_c(clang, "text"))
        stm_writef(latex, ", language={}, emph={}, emph={[2]}, emph={[3]}");

    if (block->tags != NULL && i_with_text(block->tags) == TRUE)
    {
        if (block->ref != NULL)
            stm_writef(latex, ", caption={");
        else
            stm_writef(latex, ", title={");

        i_latex_tags(latex, block->tags, FALSE, FALSE, parser);
        stm_writef(latex, "},captionpos=t]");
    }
    else
    {
        stm_writef(latex, "]");
    }

    stm_writef(latex, "\n");

    if (block->code != NULL)
    {
        const byte_t *data = stm_buffer(block->code);
        uint32_t size = stm_buffer_size(block->code);
        stm_write(latex, data, size);
    }

    stm_writef(latex, "\\end{lstlisting}\n");
}

/*---------------------------------------------------------------------------*/

void latex_math(Stream *latex, const Block *block, DocParser *parser)
{
    const char_t *clabel;
    bool_t caption;
    const byte_t *data = NULL;
    uint32_t size = 0;
    cassert_no_null(block);
    cassert_no_null(parser);
    caption = (bool_t)(block->tags != NULL && i_with_text(block->tags) == TRUE);
    clabel = doc_block_param(block, 0);
    data = stm_buffer(block->code);
    size = stm_buffer_size(block->code);
    stm_writef(latex, "\n");

    stm_writef(latex, "\\begin{equcaption}[!h]\n");
    stm_write(latex, data, size);

    if (caption == TRUE)
    {
        stm_writef(latex, "\\caption{");
        i_latex_tags(latex, block->tags, FALSE, FALSE, parser);
        stm_writef(latex, "}\n");
    }

    if (clabel != NULL)
        stm_printf(latex, "\\label{%s::equ%s}\n", doc_name(parser->doc), tc(block->ref));

    stm_writef(latex, "\\end{equcaption}\n");
}

/*---------------------------------------------------------------------------*/

void latex_type(Stream *latex, const Block *type, DocParser *parser)
{
    const char_t *param = doc_block_param(type, 0);
    cassert_no_null(parser);
    cassert(parser->item_level == 0);
    if (param != NULL)
    {
        stm_writef(latex, "\n\\subsection*{");
        i_lwrite(latex, param);
        stm_writef(latex, "}\n");
        stm_writef(latex, "\\index{");
        i_lwrite(latex, param);
        stm_writef(latex, "}\n");
        stm_printf(latex, "\\label{%s::%s}\n", doc_name(parser->doc), tc(type->ref));
    }
    else
    {
        stm_writef(latex, "\n\\subsection{NOTYPE}\n");
    }

    stm_writef(latex, "\\noindent ");
    i_latex_tags(latex, type->tags, FALSE, FALSE, parser);
    stm_writef(latex, "\n");
}

/*---------------------------------------------------------------------------*/

static void i_latex_type(Stream *latex, const char_t *type, const ptype_t ptype, DocParser *parser)
{
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

    stm_writef(latex, type);
}

/*---------------------------------------------------------------------------*/

static void i_write_constant(Stream *latex, const char_t *type, const ptype_t ptype, const char_t *name, const char_t *noconst, const char_t *val, const char_t *valsufix, DocParser *parser)
{
    if (!noconst || !str_equ_c(noconst, "noconst"))
        stm_writef(latex, "const ");

    i_latex_type(latex, type, ptype, parser);
    stm_writef(latex, " ");
    stm_writef(latex, name ? name : "");

    if (str_empty_c(val))
        stm_writef(latex, ";\n");
    else
        stm_printf(latex, " = %s%s;\n", val, valsufix);
}

/*---------------------------------------------------------------------------*/

void latex_const(Stream *latex, const Block *ctype, DocParser *parser)
{
    const char_t *type = doc_block_param(ctype, 0);
    const char_t *name = doc_block_param(ctype, 1);
    const char_t *val = doc_block_param(ctype, 2);
    const char_t *noconst = doc_block_param(ctype, 3);
    cassert_no_null(parser);
    cassert(parser->item_level == 0);

    if (type != NULL && name != NULL)
    {
        if (doc_is_real_type(type, parser->loader))
        {
            String *cname = doc_real_constant(name);
            String *fname = doc_float_constant(type, name);
            String *dname = doc_double_constant(type, name);
            stm_writef(latex, "\n\\subsection*{");
            i_lwrite(latex, tc(cname));
            stm_writef(latex, "}\n");
            stm_writef(latex, "\\index{");
            i_lwrite(latex, tc(cname));
            stm_writef(latex, "}\n");
            stm_writef(latex, "\\index{");
            i_lwrite(latex, tc(fname));
            stm_writef(latex, "}\n");
            stm_writef(latex, "\\index{");
            i_lwrite(latex, tc(dname));
            stm_writef(latex, "}\n");
            str_destroy(&cname);
            str_destroy(&fname);
            str_destroy(&dname);
        }
        else
        {
            stm_writef(latex, "\n\\subsection*{");
            i_lwrite(latex, name);
            stm_writef(latex, "}\n");
            stm_writef(latex, "\\index{");
            i_lwrite(latex, name);
            stm_writef(latex, "}\n");
        }

        stm_printf(latex, "\\label{%s::%s}\n", doc_name(parser->doc), tc(ctype->ref));
    }
    else
    {
        stm_writef(latex, "\n\\subsection{NOCONST}\n");
    }

    stm_writef(latex, "\\noindent ");
    i_latex_tags(latex, ctype->tags, FALSE, FALSE, parser);
    stm_writef(latex, "\n");
    stm_writef(latex, "\\begin{lstlisting}[numbers=none]\n");

    if (type != NULL && name != NULL)
    {
        if (doc_is_real_type(type, parser->loader))
        {
            String *cfloat = doc_float_constant(type, name);
            String *cdouble = doc_double_constant(type, name);
            String *ctempl = doc_template_constant(type, name);
            i_write_constant(latex, type, ekFLOAT, tc(cfloat), noconst, val, "f", parser);
            i_write_constant(latex, type, ekDOUBLE, tc(cdouble), noconst, val, "", parser);
            i_write_constant(latex, type, ekREAL, tc(ctempl), noconst, NULL, NULL, parser);
            str_destroy(&cfloat);
            str_destroy(&cdouble);
            str_destroy(&ctempl);
        }
        else
        {
            i_write_constant(latex, type, ekOTHER, name, noconst, val, "", parser);
        }
    }

    stm_writef(latex, "\\end{lstlisting}\n");
}

/*---------------------------------------------------------------------------*/

void latex_enum(Stream *latex, const Block *type, DocParser *parser)
{
    const char_t *par0 = doc_block_param(type, 0);
    cassert_no_null(parser);
    cassert(parser->item_level == 0);
    if (par0 != NULL)
    {
        stm_writef(latex, "\n\\subsection*{enum ");
        i_lwrite(latex, par0);
        stm_writef(latex, "}\n");
        stm_writef(latex, "\\index{");
        i_lwrite(latex, par0);
        stm_writef(latex, "}\n");
        stm_printf(latex, "\\label{%s::%s}\n", doc_name(parser->doc), tc(type->ref));
    }
    else
    {
        stm_writef(latex, "\n\\subsection{NOCONST}\n");
    }

    stm_writef(latex, "\\noindent ");
    i_latex_tags(latex, type->tags, FALSE, FALSE, parser);
    stm_writef(latex, "\n\n");

    /*     stm_writef(latex, "\\begin{lstlisting}[numbers=none,emph={[3]}]\n");
    stm_printf(latex, "enum %s\n", par0 ? par0 : "");
    stm_writef(latex, "{\n");
    arrst_foreach(value, type->children, Block)
        const char_t *val0 = doc_block_param(value, 0);
        stm_writef(latex, "    ");
        stm_writef(latex, val0 ? val0 : "");
        if (value_i < value_total - 1)
            stm_writef(latex, ",\n");
    arrst_end()

    stm_writef(latex, "\n};\n");
    stm_writef(latex, "\\end{lstlisting}\n");
 */
    stm_writef(latex, "\n\\vspace{-2mm}\n");
    stm_writef(latex, "\\renewcommand{\\arraystretch}{1.4}\n");
    stm_writef(latex, "\\begin{center}\n");
    stm_writef(latex, "\\tablefirsthead{}\n");
    stm_writef(latex, "\\tablehead{}\n");
    stm_writef(latex, "\\begin{supertabular}{ r p{10cm} }\n");

    arrst_foreach(value, type->children, Block)
        const char_t *val0 = doc_block_param(value, 0);
        stm_writef(latex, "\\index{");
        i_lwrite(latex, val0);
        stm_writef(latex, "}\n");
        stm_writef(latex, "\\lstinline[basicstyle=\\ttfamily,postbreak={}]$");
        stm_writef(latex, val0 != NULL ? val0 : "NOVALUE");
        stm_writef(latex, "$ & ");
        i_latex_tags(latex, value->tags, TRUE, FALSE, parser);
        stm_writef(latex, "\\\\\n");
    arrst_end()

    stm_writef(latex, "\\end{supertabular}\n");
    stm_writef(latex, "\\end{center}\n");
    stm_writef(latex, "\\renewcommand{\\arraystretch}{1}\n");
}

/*---------------------------------------------------------------------------*/

static void i_write_struct(Stream *latex, const char_t *name, const ptype_t ptype, const Block *sblock, DocParser *parser)
{
    stm_printf(latex, "struct %s", name ? name : "");
    if (arrst_size(sblock->children, Block) > 0)
    {
        stm_writef(latex, "\n{\n");
        arrst_foreach(member, sblock->children, Block)
            const char_t *mtype = doc_block_param(member, 0);
            const char_t *mname = doc_block_param(member, 1);
            stm_writef(latex, "    ");
            i_latex_type(latex, mtype, ptype, parser);
            stm_writef(latex, " ");
            stm_writef(latex, mname ? mname : "");
            stm_writef(latex, ";\n");
        arrst_end()
        stm_writef(latex, "};\n");
    }
    else
    {
        stm_writef(latex, ";\n");
    }
}

/*---------------------------------------------------------------------------*/

void latex_struct(Stream *latex, const Block *sblock, DocParser *parser)
{
    const char_t *name = doc_block_param(sblock, 0);
    const char_t *type = doc_block_param(sblock, 1);
    cassert_no_null(parser);
    cassert(parser->item_level == 0);
    if (name != NULL)
    {
        stm_writef(latex, "\n\\subsection*{struct ");
        i_lwrite(latex, name);
        stm_writef(latex, "}\n");
        stm_writef(latex, "\\index{");
        i_lwrite(latex, name);
        stm_writef(latex, "}\n");
        stm_printf(latex, "\\label{%s::%s}\n", doc_name(parser->doc), tc(sblock->ref));
    }
    else
    {
        stm_writef(latex, "\n\\subsection{NOCONST}\n");
    }

    stm_writef(latex, "\\noindent ");
    i_latex_tags(latex, sblock->tags, FALSE, FALSE, parser);
    stm_writef(latex, "\n\n");

    stm_writef(latex, "\\begin{lstlisting}[numbers=none]\n");

    if (type && str_equ_c(type, "real"))
    {
        String *fstruct = doc_float_struct(name);
        String *dstruct = doc_double_struct(name);
        String *tstruct = doc_template_struct(name);
        i_write_struct(latex, tc(fstruct), ekFLOAT, sblock, parser);
        stm_writef(latex, "\n");
        i_write_struct(latex, tc(dstruct), ekDOUBLE, sblock, parser);
        stm_writef(latex, "\n");
        i_write_struct(latex, tc(tstruct), ekREAL, sblock, parser);
        str_destroy(&fstruct);
        str_destroy(&dstruct);
        str_destroy(&tstruct);
    }
    else
    {
        i_write_struct(latex, name, ekOTHER, sblock, parser);
    }

    stm_writef(latex, "\\end{lstlisting}\n");

    if (arrst_size(sblock->children, Block) > 0)
    {
        stm_writef(latex, "\\vspace{-2mm}\n");
        stm_writef(latex, "\\renewcommand{\\arraystretch}{1.4}\n");
        stm_writef(latex, "\\begin{center}\n");
        stm_writef(latex, "\\tablefirsthead{}\n");
        stm_writef(latex, "\\tablehead{}\n");
        stm_writef(latex, "\\begin{supertabular}{ r p{10cm} }\n");

        arrst_foreach(member, sblock->children, Block)
            /* const char_t *mtype = doc_block_param(member, 0); */
            const char_t *mname = doc_block_param(member, 1);
            stm_writef(latex, "\\lstinline[basicstyle=\\ttfamily,postbreak={}]$");
            stm_writef(latex, mname != NULL ? mname : "NOVALUE");
            stm_writef(latex, "$ & ");
            i_latex_tags(latex, member->tags, TRUE, FALSE, parser);
            stm_writef(latex, "\\\\\n");
        arrst_end()

        stm_writef(latex, "\\end{supertabular}\n");
        stm_writef(latex, "\\end{center}\n");
        stm_writef(latex, "\\renewcommand{\\arraystretch}{1}\n");
    }
}

/*---------------------------------------------------------------------------*/

static void i_latex_param(Stream *latex, const Block *param, const ptype_t ptype, DocParser *parser)
{
    const char_t *name = NULL;
    if (param == NULL)
    {
        stm_writef(latex, "void");
        return;
    }

    name = doc_block_param(param, 1);
    if (str_empty(param->ptype) == FALSE)
    {
        if (param->is_const == TRUE)
            stm_writef(latex, "const ");

        i_latex_type(latex, tc(param->ptype), ptype, parser);

        if (name != NULL)
            stm_writef(latex, " ");

        if (param->is_dptr == TRUE)
            stm_writef(latex, "**");
        else if (param->is_ptr == TRUE)
            stm_writef(latex, "*");
    }

    if (name != NULL)
        stm_writef(latex, name);
}

/*---------------------------------------------------------------------------*/

static void i_write_func(Stream *latex, const Block *func, const char_t *fname, const ptype_t ptype, DocParser *parser)
{
    const Block *fret = NULL;
    const Block *fparams = NULL;
    uint32_t i, nparams = 0;
    uint32_t nsize = 0;
    String *blanks = NULL;

    doc_func_params(func, &fret, &fparams, &nparams);
    i_latex_param(latex, fret, ptype, parser);
    stm_writef(latex, "\n");

    nsize = str_len_c(fname);
    nsize += 1; /* "(" */

    if (func->is_ptr == TRUE)
    {
        stm_writef(latex, "(*");
        nsize += 3;
    }

    blanks = str_fill(nsize, ' ');
    stm_writef(latex, fname);

    if (func->is_ptr == TRUE)
        stm_writef(latex, ")");

    stm_writef(latex, "(");
    if (nparams == 0)
        stm_writef(latex, "void");

    for (i = 0; i < nparams; ++i)
    {
        i_latex_param(latex, fparams + i, ptype, parser);
        if (i < nparams - 1)
        {
            stm_writef(latex, ",\n");
            str_writef(latex, blanks);
        }
    }

    stm_writef(latex, ");\n");
    str_destroy(&blanks);
}

/*---------------------------------------------------------------------------*/

void latex_func(Stream *latex, const Block *func, DocParser *parser)
{
    const char_t *fname = doc_block_param(func, 0);
    const char_t *alias = doc_block_param(func, 1);
    const Block *fret = NULL;
    const Block *fparams = NULL;
    uint32_t i, nparams = 0;
    cassert_no_null(parser);
    cassert(parser->item_level == 0);
    if (fname != NULL)
    {
        stm_writef(latex, "\n\\subsection*{");

        if (alias != NULL && !func->is_ptr)
        {
            String *rfunc = doc_real_func(fname, alias);
            i_lwrite(latex, tc(rfunc));
            str_destroy(&rfunc);
        }
        else
        {
            i_lwrite(latex, fname);
        }

        stm_writef(latex, "}\n");

        if (alias != NULL && !func->is_ptr)
        {
            String *namef = doc_float_func(fname, alias);
            String *named = doc_double_func(fname, alias);
            String *namet = doc_template_func(fname, alias);
            stm_writef(latex, "\\index{");
            i_lwrite(latex, tc(namef));
            stm_writef(latex, "}\n");
            stm_writef(latex, "\\index{");
            i_lwrite(latex, tc(named));
            stm_writef(latex, "}\n");
            stm_writef(latex, "\\index{");
            i_lwrite(latex, tc(namet));
            stm_writef(latex, "}\n");
            str_destroy(&namef);
            str_destroy(&named);
            str_destroy(&namet);
        }
        else
        {
            stm_writef(latex, "\\index{");
            i_lwrite(latex, fname);
            stm_writef(latex, "}\n");
        }

        stm_printf(latex, "\\label{%s::%s}\n", doc_name(parser->doc), tc(func->ref));
    }
    else
    {
        stm_writef(latex, "\n\\subsection{NOFUNC}\n");
        return;
    }

    stm_writef(latex, "\\noindent ");
    i_latex_tags(latex, func->tags, FALSE, FALSE, parser);
    stm_writef(latex, "\n");

    stm_writef(latex, "\\begin{lstlisting}[numbers=none, frame=single]\n");

    if (alias != NULL && !func->is_ptr)
    {
        String *namef = doc_float_func(fname, alias);
        String *named = doc_double_func(fname, alias);
        String *namet = doc_template_func(fname, alias);
        i_write_func(latex, func, tc(namef), ekFLOAT, parser);
        stm_writef(latex, "\n");
        i_write_func(latex, func, tc(named), ekDOUBLE, parser);
        stm_writef(latex, "\n");
        i_write_func(latex, func, tc(namet), ekREAL, parser);
        str_destroy(&namef);
        str_destroy(&named);
        str_destroy(&namet);
    }
    else
    {
        i_write_func(latex, func, fname, ekOTHER, parser);
    }

    stm_writef(latex, "\\end{lstlisting}\n");

    if (func->code != NULL)
    {
        const byte_t *data = NULL;
        uint32_t size = 0;
        data = stm_buffer(func->code);
        size = stm_buffer_size(func->code);
        stm_writef(latex, "\n");
        stm_writef(latex, "\\begin{lstlisting}[firstnumber=1]\n");
        stm_write(latex, data, size);
        stm_writef(latex, "\\end{lstlisting}\n");
    }

    doc_func_params(func, &fret, &fparams, &nparams);
    if (nparams > 0)
    {
        stm_writef(latex, "\n");
        stm_writef(latex, "\\vspace{-2mm}\n");
        stm_writef(latex, "\\renewcommand{\\arraystretch}{1.4}\n");
        stm_writef(latex, "\\begin{center}\n");
        stm_writef(latex, "\\begin{tabular}{ r p{12cm} }\n");
        for (i = 0; i < nparams; ++i)
        {
            const char_t *name = doc_block_param(fparams + i, 1);
            i_lwrite(latex, name != NULL ? name : "NOPARAM");
            stm_writef(latex, " & ");
            i_latex_tags(latex, fparams[i].tags, TRUE, FALSE, parser);
            stm_writef(latex, " \\\\\n");
        }
        stm_writef(latex, "\\end{tabular}\n");
        stm_writef(latex, "\\end{center}\n");
        stm_writef(latex, "\\renewcommand{\\arraystretch}{1}\n");
    }

    if (fret != NULL)
    {
        stm_printf(latex, "\n\\noindent\\textbf{%s:}\n\n", respack_text(parser->respack, TEXT_24));
        i_latex_tags(latex, fret->tags, FALSE, FALSE, parser);
        stm_writef(latex, "\n");
    }

    if (func->alt1 != NULL)
    {
        stm_writef(latex, "\n");
        if (fret != NULL)
            stm_writef(latex, "\\vspace{2mm}");

        stm_printf(latex, "\\noindent\\textbf{%s:}\n\n", respack_text(parser->respack, TEXT_25));
        i_latex_tags(latex, func->alt1, FALSE, FALSE, parser);
        stm_writef(latex, "\n");
    }
}
