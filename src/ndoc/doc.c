/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: doc.c
 *
 */

/* Document */

#include "doc.h"
#include "latex.h"
#include "loader.h"
#include "nlog.h"
#include "web.h"
#include "res_ndoc.h"
#include <html5/html5.h>
#include <html5/post.h>
#include <html5/wsite.h>
#include <nlib/ssh.h>
#include <core/arrpt.h>
#include <core/arrst.h>
#include <core/event.h>
#include <core/heap.h>
#include <core/hfile.h>
#include <core/respack.h>
#include <core/stream.h>
#include <core/strings.h>
#include <sewer/bmem.h>
#include <sewer/cassert.h>
#include <sewer/ptr.h>
#include <sewer/unicode.h>

typedef struct _blockid_t BlockId;
typedef struct _tagid_t TagId;

struct _blockid_t
{
    btype_t id;
    const char_t *block;
};

static const BlockId BLOCKS[] = {
    {ekBDRAFT, "draft"},
    {ekBLANGREV, "langrev"},
    {ekBCOMPLETE, "complete"},
    {ekEBOOK_ONLY, "ebook"},
    {ekWEB_ONLY, "web"},
    {ekNOTOC, "notoc"},
    {ekNOSECNUM, "nosecnum"},
    {ekEPIG, "ep"},
    {ekDESC, "desc"},
    {ekH1, "h1"},
    {ekH2, "h2"},
    {ekH3, "h3"},
    {ekPARAG, "p"},
    {ekIMG, "img"},
    {ekIMG2, "img2"},
    {ekTABLE, "table"},
    {ekROW, "row"},
    {ekLI, "li"},
    {ekLILI, "lili"},
    {ekBQ, "bq"},
    {ekCODE, "code"},
    {ekCODEFILE, "codefile"},
    {ekMATH, "math"},
    {ekTYPE, "type"},
    {ekCONST, "const"},
    {ekENUM, "enum"},
    {ekENUMV, "enumv"},
    {ekSTRUCT, "struct"},
    {ekSMEMBER, "smember"},
    {ekFUNC, "func"},
    {ekFPAR, "fpar"},
    {ekFRET, "fret"},
    {ekFCODE, "fcode"},
    {ekFNOTE, "fnote"},
    {ekWARN, "warn"}};

static const uint32_t NBLOCKS = sizeof(BLOCKS) / sizeof(BlockId);

struct _tagid_t
{
    ttype_t id;
    const char_t *tag;
};

static const TagId TAGS[] = {
    {ekLESS, "<<>"},
    {ekGREATER, "<>>"},
    {ekBOLD_OPEN, "<b>"},
    {ekBOLD_CLOSE, "</b>"},
    {ekITALIC_OPEN, "<i>"},
    {ekITALIC_CLOSE, "</i>"},
    {ekCODE_OPEN, "<c>"},
    {ekCODE_CLOSE, "</c>"},
    {ekSUB_OPEN, "<sub>"},
    {ekSUB_CLOSE, "</sub>"},
    {ekSUP_OPEN, "<sup>"},
    {ekSUP_CLOSE, "</sup>"},
    {ekUNDER_OPEN, "<u>"},
    {ekUNDER_CLOSE, "</u>"},
    {ekSTRIKE_OPEN, "<del>"},
    {ekSTRIKE_CLOSE, "</del>"},
    {ekREF_OPEN, "<r>"},
    {ekREF_CLOSE, "</r>"},

    {ekMATH_OPEN, "<m>"},
    {ekMATH_CLOSE, "</m>"},

    {ekLINK_OPEN, "<l>"},
    {ekLINK_CLOSE, "</l>"},
    {ekLFUNC_OPEN, "<lf>"},
    {ekLFUNC_CLOSE, "</lf>"},
    {ekLTYPE_OPEN, "<lt>"},
    {ekLTYPE_CLOSE, "</lt>"},
    {ekLPAGE_OPEN, "<lp>"},
    {ekLPAGE_CLOSE, "</lp>"},
    {ekLHEAD_OPEN, "<lh>"},
    {ekLHEAD_CLOSE, "</lh>"}};

static const uint32_t NTAGS = sizeof(TAGS) / sizeof(TagId);

typedef enum _docstate_t
{
    ekDRAFT,
    ekLANGREV,
    ekMISSING,
    ekCOMPLETE
} docstate_t;

typedef enum _doctype_t
{
    ekDOCPOST,
    ekDOCLANDING
} doctype_t;

typedef enum _generator_t
{
    ekGEN_EBOOK,
    ekGEN_WEB,
    ekGEN_ALL
} generator_t;

struct _doc_t
{
    doctype_t type;
    docstate_t state;
    String *packname;
    String *name;
    bool_t with_toc;
    bool_t with_secnum;
    Block desc;
    Block h1;
    ArrSt(Block) *blocks;
    ArrSt(Block) *types;
    ArrSt(Block) *funcs;
};

DeclSt(Doc);

/*---------------------------------------------------------------------------*/

static void i_remove_tag(Tag *tag)
{
    str_destopt(&tag->text);
}

/*---------------------------------------------------------------------------*/

static void i_remove_block(Block *block)
{
    if (block->params != NULL)
        arrpt_destroy(&block->params, str_destroy, String);

    if (block->tags != NULL)
        arrst_destroy(&block->tags, i_remove_tag, Tag);

    if (block->alt1 != NULL)
        arrst_destroy(&block->alt1, i_remove_tag, Tag);

    if (block->alt2 != NULL)
        arrst_destroy(&block->alt2, i_remove_tag, Tag);

    if (block->children != NULL)
        arrst_destroy(&block->children, i_remove_block, Block);

    if (block->ref != NULL)
        str_destroy(&block->ref);

    if (block->code != NULL)
        stm_close(&block->code);

    if (block->ptype != NULL)
        str_destroy(&block->ptype);
}

/*---------------------------------------------------------------------------*/

static uint32_t i_seach_tag(const char_t *text, const TagId *tags, const uint32_t n, uint32_t *tag)
{
    uint32_t i, min = UINT32_MAX, stag = UINT32_MAX;
    for (i = 0; i < n; ++i)
    {
        const char_t *st = str_str(text, tags[i].tag);
        if (st != NULL)
        {
            uint32_t offset = (uint32_t)(st - text);
            if (offset < min)
            {
                min = offset;
                stag = tags[i].id;
            }
        }
    }

    *tag = stag;
    return min;
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

static void i_warn(String **str, const DocParser *parser)
{
    String *msg = NULL;
    cassert_no_null(str);
    cassert_no_null(parser);
    msg = str_printf("[%s:%s-%u]-%s.", i_lang(parser->config, parser->lang_id), tc(parser->doc->name), parser->line, tc(*str));
    nlog_warn(&msg);
    str_destroy(str);
}

/*---------------------------------------------------------------------------*/

static void i_lwarn(String **str, const char_t *docname, const uint32_t line, const char_t *lang1, const char_t *lang2)
{
    String *msg = NULL;
    cassert_no_null(str);
    msg = str_printf("[%s:%s:%s-%u]-%s.", lang1, lang2, docname, line, tc(*str));
    nlog_warn(&msg);
    str_destroy(str);
}

/*---------------------------------------------------------------------------*/

static void i_error(String **str, const DocParser *parser)
{
    String *msg = NULL;
    cassert_no_null(str);
    cassert_no_null(parser);
    msg = str_printf("[%s:%s-%u]-%s.", i_lang(parser->config, parser->lang_id), tc(parser->doc->name), parser->line, tc(*str));
    nlog_error(&msg);
    str_destroy(str);
}

/*---------------------------------------------------------------------------*/

static ArrSt(Tag) *i_parse_tags(const char_t *text, DocParser *parser)
{
    ArrSt(Tag) *tags = arrst_create(Tag);
    ArrSt(uint32_t) *tstack = arrst_create(uint32_t);
    cassert_no_null(text);

    if (unicode_isspace((uint32_t)text[0]) == TRUE)
    {
        String *str = str_printf("Block beginning with blank-space");
        i_warn(&str, parser);
    }

    for (;;)
    {
        uint32_t type;
        uint32_t offset = i_seach_tag(text, TAGS, NTAGS, &type);
        Tag *tag = arrst_new0(tags, Tag);

        /* Previous tag-text is plain-text */
        if (offset > 0)
        {
            tag->type = ekPLAINTEXT;
            if (offset == UINT32_MAX)
            {
                tag->text = str_c(text);
                if (arrst_size(tstack, uint32_t) > 0)
                {
                    uint32_t *l = arrst_last(tstack, uint32_t);
                    String *str = str_printf("Unclosed tag '%s'", TAGS[*l].tag);
                    i_warn(&str, parser);
                }
                break;
            }
            else
            {
                tag->text = str_cn(text, offset);
                text += offset;
            }
            tag = arrst_new0(tags, Tag);
        }

        tag->type = (ttype_t)type;
        text += str_len_c(TAGS[type].tag);

        switch (type)
        {
        case ekLESS:
        case ekGREATER:
            break;

        case ekBOLD_OPEN:
        case ekITALIC_OPEN:
        case ekCODE_OPEN:
        case ekSUB_OPEN:
        case ekSUP_OPEN:
        case ekUNDER_OPEN:
        case ekSTRIKE_OPEN:
        case ekMATH_OPEN:
            arrst_append(tstack, type, uint32_t);
            break;

        case ekLINK_OPEN:
        case ekLPAGE_OPEN:
        case ekLHEAD_OPEN:
        {
            uint32_t i = 0;
            const char_t *link = text;
            for (; *text != '\'' && *text != '<' && *text != '\0'; ++text, ++i)
                ;
            tag->text = str_cn(link, i);

            /* We have an alias text for link --> www.google.es''google */
            if (*text == '\'')
            {
                text++;
                if (*text != '\'')
                {
                    String *str = str_printf("Bad link separator '%c'", *text);
                    i_warn(&str, parser);
                }
                text++;
            }
            /* The visible text will be the same link text */
            else if (type != ekLPAGE_OPEN)
            {
                text = link;
            }

            arrst_append(tstack, type, uint32_t);
            break;
        }

        case ekLFUNC_OPEN:
        case ekLTYPE_OPEN:
        case ekREF_OPEN:
        {
            const char_t *end = str_str(text, TAGS[type + 1].tag);
            if (end != NULL)
            {
                tag->text = str_cn(text, (uint32_t)(end - text));
                text = end;
                text += str_len_c(TAGS[type + 1].tag);

                /* if (tag->type == ekREF_OPEN)
                {
                   String *tag_text = str_printf("%s::%s", tc(parser->doc->packname), tc(tag->text));
                   str_destroy(&tag->text);
                   tag->text = tag_text;
                } */
            }
            else
            {
                String *str = str_printf("Unclosed tag '%s'", TAGS[type].tag);
                i_warn(&str, parser);
            }
            break;
        }

        case ekBOLD_CLOSE:
        case ekITALIC_CLOSE:
        case ekCODE_CLOSE:
        case ekSUB_CLOSE:
        case ekSUP_CLOSE:
        case ekUNDER_CLOSE:
        case ekSTRIKE_CLOSE:
        case ekMATH_CLOSE:
        case ekLINK_CLOSE:
        case ekLPAGE_CLOSE:
        case ekLHEAD_CLOSE:
        {
            uint32_t *prev = NULL;
            if (arrst_size(tstack, uint32_t) > 0)
                prev = arrst_last(tstack, uint32_t);
            if (prev == NULL || *prev != type - 1)
            {
                String *str = str_printf("Unexpected tag '%s'", TAGS[type].tag);
                i_warn(&str, parser);
            }
            else
            {
                uint32_t l = arrst_size(tstack, uint32_t);
                arrst_delete(tstack, l - 1, NULL, uint32_t);
            }

            break;
        }

        case ekLFUNC_CLOSE:
        case ekLTYPE_CLOSE:
        case ekREF_CLOSE:
        {
            String *str = str_printf("Unexpected tag '%s'", TAGS[type].tag);
            i_warn(&str, parser);
            break;
        }

        default:
            cassert_default(type);
        }
    }

    arrst_destroy(&tstack, NULL, uint32_t);
    return tags;
}

/*---------------------------------------------------------------------------*/

static ArrPt(String) *i_parse_params(const char_t **dtext, DocParser *parser)
{
    const char_t *text = *dtext;
    ArrPt(String) *params = NULL;
    if (text[0] != '\0' && text[0] == '(')
    {
        const char_t *param = NULL;
        uint32_t p = 0;
        params = arrpt_create(String);
        text++;
        param = text;
        while (*text != '\0')
        {
            if (*text == ',' || (*text == ')' && p == 0))
            {
                String *sparam = str_trim_n(param, (uint32_t)(text - param));
                arrpt_append(params, sparam, String);
                text++;
                param = text;
                if (text[-1] == ')')
                    break;
            }
            else if (*text == '(')
            {
                p += 1;
                text++;
            }
            else if (*text == ')')
            {
                cassert(p > 0);
                p -= 1;
                text++;
            }
            else
            {
                text++;
            }
        }

        if (*text == '\0')
        {
            String *str = str_c("Unexpected end of line procesing block parameters");
            i_warn(&str, parser);
        }
        else if (*text == '.')
        {
            text++;
        }
        else
        {
            String *str = str_c("Expected '.' after block parameters");
            i_warn(&str, parser);
        }
    }
    /* No params */
    else if (text[0] != '\0' && text[0] == '.')
    {
        text++;
    }
    else
    {
        String *str = str_c("Expected '.' after block tag");
        i_warn(&str, parser);
    }

    *dtext = text;
    return params;
}

/*---------------------------------------------------------------------------*/

static bool_t i_parse_line(const char_t *text, Block *block, DocParser *parser)
{
    uint32_t i;

    cassert_no_null(text);
    bmem_zero(block, Block);
    block->line = parser->line;

    if (text[0] == '\0')
        return FALSE;

    if (text[0] == '#')
        return FALSE;

    if (text[0] == '/' && text[1] != '\0' && text[1] == '/')
        return FALSE;

    for (i = 0; i < NBLOCKS; ++i)
    {
        uint32_t blen = str_len_c(BLOCKS[i].block);
        if (str_equ_cn(text, BLOCKS[i].block, blen))
        {
            if (text[blen] == '(' || text[blen] == '.')
            {
                text += blen;
                block->type = BLOCKS[i].id;
                block->params = i_parse_params(&text, parser);
                block->tags = i_parse_tags(text, parser);
                break;
            }
        }
    }

    if (i == NBLOCKS)
    {
        cassert(block->type == 0);
        block->type = ekNOTAG;
        block->tags = i_parse_tags(text, parser);
    }

    return TRUE;
}

/*---------------------------------------------------------------------------*/

static void i_set_alt(ArrSt(Tag) **alt, ArrSt(Tag) **tags, DocParser *parser)
{
    cassert_no_null(alt);
    cassert_no_null(tags);
    cassert_no_null(*tags);

    if (*alt != NULL)
    {
        String *str = str_c("Unexpected unlabeled block");
        i_warn(&str, parser);
    }
    else
    {
        *alt = *tags;
        *tags = NULL;
    }
}

/*---------------------------------------------------------------------------*/

static void i_check_img(const Block *block, const uint32_t index, const char_t *imgpath, ArrPt(String) *pack_images, DocParser *parser)
{
    if (block->params == NULL || arrpt_size(block->params, String) <= index)
    {
        String *str = str_c("Image file not specified");
        i_warn(&str, parser);
    }
    else
    {
        const String *imname = arrpt_get(block->params, index, String);
        const char_t *ext = str_filext(tc(imname));
        String *img = str_cpath("%s/%s", imgpath, tc(imname));

        if (!ext ||
            (!str_equ_nocase(ext, "png") && !str_equ_nocase(ext, "jpg") && !str_equ_nocase(ext, "gif") && !str_equ_nocase(ext, "svg")))
        {
            String *str = str_printf("Image type '%s' not supported (png,jpg,gif,svg)", ext);
            i_error(&str, parser);
        }

        if (hfile_exists(tc(img), NULL) == FALSE)
        {
            String *str = str_printf("Image '%s' not exists", tc(imname));
            i_error(&str, parser);
        }
        else
        {
            /* Deleting the image from 'pack_images' is mark the image as used */
            uint32_t pos = UINT32_MAX;
            if (arrpt_search(pack_images, str_cmp, tc(imname), &pos, String, char_t) != NULL)
            {
                String *basefile = NULL;
                arrpt_delete(pack_images, pos, str_destroy, String);

                str_split_pathext(tc(imname), NULL, &basefile, NULL);

                /* Gifs images must have a .png "mirror" for ebook version */
                if (str_equ_nocase(ext, "gif") == TRUE)
                {
                    String *png_image = str_printf("%s.png", tc(basefile));
                    if (arrpt_search(pack_images, str_cmp, tc(png_image), &pos, String, char_t) != NULL)
                    {
                        arrpt_delete(pack_images, pos, str_destroy, String);
                    }
                    /*
                    * LaTeX module will warn about the .png mirror image (if LaTeX is used)
                    *
                    else
                    {
                        String *msg = str_printf("Image '%s' doesn't a .png mirror image", tc(imname));
                        i_warn(&msg, parser);
                    }
                    */
                    str_destroy(&png_image);
                }

                /* An specialiced image con be defined for ebooks */
                {
                    String *ebook_image = str_printf("%s_book.%s", tc(basefile), ext);
                    if (arrpt_search(pack_images, str_cmp, tc(ebook_image), &pos, String, char_t) != NULL)
                        arrpt_delete(pack_images, pos, str_destroy, String);
                    str_destroy(&ebook_image);
                }

                str_destroy(&basefile);
            }
        }

        str_destroy(&img);
    }
}

/*---------------------------------------------------------------------------*/

static const char_t *i_param(const Block *block, const uint32_t index)
{
    cassert_no_null(block);
    if (block->params != NULL)
    {
        if (arrpt_size(block->params, String) > index)
        {
            const String *param = arrpt_get(block->params, index, String);
            return tc(param);
        }
    }
    return NULL;
}

/*---------------------------------------------------------------------------*/

static real32_t i_real_param(const Block *block, const uint32_t index)
{
    cassert_no_null(block);
    if (block->params != NULL)
    {
        if (arrpt_size(block->params, String) > index)
        {
            const String *param = arrpt_get(block->params, index, String);
            return str_to_r32(tc(param), NULL);
        }
    }
    return -1.f;
}

/*---------------------------------------------------------------------------*/

static void i_check_limit(const Block *block, const uint32_t index, const real32_t max, DocParser *parser)
{
    real32_t value = i_real_param(block, index);
    if (value > max)
    {
        String *str = str_printf("Image LaTeX width '%.2f' is too big (<=1)", value);
        i_warn(&str, parser);
    }
}

/*---------------------------------------------------------------------------*/

static void i_check_imgs(const Block *block, const char_t *imgpath, ArrPt(String) *pack_images, DocParser *parser)
{
    cassert_no_null(block);
    if (block->type == ekIMG)
    {
        i_check_img(block, 0, imgpath, pack_images, parser);
        i_check_limit(block, 2, 1.f, parser);
    }
    else if (block->type == ekIMG2)
    {
        i_check_img(block, 0, imgpath, pack_images, parser);
        i_check_img(block, 1, imgpath, pack_images, parser);
    }
    else
    {
        cassert(FALSE);
    }
}

/*---------------------------------------------------------------------------*/

static void i_fparam(Block *block, const bool_t is_return, DocParser *parser)
{
    const char_t *param = NULL;
    cassert_no_null(block);
    cassert(block->ptype == NULL);
    param = i_param(block, 0);
    if (param != NULL)
    {
        block->is_const = (bool_t)(str_str(param, "const") != NULL);
        block->is_dptr = (bool_t)(str_str(param, "**") != NULL);
        block->is_ptr = (bool_t)(str_str(param, "*") != NULL);
        block->ptype = str_repl(param, "const", "", "**", "", "*", "", " ", "", 0);
    }
    else
    {
        String *warn = str_c("Function parameter 'type' not provided");
        i_warn(&warn, parser);
    }

    if (is_return == FALSE)
    {
        if (i_param(block, 1) == NULL)
        {
            String *warn = str_c("Function parameter value not provided");
            i_warn(&warn, parser);
        }
    }
}

/*---------------------------------------------------------------------------*/

static const Block *i_block(ArrSt(Block) *blocks, const btype_t type)
{
    arrst_foreach(block, blocks, Block)
        if (block->type == type)
            return block;
    arrst_end()
    return NULL;
}

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

static Stream *i_codefile(const char_t *src_repo_url, const char_t *src_repo_user, const char_t *src_repo_pass, const uint32_t src_repo_vers, const char_t *filename)
{
    String *path = str_printf("%s/%s", src_repo_url, filename);
    Stream *stm = ssh_repo_cat(tc(path), src_repo_vers, src_repo_user, src_repo_pass);
    str_destroy(&path);
    return stm;
}

/*---------------------------------------------------------------------------*/

Doc *doc_create(
    const char_t *docpath,
    const char_t *src_repo_url,
    const char_t *src_repo_user,
    const char_t *src_repo_pass,
    const uint32_t src_repo_vers,
    const char_t *packname,
    const char_t *docname,
    const char_t *imgpath,
    const Config *config,
    const uint32_t lang_id,
    ArrPt(String) *pack_images,
    Loader *loader)
{
    String *dpath = str_cpath("%s/%s/%s/%s.htm", docpath, packname, i_lang(config, lang_id), docname);
    Stream *stm = stm_from_file(tc(dpath), NULL);
    Doc *doc = NULL;
    DocParser parser;
    uint32_t incode = UINT32_MAX;
    uint32_t imgid = 0;
    uint32_t tableid = 0;
    uint32_t listid = 0;
    uint32_t mathid = 0;
    uint32_t secid = 0;
    uint32_t subsecid = 0;
    uint32_t typesid = 0;
    uint32_t constid = 0;
    uint32_t enumid = 0;
    uint32_t structid = 0;
    uint32_t funcid = 0;
    Block *last = NULL;
    uint32_t last_id = UINT32_MAX;
    doc = heap_new0(Doc);
    doc->type = ekDOCPOST;
    doc->state = ekCOMPLETE;
    doc->packname = str_c(packname);
    doc->name = str_c(docname);
    doc->with_toc = TRUE;
    doc->with_secnum = TRUE;
    doc->blocks = arrst_create(Block);
    doc->types = arrst_create(Block);
    doc->funcs = arrst_create(Block);

    bmem_zero(&parser, DocParser);
    parser.loader = loader;
    parser.config = config;
    parser.lang_id = lang_id;
    parser.doc = doc;
    parser.line = 1;

    if (stm == NULL)
    {
        Tag *tag;
        String *warn = str_printf("Error loading '%s' document", tc(dpath));
        i_error(&warn, &parser);
        doc->state = ekMISSING;
        doc->h1.type = ekH1;
        doc->h1.tags = arrst_create(Tag);
        tag = arrst_new(doc->h1.tags, Tag);
        tag->type = ekPLAINTEXT;
        tag->text = str_c(docname);
        goto __endparsing;
    }

    stm_lines(text, stm)
        if (incode != UINT32_MAX)
        {
            cassert_no_null(last);
            if (last->type == ekCODE)
            {
                if (str_equ_cn(text, "code.", 5) == TRUE)
                {
                    incode = UINT32_MAX;
                }
                else
                {
                    stm_writef(last->code, text);
                    stm_writef(last->code, "\n");
                }
            }
            else if (last->type == ekMATH)
            {
                if (str_equ_cn(text, "math.", 5) == TRUE)
                {
                    incode = UINT32_MAX;
                }
                else
                {
                    stm_writef(last->code, text);
                    stm_writef(last->code, "\n");
                }
            }
            else
            {
                cassert(last->type == ekFUNC);
                if (str_equ_cn(text, "fcode.", 6) == TRUE)
                {
                    incode = UINT32_MAX;
                }
                else
                {
                    stm_writef(last->code, text);
                    stm_writef(last->code, "\n");
                }
            }
        }
        else
        {
            Block block;
            if (i_parse_line(text, &block, &parser) == TRUE)
            {
                if (block.type == ekNOTAG)
                {
                    if (last == NULL)
                    {
                        String *str = str_c("Unexpected unlabeled block");
                        i_warn(&str, &parser);
                    }
                    else if (last->type == ekIMG || last->type == ekEPIG)
                    {
                        i_set_alt(&last->alt1, &block.tags, &parser);
                    }
                    else if (last->type == ekIMG2)
                    {
                        if (last->alt1 == NULL)
                            i_set_alt(&last->alt1, &block.tags, &parser);
                        else
                            i_set_alt(&last->alt2, &block.tags, &parser);
                    }
                    else
                    {
                        String *str = str_c("Unexpected unlabeled block");
                        i_warn(&str, &parser);
                    }
                    i_remove_block(&block);
                }
                else if (last != NULL && last->type == ekTABLE)
                {
                    if (block.type != ekTABLE)
                    {
                        if (block.type == ekIMG)
                        {
                            i_check_imgs(&block, imgpath, pack_images, &parser);
                            arrst_append(last->children, block, Block);
                        }
                        else if (block.type == ekPARAG || block.type == ekROW)
                        {
                            arrst_append(last->children, block, Block);
                        }
                        else
                        {
                            String *str = str_printf("Unexpected block in table");
                            i_warn(&str, &parser);
                            i_remove_block(&block);
                        }
                    }
                    else
                    {
                        i_remove_block(&block);
                        last = NULL;
                    }
                }
                else
                {
                    switch (block.type)
                    {
                    case ekCODE:
                    {
                        const char_t *clabel = doc_block_param(&block, 1);
                        cassert(block.code == NULL);
                        cassert(incode == UINT32_MAX);
                        cassert(block.ref == NULL);
                        if (str_empty_c(clabel) == FALSE)
                        {
                            listid += 1;
                            block.ref = str_printf("%d", listid);
                        }

                        incode = parser.line;
                        block.code = stm_memory(512);
                        last = arrst_new(doc->blocks, Block);
                        *last = block;
                        break;
                    }

                    case ekCODEFILE:
                    {
                        const char_t *filename = i_param(&block, 0);
                        cassert(block.code == NULL);
                        cassert(incode == UINT32_MAX);
                        cassert(block.ref == NULL);
                        listid += 1;
                        block.ref = str_printf("%d", listid);
                        if (filename != NULL)
                        {
                            if (src_repo_url != NULL)
                            {
                                block.code = i_codefile(src_repo_url, src_repo_user, src_repo_pass, src_repo_vers, filename);
                                if (block.code == NULL)
                                {
                                    String *warn = str_printf("codefile. Error loading '%s'", filename);
                                    i_error(&warn, &parser);
                                }
                                else
                                {
                                    if (i_with_text(block.tags) == FALSE)
                                    {
                                        Tag *tag = arrst_prepend_n(block.tags, 1, Tag);
                                        tag->type = ekPLAINTEXT;
                                        tag->text = str_c(filename);
                                    }
                                }
                            }
                            else
                            {
                                String *warn = str_c("codefile. Unknown file repository");
                                i_error(&warn, &parser);
                            }
                        }
                        else
                        {
                            String *warn = str_c("codefile. Unknown file");
                            i_error(&warn, &parser);
                        }

                        arrst_append(doc->blocks, block, Block);
                        last = NULL;
                        break;
                    }

                    case ekMATH:
                        cassert(block.code == NULL);
                        cassert(incode == UINT32_MAX);
                        cassert(block.ref == NULL);
                        mathid += 1;
                        block.ref = str_printf("%d", mathid);
                        incode = parser.line;
                        block.code = stm_memory(512);
                        last = arrst_new(doc->blocks, Block);
                        *last = block;
                        break;

                    case ekBLANGREV:
                        doc->state = ekLANGREV;
                        i_remove_block(&block);
                        last = NULL;
                        break;

                    case ekBCOMPLETE:
                        doc->state = ekCOMPLETE;
                        i_remove_block(&block);
                        last = NULL;
                        break;

                    case ekBDRAFT:
                        doc->state = ekDRAFT;
                        i_remove_block(&block);
                        last = NULL;
                        break;

                    case ekNOTOC:
                        if (doc->with_toc == TRUE)
                        {
                            doc->with_toc = FALSE;
                        }
                        else
                        {
                            String *warn = str_c("notoc. Ignored. Only one allowed");
                            i_warn(&warn, &parser);
                        }
                        i_remove_block(&block);
                        last = NULL;
                        break;

                    case ekNOSECNUM:
                        if (doc->with_secnum == TRUE)
                        {
                            doc->with_secnum = FALSE;
                        }
                        else
                        {
                            String *warn = str_c("nosecnum. Ignored. Only one allowed");
                            i_warn(&warn, &parser);
                        }
                        i_remove_block(&block);
                        last = NULL;
                        break;

                    case ekWARN:
                    {
                        const Tag *tag = arrst_get(block.tags, 0, Tag);
                        String *str = str_printf("WARN! %s", tc(tag->text));
                        i_warn(&str, &parser);
                        i_remove_block(&block);
                        last = NULL;
                        break;
                    }

                    case ekIMG:
                    case ekIMG2:
                        cassert(block.ref == NULL);
                        imgid += 1;
                        block.ref = str_printf("%d", imgid);
                        i_check_imgs(&block, imgpath, pack_images, &parser);
                        last = arrst_new(doc->blocks, Block);
                        *last = block;
                        break;

                    case ekTABLE:
                        cassert(block.ref == NULL);
                        tableid += 1;
                        block.ref = str_printf("%d", tableid);
                        block.children = arrst_create(Block);
                        last = arrst_new(doc->blocks, Block);
                        *last = block;
                        break;

                    case ekH1:
                        if (doc->h1.type == ekH1)
                        {
                            String *warn = str_c("h1. Ignored. Only one allowed");
                            i_warn(&warn, &parser);
                            i_remove_block(&block);
                        }
                        else
                        {
                            cassert(doc->h1.type == 0);
                            doc->h1 = block;
                        }
                        last = NULL;
                        break;

                    case ekH2:
                    case ekH3:
                    {
                        DIndex *dindex = loader_section_index(loader);
                        dindex->doc = doc;
                        dindex->type = block.type;
                        dindex->block_id = arrst_size(doc->blocks, Block);
                        dindex->child_id = UINT32_MAX;
                        dindex->name = i_plain_str(block.tags);
                        cassert(block.ref == NULL);
                        if (block.type == ekH2)
                        {
                            secid += 1;
                            subsecid = 0;
                            block.ref = str_printf("h%d", secid);
                        }
                        else
                        {
                            subsecid += 1;
                            block.ref = str_printf("h%d.%d", secid, subsecid);
                        }

                        arrst_append(doc->blocks, block, Block);
                        last = NULL;
                        break;
                    }

                    case ekEPIG:
                        if (i_block(doc->blocks, ekEPIG) != NULL)
                        {
                            String *str = str_c("epig. Ignored. Only one allowed");
                            i_warn(&str, &parser);
                        }

                        last = arrst_new(doc->blocks, Block);
                        cassert(last != NULL);
                        *last = block;
                        break;

                    case ekDESC:
                        if (doc->desc.type == ekDESC)
                        {
                            String *warn = str_c("desc. Ignored. Only one allowed");
                            i_warn(&warn, &parser);
                            i_remove_block(&block);
                        }
                        else
                        {
                            cassert(doc->desc.type == 0);
                            doc->desc = block;
                        }
                        last = NULL;
                        break;

                    case ekTYPE:
                    {
                        const char_t *param = i_param(&block, 0);
                        cassert(block.ref == NULL);
                        typesid += 1;
                        block.ref = str_printf("t%d", typesid);
                        if (param != NULL)
                        {
                            DIndex *dindex = loader_type_index(loader);
                            dindex->doc = doc;
                            dindex->type = block.type;
                            dindex->block_id = arrst_size(doc->types, Block);
                            dindex->child_id = UINT32_MAX;
                            dindex->name = str_c(param);
                            arrst_append(doc->types, block, Block);
                        }
                        else
                        {
                            String *warn = str_c("'type' not provided");
                            i_warn(&warn, &parser);
                            i_remove_block(&block);
                        }

                        last = NULL;
                        break;
                    }

                    case ekCONST:
                    {
                        const char_t *type = i_param(&block, 0);
                        const char_t *name = i_param(&block, 1);
                        cassert(block.ref == NULL);
                        constid += 1;
                        block.ref = str_printf("c%d", constid);

                        if (type == NULL)
                        {
                            String *warn = str_c("Constant 'type' not provided");
                            i_warn(&warn, &parser);
                        }

                        if (i_param(&block, 2) == NULL)
                        {
                            /* String *warn = str_c("Constant 'value' not provided");
                            i_warn(&warn, &parser); */
                        }

                        if (type != NULL && name != NULL)
                        {
                            if (doc_is_real_type(type, loader) == TRUE)
                            {
                                /* Template index entr */
                                DIndex *dindex = loader_type_index(loader);
                                dindex->doc = doc;
                                dindex->type = block.type;
                                dindex->block_id = arrst_size(doc->types, Block);
                                dindex->child_id = UINT32_MAX;
                                dindex->name = doc_template_constant(type, name);

                                /* Generic index entry */
                                {
                                    DIndex *dindex2 = loader_type_index(loader);
                                    dindex2->doc = doc;
                                    dindex2->type = block.type;
                                    dindex2->block_id = arrst_size(doc->types, Block);
                                    dindex2->child_id = UINT32_MAX;
                                    dindex2->name = doc_real_constant(name);
                                }

                                /* Float index entry */
                                {
                                    DIndex *dindex2 = loader_type_index(loader);
                                    dindex2->doc = doc;
                                    dindex2->type = block.type;
                                    dindex2->block_id = arrst_size(doc->types, Block);
                                    dindex2->child_id = UINT32_MAX;
                                    dindex2->name = doc_float_constant(type, name);
                                }

                                /* Double index entry */
                                {
                                    DIndex *dindex2 = loader_type_index(loader);
                                    dindex2->doc = doc;
                                    dindex2->type = block.type;
                                    dindex2->block_id = arrst_size(doc->types, Block);
                                    dindex2->child_id = UINT32_MAX;
                                    dindex2->name = doc_double_constant(type, name);
                                }
                            }
                            /* Other type constant */
                            else
                            {
                                DIndex *dindex = loader_type_index(loader);
                                dindex->doc = doc;
                                dindex->type = block.type;
                                dindex->block_id = arrst_size(doc->types, Block);
                                dindex->child_id = UINT32_MAX;
                                dindex->name = str_c(name);
                            }

                            arrst_append(doc->types, block, Block);
                        }
                        else
                        {
                            String *warn = str_c("Constant 'name' not provided");
                            i_warn(&warn, &parser);
                            i_remove_block(&block);
                        }

                        last = NULL;
                        break;
                    }

                    case ekENUM:
                    {
                        const char_t *name = i_param(&block, 0);
                        cassert(block.ref == NULL);
                        enumid += 1;
                        block.ref = str_printf("e%d", enumid);

                        if (name != NULL)
                        {
                            DIndex *dindex = loader_type_index(loader);
                            dindex->doc = doc;
                            dindex->type = block.type;
                            dindex->block_id = arrst_size(doc->types, Block);
                            dindex->child_id = UINT32_MAX;
                            dindex->name = str_c(name);
                            last_id = dindex->block_id;
                            cassert(block.children == NULL);
                            block.children = arrst_create(Block);
                            last = arrst_new(doc->types, Block);
                            *last = block;
                        }
                        else
                        {
                            String *warn = str_c("Enum 'type' not provided");
                            i_warn(&warn, &parser);
                            i_remove_block(&block);
                            last = NULL;
                        }
                        break;
                    }

                    case ekENUMV:
                        if (last != NULL && last->type == ekENUM)
                        {
                            const char_t *value = i_param(&block, 0);
                            if (value != NULL)
                            {
                                DIndex *dindex = loader_type_index(loader);
                                dindex->doc = doc;
                                dindex->type = block.type;
                                dindex->block_id = last_id;
                                dindex->child_id = arrst_size(last->children, Block);
                                dindex->name = str_c(value);
                                cassert(block.ref == NULL);
                                block.ref = str_copy(last->ref);
                                arrst_append(last->children, block, Block);
                            }
                            else
                            {
                                String *warn = str_c("Enum 'value' not provided");
                                i_warn(&warn, &parser);
                                i_remove_block(&block);
                            }
                        }
                        else
                        {
                            String *str = str_c("Unexpected 'enumv' block (no 'enum' defined)");
                            i_warn(&str, &parser);
                            i_remove_block(&block);
                        }
                        break;

                    case ekSTRUCT:
                    {
                        const char_t *name = i_param(&block, 0);
                        const char_t *type = i_param(&block, 1);
                        cassert(block.ref == NULL);
                        structid += 1;
                        block.ref = str_printf("s%d", structid);

                        if (name != NULL)
                        {
                            /* Real-base math struct */
                            if (type != NULL && str_equ_c(type, "real"))
                            {
                                /* Template index entry */
                                DIndex *dindex = loader_type_index(loader);
                                dindex->doc = doc;
                                dindex->type = block.type;
                                dindex->block_id = arrst_size(doc->types, Block);
                                dindex->child_id = UINT32_MAX;
                                dindex->name = doc_template_struct(name);

                                /* Float index entry */
                                {
                                    DIndex *dindex2 = loader_type_index(loader);
                                    dindex2->doc = doc;
                                    dindex2->type = block.type;
                                    dindex2->block_id = arrst_size(doc->types, Block);
                                    dindex2->child_id = UINT32_MAX;
                                    dindex2->name = doc_float_struct(name);
                                }

                                /* Double index entry */
                                {
                                    DIndex *dindex2 = loader_type_index(loader);
                                    dindex2->doc = doc;
                                    dindex2->type = block.type;
                                    dindex2->block_id = arrst_size(doc->types, Block);
                                    dindex2->child_id = UINT32_MAX;
                                    dindex2->name = doc_double_struct(name);
                                }
                            }
                            else if (type == NULL)
                            {
                                DIndex *dindex = loader_type_index(loader);
                                dindex->doc = doc;
                                dindex->type = block.type;
                                dindex->block_id = arrst_size(doc->types, Block);
                                dindex->child_id = UINT32_MAX;
                                dindex->name = str_c(name);
                            }
                            else
                            {
                                String *warn = str_c("Struct invalid 'type' param");
                                i_warn(&warn, &parser);
                            }

                            cassert(block.children == NULL);
                            block.children = arrst_create(Block);
                            last = arrst_new(doc->types, Block);
                            *last = block;
                        }
                        else
                        {
                            String *warn = str_c("Struct 'type' not provided");
                            i_warn(&warn, &parser);
                            i_remove_block(&block);
                        }
                        break;
                    }

                    case ekSMEMBER:
                        if (last != NULL && last->type == ekSTRUCT)
                        {
                            const char_t *type = i_param(&block, 0);
                            const char_t *name = i_param(&block, 1);
                            if (type == NULL)
                            {
                                String *warn = str_c("Struct member 'type' not provided");
                                i_warn(&warn, &parser);
                            }

                            if (name == NULL)
                            {
                                String *warn = str_c("Struct member 'name' not provided");
                                i_warn(&warn, &parser);
                            }

                            arrst_append(last->children, block, Block);
                        }
                        else
                        {
                            String *str = str_c("Unexpected 'smember' block (no 'struct' defined)");
                            i_warn(&str, &parser);
                            i_remove_block(&block);
                        }
                        break;

                    case ekFUNC:
                    {
                        const char_t *fname = i_param(&block, 0);
                        const char_t *falias = i_param(&block, 1);
                        bool_t isreal = FALSE;
                        cassert(block.ref == NULL);
                        funcid += 1;
                        block.ref = str_printf("f%d", funcid);

                        if (falias != NULL)
                        {
                            if (str_equ_c(falias, "ptr") == TRUE)
                                block.is_ptr = TRUE;
                            else
                                isreal = TRUE;
                        }

                        if (fname != NULL)
                        {
                            if (isreal == TRUE)
                            {
                                /* Template index entry */
                                DIndex *dindex = loader_function_index(loader);
                                dindex->doc = doc;
                                dindex->type = block.type;
                                dindex->block_id = arrst_size(doc->funcs, Block);
                                dindex->child_id = UINT32_MAX;
                                dindex->name = doc_template_func(fname, falias);

                                /*
                                Generic index entry
                                Deshabilitado porque pilla nombres de variable muy genéricos como:
                                'max', 'min', 'center', 'box', etc...
                                {
                                   DIndex *dindex2 = loader_function_index(loader);
                                   *dindex2 = *dindex;
                                   dindex2->name = str_c(falias);
                                }
                                */

                                /* Float index entry */
                                {
                                    DIndex *dindex2 = loader_function_index(loader);
                                    dindex2->doc = doc;
                                    dindex2->type = block.type;
                                    dindex2->block_id = arrst_size(doc->funcs, Block);
                                    dindex2->child_id = UINT32_MAX;
                                    dindex2->name = doc_float_func(fname, falias);
                                }

                                /* Double index entry */
                                {
                                    DIndex *dindex2 = loader_function_index(loader);
                                    dindex2->doc = doc;
                                    dindex2->type = block.type;
                                    dindex2->block_id = arrst_size(doc->funcs, Block);
                                    dindex2->child_id = UINT32_MAX;
                                    dindex2->name = doc_double_func(fname, falias);
                                }
                            }
                            else
                            {
                                DIndex *dindex = loader_function_index(loader);
                                dindex->doc = doc;
                                dindex->type = block.type;
                                dindex->block_id = arrst_size(doc->funcs, Block);
                                dindex->child_id = UINT32_MAX;
                                dindex->name = str_c(fname);
                            }

                            cassert(block.children == NULL);
                            block.children = arrst_create(Block);
                            last = arrst_new(doc->funcs, Block);
                            *last = block;
                        }
                        else
                        {
                            String *warn = str_c("Function 'name' not provided");
                            i_warn(&warn, &parser);
                            i_remove_block(&block);
                            last = NULL;
                        }

                        break;
                    }

                    case ekFRET:
                        if (last != NULL && last->type == ekFUNC)
                        {
                            i_fparam(&block, TRUE, &parser);
                            if (arrst_size(last->children, Block) > 0)
                            {
                                String *warn = str_c("Only one 'fret' is allowed as first function param");
                                i_warn(&warn, &parser);
                            }
                            arrst_append(last->children, block, Block);
                        }
                        else
                        {
                            String *str = str_c("Unexpected 'fret' block (no 'func' defined)");
                            i_warn(&str, &parser);
                            i_remove_block(&block);
                        }
                        break;

                    case ekFPAR:
                        if (last != NULL && last->type == ekFUNC)
                        {
                            i_fparam(&block, FALSE, &parser);
                            arrst_append(last->children, block, Block);
                        }
                        else
                        {
                            String *str = str_c("Unexpected 'fpar' block (no 'func' defined)");
                            i_warn(&str, &parser);
                            i_remove_block(&block);
                        }
                        break;

                    case ekFCODE:
                        if (last != NULL && last->type == ekFUNC)
                        {
                            if (last->code == NULL)
                            {
                                cassert(incode == UINT32_MAX);
                                incode = parser.line;
                                last->code = stm_memory(512);
                            }
                            else
                            {
                                String *str = str_c("Only one 'fcode' block is allowed");
                                i_warn(&str, &parser);
                            }
                        }
                        else
                        {
                            String *str = str_c("Unexpected 'fcode' block (no 'func' defined)");
                            i_warn(&str, &parser);
                        }
                        i_remove_block(&block);
                        break;

                    case ekFNOTE:
                        if (last != NULL && last->type == ekFUNC)
                        {
                            if (last->alt1 == NULL)
                            {
                                last->alt1 = block.tags;
                                block.tags = NULL;
                            }
                            else
                            {
                                String *str = str_c("Only one 'fnote' block is allowed");
                                i_warn(&str, &parser);
                            }
                        }
                        else
                        {
                            String *str = str_c("Unexpected 'fnote' block (no 'func' defined)");
                            i_warn(&str, &parser);
                        }
                        i_remove_block(&block);
                        break;

                    case ekLI:
                    case ekLILI:
                    {
                        const char_t *icon = doc_block_param(&block, 0);
                        if (icon != NULL)
                            i_check_img(&block, 0, imgpath, pack_images, &parser);

                        arrst_append(doc->blocks, block, Block);
                        last = NULL;
                        break;
                    }

                    case ekPARAG:
                    case ekROW:
                    case ekBQ:
                    case ekNOTAG:
                    case ekEBOOK_ONLY:
                    case ekWEB_ONLY:
                    default:
                        arrst_append(doc->blocks, block, Block);
                        last = NULL;
                        break;
                    }
                }
            }
            else
            {
                last = NULL;
            }
        }
        parser.line += 1;
    stm_next(text, stm)

    if (doc->h1.type == 0)
    {
        Tag *tag;
        String *str = str_c("h1. block is requiered");
        i_warn(&str, &parser);
        doc->h1.type = ekH1;
        doc->h1.tags = arrst_create(Tag);
        tag = arrst_new(doc->h1.tags, Tag);
        tag->type = ekPLAINTEXT;
        tag->text = str_c(docname);
    }

    if (incode != UINT32_MAX)
    {
        String *str = NULL;
        if (last->type == ekCODE)
            str = str_c("Unclosed .code block");
        else
            str = str_c("Unclosed .fcode block");
        i_warn(&str, &parser);
    }

__endparsing:

    str_destroy(&dpath);
    ptr_destopt(stm_close, &stm, Stream);
    return doc;
}

/*---------------------------------------------------------------------------*/

static void i_compare_block(const Block *block1, const Block *block2, const char_t *docname, const char_t *lang1, const char_t *lang2)
{
    cassert_no_null(block1);
    cassert_no_null(block2);
    if (block1->line != block2->line)
    {
        String *str = str_printf("Block line number '%u'", block2->line);
        i_lwarn(&str, docname, block1->line, lang1, lang2);
    }

    if (block1->type != block2->type)
    {
        String *str = str_printf("Block type '%u-%u'", block1->type, block2->type);
        i_lwarn(&str, docname, block1->line, lang1, lang2);
        return;
    }

    if (block1->params != NULL && block2->params != NULL)
    {
        String **str1 = arrpt_all(block1->params, String);
        String **str2 = arrpt_all(block2->params, String);
        uint32_t i, n = arrpt_size(block1->params, String);
        if (n == arrpt_size(block2->params, String))
        {
            for (i = 0; i < n; ++i, ++str1, ++str2)
            {
                if (str_equ(*str1, tc(*str2)) == FALSE)
                {
                    String *str = str_printf("Block params");
                    i_lwarn(&str, docname, block1->line, lang1, lang2);
                }
            }
        }
        else
        {
            String *str = str_printf("Block params");
            i_lwarn(&str, docname, block1->line, lang1, lang2);
        }
    }
    else if (block1->params != NULL || block2->params != NULL)
    {
        String *str = str_printf("Block params");
        i_lwarn(&str, docname, block1->line, lang1, lang2);
    }

    if (str_equ(block1->ref, tc(block2->ref)) == FALSE)
    {
        String *str = str_printf("Block ref");
        i_lwarn(&str, docname, block1->line, lang1, lang2);
    }

    if (block1->code != NULL)
    {
        if (block2->code != NULL)
        {
            const char_t *data1 = (const char_t *)stm_buffer(block1->code);
            const char_t *data2 = (const char_t *)stm_buffer(block2->code);
            uint32_t size1 = stm_buffer_size(block1->code);
            uint32_t size2 = stm_buffer_size(block1->code);
            if (size1 != size2 || str_equ_cn(data1, data2, size1) == FALSE)
            {
                String *str = str_printf("Block source code");
                i_lwarn(&str, docname, block1->line, lang1, lang2);
            }
        }
        else
        {
            String *str = str_printf("Block source code");
            i_lwarn(&str, docname, block1->line, lang1, lang2);
        }
    }
    else if (block2->code != NULL)
    {
        String *str = str_printf("Block source code");
        i_lwarn(&str, docname, block1->line, lang1, lang2);
    }

    if (block1->is_const != block2->is_const || block1->is_ptr != block2->is_ptr || block1->is_dptr != block2->is_dptr || str_equ(block1->ptype, tc(block2->ptype)) == FALSE)
    {
        String *str = str_printf("Block func param");
        i_lwarn(&str, docname, block1->line, lang1, lang2);
    }

    if (block1->children != NULL && block2->children != NULL)
    {
        const Block *child1 = arrst_all(block1->children, Block);
        const Block *child2 = arrst_all(block2->children, Block);
        uint32_t n1 = arrst_size(block1->children, Block);
        uint32_t n2 = arrst_size(block2->children, Block);
        if (n1 == n2)
        {
            uint32_t i;
            for (i = 0; i < n1; ++i, ++child1, ++child2)
                i_compare_block(child1, child2, docname, lang1, lang2);
        }
        else
        {
            String *str = str_printf("Block children");
            i_lwarn(&str, docname, block1->line, lang1, lang2);
        }
    }
    else if (block1->children != NULL || block2->children != NULL)
    {
        String *str = str_printf("Block children");
        i_lwarn(&str, docname, block1->line, lang1, lang2);
    }
}

/*---------------------------------------------------------------------------*/

static void i_compare_blocks(const ArrSt(Block) *blocks1, const ArrSt(Block) *blocks2, const char_t *docname, const char_t *blockref, const char_t *lang1, const char_t *lang2)
{
    const Block *block1 = arrst_all_const(blocks1, Block);
    const Block *block2 = arrst_all_const(blocks2, Block);
    uint32_t n1 = arrst_size(blocks1, Block);
    uint32_t n2 = arrst_size(blocks2, Block);
    uint32_t i;

    if (n1 != n2)
    {
        String *str = str_printf("Different number of '%s' blocks", blockref);
        i_lwarn(&str, docname, 0, lang1, lang2);
        return;
    }

    for (i = 0; i < n1; ++i, ++block1, ++block2)
        i_compare_block(block1, block2, docname, lang1, lang2);
}

/*---------------------------------------------------------------------------*/

void doc_compare(const Doc *doc1, const Doc *doc2, const char_t *lang1, const char_t *lang2)
{
    cassert_no_null(doc1);
    cassert_no_null(doc2);
    if (str_equ(doc1->name, tc(doc2->name)) == FALSE)
    {
        String *str = str_printf("Different doc name '%s'", tc(doc2->name));
        i_lwarn(&str, tc(doc1->name), 0, lang1, lang2);
        return;
    }

    i_compare_blocks(doc1->blocks, doc2->blocks, tc(doc1->name), "Documentation", lang1, lang2);
    i_compare_blocks(doc1->types, doc2->types, tc(doc1->name), "Types", lang1, lang2);
    i_compare_blocks(doc1->funcs, doc2->funcs, tc(doc1->name), "Functions", lang1, lang2);
}

/*---------------------------------------------------------------------------*/

void doc_destroy(Doc **doc)
{
    cassert_no_null(doc);
    cassert_no_null(*doc);
    str_destroy(&(*doc)->name);
    str_destroy(&(*doc)->packname);
    i_remove_block(&(*doc)->h1);
    i_remove_block(&(*doc)->desc);
    arrst_destroy(&(*doc)->blocks, i_remove_block, Block);
    arrst_destroy(&(*doc)->types, i_remove_block, Block);
    arrst_destroy(&(*doc)->funcs, i_remove_block, Block);
    heap_delete(doc, Doc);
}

/*---------------------------------------------------------------------------*/

const char_t *doc_name(const Doc *doc)
{
    cassert_no_null(doc);
    return tc(doc->name);
}

/*---------------------------------------------------------------------------*/

const char_t *doc_packname(const Doc *doc)
{
    cassert_no_null(doc);
    return tc(doc->packname);
}

/*---------------------------------------------------------------------------*/

const char_t *doc_refid(const Doc *doc, const char_t *refname, btype_t *type)
{
    uint32_t s;
    cassert_no_null(doc);
    cassert_no_null(type);
    s = str_len_c(refname);
    arrst_foreach(block, doc->blocks, Block)
        const char_t *name = NULL;
        switch (block->type)
        {
        case ekIMG:
        case ekIMG2:
            name = i_param(block, 0);
            break;

        case ekCODE:
            name = i_param(block, 1);
            break;

        case ekTABLE:
            name = i_param(block, 0);
            break;

        case ekMATH:
            name = i_param(block, 0);
            break;

        case ekBDRAFT:
        case ekNOSECNUM:
        case ekBLANGREV:
        case ekBCOMPLETE:
        case ekEBOOK_ONLY:
        case ekWEB_ONLY:
        case ekNOTOC:
        case ekEPIG:
        case ekDESC:
        case ekH1:
        case ekH2:
        case ekH3:
        case ekPARAG:
        case ekROW:
        case ekLI:
        case ekLILI:
        case ekBQ:
        case ekCODEFILE:
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
        default:
            break;
        }

        if (name && str_equ_cn(name, refname, s) == TRUE)
        {
            *type = block->type;
            return tc(block->ref);
        }

    arrst_end()
    return NULL;
}

/*---------------------------------------------------------------------------*/

static bool_t i_plaintext_eq(const ArrSt(Tag) *tags, const char_t *text)
{
    uint32_t len = str_len_c(text);
    arrst_foreach_const(tag, tags, Tag)
        if (tag->type == ekPLAINTEXT)
        {
            uint32_t l = str_len(tag->text);
            if (l > len)
                return FALSE;

            if (str_equ_cn(tc(tag->text), text, l) == FALSE)
                return FALSE;

            text += l;
            len -= l;
            if (len == 0)
                return TRUE;
        }
    arrst_end()
    return FALSE;
}

/*---------------------------------------------------------------------------*/

uint32_t doc_section(const Doc *doc, const char_t *secname)
{
    arrst_foreach(block, doc->blocks, Block)
        if (block->type == ekH2 || block->type == ekH3)
        {
            if (i_plaintext_eq(block->tags, secname) == TRUE)
                return block_i;
        }
    arrst_end()
    return UINT32_MAX;
}

/*---------------------------------------------------------------------------*/

const Block *doc_h1(const Doc *doc)
{
    cassert_no_null(doc);
    return &doc->h1;
}

/*---------------------------------------------------------------------------*/

const Block *doc_block(const Doc *doc, const uint32_t index)
{
    cassert_no_null(doc);
    return arrst_get(doc->blocks, index, Block);
}

/*---------------------------------------------------------------------------*/

const Block *doc_type(const Doc *doc, const uint32_t index, const uint32_t child_id)
{
    const Block *block;
    cassert_no_null(doc);
    block = arrst_get(doc->types, index, Block);
    cassert_no_null(block);
    if (child_id != UINT32_MAX)
    {
        cassert(block->type == ekENUM);
        return arrst_get(block->children, child_id, Block);
    }
    return block;
}

/*---------------------------------------------------------------------------*/

const Block *doc_func(const Doc *doc, const uint32_t index)
{
    cassert_no_null(doc);
    return arrst_get(doc->funcs, index, Block);
}

/*---------------------------------------------------------------------------*/

const char_t *doc_block_param(const Block *block, const uint32_t index)
{
    return i_param(block, index);
}

/*---------------------------------------------------------------------------*/

real32_t doc_real_param(const Block *block, const uint32_t index)
{
    return i_real_param(block, index);
}

/*---------------------------------------------------------------------------*/

uint32_t doc_int_param(const Block *block, const uint32_t index)
{
    real32_t p = i_real_param(block, index);
    return p >= 0.f ? (uint32_t)p : UINT32_MAX;
}

/*---------------------------------------------------------------------------*/

bool_t doc_is_real_type(const char_t *type, const Loader *loader)
{
    if (type == NULL)
        return FALSE;

    if (str_equ_nocase(type, "bmath"))
        return TRUE;

    if (str_equ_c(type, "real"))
        return TRUE;

    if (loader_float_type(loader, type) != NULL)
    {
        cassert(loader_double_type(loader, type) != NULL);
        return TRUE;
    }

    return FALSE;
}

/*---------------------------------------------------------------------------*/

const char_t *doc_float_type(const char_t *type, const Loader *loader)
{
    if (type == NULL)
        return "";

    if (str_equ_nocase(type, "bmath"))
        return "real32_t";

    if (str_equ_nocase(type, "real"))
        return "real32_t";

    {
        const char_t *ftype = loader_float_type(loader, type);
        if (ftype)
            return ftype;
    }

    return type;
}

/*---------------------------------------------------------------------------*/

const char_t *doc_double_type(const char_t *type, const Loader *loader)
{
    if (type == NULL)
        return "";

    if (str_equ_nocase(type, "bmath"))
        return "real64_t";

    if (str_equ_nocase(type, "real"))
        return "real64_t";

    {
        const char_t *ftype = loader_double_type(loader, type);
        if (ftype)
            return ftype;
    }

    return type;
}

/*---------------------------------------------------------------------------*/

const char_t *doc_real_type(const char_t *type, const Loader *loader)
{
    if (type == NULL)
        return "";

    if (str_equ_nocase(type, "bmath"))
        return "real";

    unref(loader);
    return type;
}

/*---------------------------------------------------------------------------*/

String *doc_template_constant(const char_t *type, const char_t *name)
{
    return str_printf("%s::k%s", type, name);
}

/*---------------------------------------------------------------------------*/

static String *i_real_constant(const char_t *type, const char_t *name, const char_t *sufix)
{
    String *ctype = str_c(type);
    String *creal;
    str_upper(ctype);
    creal = str_printf("k%s_%s%s", tc(ctype), name, sufix);
    str_destroy(&ctype);
    return creal;
}

/*---------------------------------------------------------------------------*/

String *doc_float_constant(const char_t *type, const char_t *name)
{
    return i_real_constant(type, name, "f");
}

/*---------------------------------------------------------------------------*/

String *doc_double_constant(const char_t *type, const char_t *name)
{
    return i_real_constant(type, name, "d");
}

/*---------------------------------------------------------------------------*/

String *doc_real_constant(const char_t *name)
{
    return str_printf("k%s", name);
}

/*---------------------------------------------------------------------------*/

String *doc_template_func(const char_t *base, const char_t *fname)
{
    if (!str_empty_c(fname))
        return str_printf("%s::%s", base, fname);
    else
        return str_c(base);
}

/*---------------------------------------------------------------------------*/

static String *i_real_func(const char_t *base, const char_t *fname, const char_t *sufix)
{
    String *cbase = str_c(base);
    String *creal;
    str_lower(cbase);
    if (!str_empty_c(fname))
        creal = str_printf("%s_%s%s", tc(cbase), fname, sufix);
    else
        creal = str_printf("%s%s", tc(cbase), sufix);
    str_destroy(&cbase);
    return creal;
}

/*---------------------------------------------------------------------------*/

String *doc_float_func(const char_t *base, const char_t *fname)
{
    return i_real_func(base, fname, "f");
}

/*---------------------------------------------------------------------------*/

String *doc_double_func(const char_t *base, const char_t *fname)
{
    return i_real_func(base, fname, "d");
}

/*---------------------------------------------------------------------------*/

String *doc_real_func(const char_t *base, const char_t *fname)
{
    return i_real_func(base, fname, "");
}

/*---------------------------------------------------------------------------*/

String *doc_template_struct(const char_t *sname)
{
    return str_c(sname);
}

/*---------------------------------------------------------------------------*/

String *doc_float_struct(const char_t *sname)
{
    return str_printf("%sf", sname);
}

/*---------------------------------------------------------------------------*/

String *doc_double_struct(const char_t *sname)
{
    return str_printf("%sd", sname);
}

/*---------------------------------------------------------------------------*/

void doc_func_params(const Block *func, const Block **fret, const Block **fparams, uint32_t *nparams)
{
    uint32_t n;
    cassert_no_null(func);
    cassert_no_null(fret);
    cassert_no_null(fparams);
    cassert_no_null(nparams);
    *fret = NULL;
    *fparams = NULL;
    n = arrst_size(func->children, Block);
    if (n > 0)
    {
        *fparams = arrst_all(func->children, Block);
        if ((*fparams)[0].type == ekFRET)
        {
            *fret = *fparams;
            (*fparams) += 1;
            n -= 1;
        }
    }

    *nparams = n;
}

/*---------------------------------------------------------------------------*/

static void i_latex_chapter(const Doc *doc, const char_t *title, Stream *latex, DocParser *parser)
{
    const Block *epig = NULL;
    cassert_no_null(doc);
    if (!str_equ_nocase(tc(doc->name), "home"))
    {
        stm_writef(latex, "\\chapter{");

        if (str_empty_c(title) == FALSE)
            stm_writef(latex, title);
        else if (doc->h1.type == ekH1)
            latex_h1(latex, &doc->h1, parser);
        else
            stm_writef(latex, "NOTITLE");

        stm_writef(latex, "}\n");

        if (str_empty_c(title) == TRUE)
            stm_printf(latex, "\\label{%s}\n", doc_name(doc));
    }
    else
    {
        stm_writef(latex, "\\cleardoublepage\n");
    }

    stm_writef(latex, "\\thispagestyle{empty}\n");

    epig = i_block(doc->blocks, ekEPIG);
    if (epig != NULL)
        latex_epig(latex, epig, parser);

    stm_writef(latex, "\\minitoc\n");
    stm_writef(latex, "\\vspace{1em}\n");
}

/*---------------------------------------------------------------------------*/

static void i_latex_funcs_chapter(const Doc *doc, const char_t *title, Stream *latex, DocParser *parser)
{
    cassert_no_null(doc);
    cassert(str_equ_nocase(tc(doc->name), "home") == FALSE);
    stm_writef(latex, "\\chapter{");

    if (str_empty_c(title) == FALSE)
        stm_writef(latex, title);
    else if (doc->h1.type == ekH1)
        latex_h1(latex, &doc->h1, parser);
    else
        stm_writef(latex, "NOTITLE");

    stm_writef(latex, "}\n");
}

/*---------------------------------------------------------------------------*/

static bool_t i_section_empty(const ArrSt(Block) *blocks, uint32_t index)
{
    const Block *block = arrst_all_const(blocks, Block);
    uint32_t i, n = arrst_size(blocks, Block);
    cassert(index == UINT32_MAX || block[index].type == ekH2 || block[index].type == ekH3);
    i = (index == UINT32_MAX) ? 0 : index + 1;
    for (; i < n; ++i)
    {
        switch (block[i].type)
        {
        case ekPARAG:
        case ekIMG:
        case ekIMG2:
        case ekTABLE:
        case ekLI:
        case ekLILI:
        case ekBQ:
        case ekCODE:
        case ekCODEFILE:
        case ekMATH:
            return FALSE;

        case ekH2:
        case ekH3:
            if (index == UINT32_MAX)
                continue;
            else
                return TRUE;

        case ekEBOOK_ONLY:
        case ekWEB_ONLY:
            continue;

        case ekEPIG:
            break;

        case ekBDRAFT:
        case ekNOSECNUM:
        case ekBLANGREV:
        case ekBCOMPLETE:
        case ekDESC:
        case ekH1:
        case ekROW:
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
        default:
            cassert_default(block[i].type);
        }
    }

    return TRUE;
}

/*---------------------------------------------------------------------------*/

static bool_t i_chapter_empty(const ArrSt(Block) *blocks)
{
    return i_section_empty(blocks, UINT32_MAX);
}

/*---------------------------------------------------------------------------*/

static void i_update_generator(generator_t *gen, const btype_t type, const DocParser *parser)
{
    cassert_no_null(gen);
    if (type == ekEBOOK_ONLY)
    {
        switch (*gen)
        {
        case ekGEN_EBOOK:
            *gen = ekGEN_ALL;
            break;

        case ekGEN_ALL:
            *gen = ekGEN_EBOOK;
            break;

        case ekGEN_WEB:
        {
            String *str = str_printf("Tag 'web.' its not closed");
            i_warn(&str, parser);
            *gen = ekGEN_EBOOK;
            break;
        }

        default:
            cassert_default(*gen);
        }
    }
    else if (type == ekWEB_ONLY)
    {
        switch (*gen)
        {
        case ekGEN_WEB:
            *gen = ekGEN_ALL;
            break;

        case ekGEN_ALL:
            *gen = ekGEN_WEB;
            break;

        case ekGEN_EBOOK:
        {
            String *str = str_printf("Tag 'ebook.' its not closed");
            i_warn(&str, parser);
            *gen = ekGEN_WEB;
            break;
        }

        default:
            cassert_default(*gen);
        }
    }
}

/*---------------------------------------------------------------------------*/

static void i_latex_docblocks(const Doc *doc, Stream *latex, const Config *config, const char_t *prespath, const bool_t group, DocParser *parser)
{
    generator_t gen = ekGEN_ALL;
    cassert_no_null(doc);
    arrst_foreach(block, doc->blocks, Block)

        i_update_generator(&gen, block->type, parser);

        if (gen != ekGEN_EBOOK && gen != ekGEN_ALL)
            continue;

        parser->line = block->line;

        switch (block->type)
        {
        case ekH2:
            latex_section(latex, block, group ? "subsection" : "section", tc(block->ref), parser);
            if (i_section_empty(doc->blocks, block_i) == TRUE)
            {
                String *str0 = i_plain_str(block->tags);
                String *str1 = str_printf("Section '%s' is empty in eBook", tc(str0));
                i_warn(&str1, parser);
                str_destroy(&str0);
            }
            break;

        case ekH3:
            latex_section(latex, block, "subsection", tc(block->ref), parser);
            if (i_section_empty(doc->blocks, block_i) == TRUE)
            {
                String *str0 = i_plain_str(block->tags);
                String *str1 = str_printf("Subsection '%s' is empty in eBook", tc(str0));
                i_warn(&str1, parser);
                str_destroy(&str0);
            }
            break;

        case ekPARAG:
            latex_parag(latex, block, parser);
            break;

        case ekBQ:
            latex_bq(latex, block, parser);
            break;

        case ekLI:
            latex_li(latex, block, config, parser->docpath, prespath, parser);
            break;

        case ekLILI:
            latex_lili(latex, block, config, parser->docpath, prespath, parser);
            break;

        case ekIMG:
            latex_img(latex, block, parser->docpath, prespath, parser);
            break;

        case ekIMG2:
            latex_img2(latex, block, parser->docpath, prespath, parser);
            break;

        case ekTABLE:
            latex_table(latex, block, parser->docpath, prespath, parser);
            break;

        case ekCODE:
            latex_code(latex, block, parser);
            break;

        case ekCODEFILE:
            latex_code(latex, block, parser);
            break;

        case ekMATH:
            latex_math(latex, block, parser);
            break;

        case ekEPIG:
        case ekEBOOK_ONLY:
        case ekWEB_ONLY:
            break;

        case ekBDRAFT:
        case ekNOSECNUM:
        case ekBLANGREV:
        case ekBCOMPLETE:
        case ekNOTOC:
        case ekDESC:
        case ekH1:
        case ekROW:
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
        default:
            cassert_default(block->type);
        }
    arrst_end()

    latex_liend(latex, parser);
}

/*---------------------------------------------------------------------------*/

static void i_latex_types(const Doc **docs, const uint32_t n, Stream *latex, DocParser *parser)
{
    uint32_t i;
    cassert_no_null(docs);

    for (i = 0; i < n; ++i)
    {
        if (arrst_size(docs[i]->types, Block) > 0)
            break;
    }

    /* No Types */
    if (i == n)
        return;

    stm_writef(latex, "\n");
    stm_printf(latex, "\\section{%s}\n", respack_text(parser->respack, TEXT_03));

    for (i = 0; i < n; ++i)
    {
        parser->doc = docs[i];
        arrst_foreach(type, docs[i]->types, Block)
            if (type->type == ekTYPE)
            {
                parser->line = type->line;
                latex_type(latex, type, parser);
            }
        arrst_end()
    }

    for (i = 0; i < n; ++i)
    {
        parser->doc = docs[i];
        arrst_foreach(type, docs[i]->types, Block)
            if (type->type == ekCONST)
            {
                parser->line = type->line;
                latex_const(latex, type, parser);
            }
        arrst_end()
    }

    for (i = 0; i < n; ++i)
    {
        parser->doc = docs[i];
        arrst_foreach(type, docs[i]->types, Block)
            if (type->type == ekENUM)
            {
                parser->line = type->line;
                latex_enum(latex, type, parser);
            }
        arrst_end()
    }

    for (i = 0; i < n; ++i)
    {
        parser->doc = docs[i];
        arrst_foreach(type, docs[i]->types, Block)
            if (type->type == ekSTRUCT)
            {
                parser->line = type->line;
                latex_struct(latex, type, parser);
            }
        arrst_end()
    }
}

/*---------------------------------------------------------------------------*/

static void i_latex_funcs(const Doc **docs, const uint32_t n, Stream *latex, DocParser *parser)
{
    uint32_t i;
    for (i = 0; i < n; ++i)
    {
        if (arrst_size(docs[i]->funcs, Block) > 0)
            break;
    }

    /* No Functions */
    if (i == n)
        return;

    stm_writef(latex, "\n");
    stm_printf(latex, "\\section{%s}\n", respack_text(parser->respack, TEXT_02));

    for (i = 0; i < n; ++i)
    {
        parser->doc = docs[i];
        arrst_foreach(type, docs[i]->funcs, Block)
            if (type->type == ekFUNC)
            {
                parser->line = type->line;
                latex_func(latex, type, parser);
            }
        arrst_end()
    }
}

/*---------------------------------------------------------------------------*/

static const Doc *i_get_doc(const ArrPt(Doc) *docs, const char_t *name)
{
    arrpt_foreach_const(doc, docs, Doc)
        if (str_equ_c(doc_name(doc), name) == TRUE)
            return doc;
    arrpt_end()
    return NULL;
}

/*---------------------------------------------------------------------------*/

static bool_t i_with_func_or_types(const ArrPt(Doc) *docs)
{
    arrpt_foreach_const(doc, docs, Doc)
        if (arrst_size(doc->types, Block) > 0)
            return TRUE;

        if (arrst_size(doc->funcs, Block) > 0)
            return TRUE;
    arrpt_end()

    return FALSE;
}

/*---------------------------------------------------------------------------*/

void doc_latex(const ArrPt(String) *names, const ArrPt(Doc) *docs, const char_t *title, Stream *latex, const Config *config, const uint32_t lang_id, const Loader *loader, const char_t *docpath, const char_t *prespath, const bool_t write_sections, const bool_t write_funcs)
{
    ArrPt(Doc) *group = arrpt_create(Doc);
    bool_t process = TRUE;

    if (arrpt_size(config->ebook_chapters, String) > 0)
        process = FALSE;

    arrpt_foreach_const(name, names, String)
        const Doc *doc = i_get_doc(docs, tc(name));
        cassert_no_null(doc);
        arrpt_append(group, cast(doc, Doc), Doc);

        if (process == FALSE)
        {
            arrpt_foreach(chapter, config->ebook_chapters, String)
                if (str_scmp(chapter, doc->name) == 0)
                {
                    process = TRUE;
                    break;
                }
            arrpt_end()
        }

    arrpt_end()

    if (process == TRUE)
    {
        DocParser parser;
        const Doc *first = arrpt_get(group, 0, Doc);
        uint32_t n = arrpt_size(group, Doc);

        bmem_zero(&parser, DocParser);
        parser.loader = loader;
        parser.config = config;
        parser.respack = loader_respack(loader);
        parser.doc = first;
        parser.lang_id = lang_id;
        parser.docpath = docpath;
        parser.item_level = 0;

        if (write_sections == TRUE)
        {
            i_latex_chapter(first, title, latex, &parser);

            arrpt_foreach(doc, group, Doc)
                /* Chapter titles always start a section */
                if (i_chapter_empty(doc->blocks) == FALSE || str_empty_c(title) == FALSE)
                {
                    parser.doc = doc;
                    parser.item_level = 0;
                    latex_section(latex, &doc->h1, "section", NULL, &parser);
                    i_latex_docblocks(doc, latex, config, prespath, n > 1 ? TRUE : FALSE, &parser);
                }
            arrpt_end()
        }

        if (write_funcs == TRUE)
        {
            if (i_with_func_or_types(group) == TRUE)
            {
                const Doc **alldocs = (const Doc **)arrpt_all(group, Doc);
                if (write_sections == FALSE)
                    i_latex_funcs_chapter(first, title, latex, &parser);

                i_latex_types(alldocs, n, latex, &parser);
                i_latex_funcs(alldocs, n, latex, &parser);
            }
        }
    }

    arrpt_destroy(&group, NULL, Doc);
}

/*---------------------------------------------------------------------------*/

/*
static void i_toc_type(const Post *post, ArrSt(Block) *types, Stream *html)
{
   arrst_foreach(block, types, Block)
   if (block->type == ekTYPE)
   {
       const char_t *type = i_param(block, 0);
       post_toc_h3(post, tc(block->ref), html);
       if (type)
           stm_writef(html, type);
   }
   arrst_end()
}
 */

/*---------------------------------------------------------------------------*/

/*
static void i_toc_const(const Post *post, ArrSt(Block) *types, Stream *html)
{
   arrst_foreach(block, types, Block)
   if (block->type == ekCONST)
   {
       const char_t *id = i_param(block, 1);
       post_toc_h3(post, tc(block->ref), html);
       if (id)
           stm_writef(html, id);
   }
   arrst_end()
}
 */

/*---------------------------------------------------------------------------*/

/*
static void i_toc_enum(const Post *post, ArrSt(Block) *types, Stream *html)
{
   arrst_foreach(block, types, Block)
   if (block->type == ekENUM)
   {
       const char_t *type = i_param(block, 0);
       post_toc_h3(post, tc(block->ref), html);
       if (type)
           stm_printf(html, "enum %s", type);
   }
   arrst_end()
}
 */

/*---------------------------------------------------------------------------*/

static void i_toc(Stream *html, DocParser *parser, Html5Status *status)
{
    generator_t gen = ekGEN_ALL;
    cassert_no_null(parser);
    cassert_no_null(parser->doc);
    if (i_block(parser->doc->blocks, ekH2) == NULL)
        return;

    post_toc(parser->post, html);
    arrst_foreach(block, parser->doc->blocks, Block)

        i_update_generator(&gen, block->type, parser);

        if (gen != ekGEN_WEB && gen != ekGEN_ALL)
            continue;

        if (block->type == ekH2)
        {
            post_toc_h2(parser->post, tc(block->ref), html);
            web_h2_plain(html, block, parser);
        }
        else if (block->type == ekH3)
        {
            post_toc_h3(parser->post, tc(block->ref), html);
            web_h3_plain(html, block, parser);
        }

    arrst_end()

    /*
    if (arrst_size(parser->doc->types, Block) > 0)
    {
        post_toc_h2(parser->post, "t_and_c", html);
        if (str_equ_c(parser->lang, "es"))
            stm_writef(html, "Tipos y Constantes");
        else
            stm_writef(html, "Types and Constants");
    }

    if (arrst_size(parser->doc->funcs, Block) > 0)
    {
        post_toc_h2(parser->post, "funcs", html);
        if (str_equ_c(parser->lang, "es"))
            stm_writef(html, "Funciones");
        else
            stm_writef(html, "Functions");
    }*/

    post_toc_end(parser->post, html, status);
}

/*---------------------------------------------------------------------------*/

static bool_t i_with_docblocks(const ArrSt(Block) *blocks)
{
    arrst_foreach_const(block, blocks, Block)
        if (block->type != ekEPIG)
            return TRUE;
    arrst_end()
    return FALSE;
}

/*---------------------------------------------------------------------------*/

static void i_web(Stream *html, DocParser *parser)
{
    const Block *epig = NULL;
    generator_t gen = ekGEN_ALL;

    cassert_no_null(parser);
    cassert_no_null(parser->doc);

    web_nav_buttons(html, parser);

    if (parser->doc->state == ekLANGREV)
    {
        String *url = str_printf("%s%s/%s/%s.htm", tc(parser->config->remote_repo), tc(parser->doc->packname), i_lang(parser->config, parser->lang_id), tc(parser->doc->name));
        String *str = str_printf("bq.%s", i_str(parser->config->web_langrev, parser->lang_id));
        String *blktext = str_printf(tc(str), tc(url));
        Block block;
        bool_t ok = i_parse_line(tc(blktext), &block, parser);
        cassert_unref(ok == TRUE, ok);
        cassert(block.type == ekBQ);
        web_bq(html, &block, parser);
        i_remove_block(&block);
        str_destroy(&blktext);
        str_destroy(&str);
        str_destroy(&url);
    }

    epig = i_block(parser->doc->blocks, ekEPIG);
    if (epig != NULL)
        web_epig(html, epig, parser);

    /* Header */
    if (arrst_size(parser->doc->funcs, Block) > 0 ||
        arrst_size(parser->doc->types, Block) > 0)
    {
        post_line(parser->post, html);
        post_h2(parser->post, "header", NULL, FALSE, html);
        stm_writef(html, respack_text(parser->respack, TEXT_27));
        post_h2_end(parser->post, html);
        post_p(parser->post, FALSE, html);
        stm_printf(html, "<code>#include &lt;%s/%s.h&gt;</code>\n", tc(parser->doc->packname), tc(parser->doc->name));
        post_p_end(parser->post, html);
    }

    /* Functions headers */
    if (arrst_size(parser->doc->funcs, Block) > 0)
    {
        post_line(parser->post, html);
        post_h2(parser->post, "funcs", NULL, FALSE, html);
        stm_writef(html, respack_text(parser->respack, TEXT_02));
        post_h2_end(parser->post, html);
        web_funcs_header(html, parser->doc->funcs, parser);
    }

    /* Types headers */
    if (arrst_size(parser->doc->types, Block) > 0)
    {
        post_line(parser->post, html);
        post_h2(parser->post, "t_and_c", NULL, FALSE, html);
        stm_writef(html, respack_text(parser->respack, TEXT_03));
        post_h2_end(parser->post, html);
        web_types_header(html, parser->doc->types, parser);
    }

    if (parser->doc->with_toc == TRUE && i_with_docblocks(parser->doc->blocks) == TRUE)
    {
        post_line(parser->post, html);
        i_toc(html, parser, parser->status);
        nlog_html5(parser->status, "");
    }

    /* Documentation blocks */
    arrst_foreach(block, parser->doc->blocks, Block)

        i_update_generator(&gen, block->type, parser);

        if (gen != ekGEN_WEB && gen != ekGEN_ALL)
            continue;

        parser->line = block->line;

        switch (block->type)
        {
        case ekH2:
            web_h2(html, block, parser);
            break;

        case ekH3:
            web_h3(html, block, parser);
            break;

        case ekPARAG:
            web_parag(html, block, parser);
            break;

        case ekBQ:
            web_bq(html, block, parser);
            break;

        case ekLI:
            web_li(html, block, parser);
            break;

        case ekLILI:
            web_lili(html, block, parser);
            break;

        case ekIMG:
            web_img(html, block, parser);
            break;

        case ekIMG2:
            web_img2(html, block, parser);
            break;

        case ekTABLE:
            web_table(html, block, parser);
            break;

        case ekCODE:
        {
            const char_t *clang = doc_block_param(block, 0);
            web_code(html, block, clang, parser);
            break;
        }

        case ekCODEFILE:
            post_ul_end(parser->post, html);
            web_code(html, block, "cpp", parser);
            break;

        case ekMATH:
            web_math(html, block, parser);
            break;

        case ekEPIG:
        case ekEBOOK_ONLY:
        case ekWEB_ONLY:
            break;

        case ekBDRAFT:
        case ekNOSECNUM:
        case ekBLANGREV:
        case ekBCOMPLETE:
        case ekDESC:
        case ekH1:
        case ekROW:
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
        default:
            cassert_default(block->type);
        }
    arrst_end()

    post_ul_end(parser->post, html);

    web_nav_buttons(html, parser);

    /* Types body */
    if (arrst_size(parser->doc->types, Block) > 0)
        web_types_body(html, parser->doc->types, parser);

    /* Funcs body */
    if (arrst_size(parser->doc->funcs, Block) > 0)
        web_funcs_body(html, parser->doc->funcs, parser);

    if (arrst_size(parser->doc->types, Block) > 0 || arrst_size(parser->doc->funcs, Block) > 0)
        web_nav_buttons(html, parser);

    post_line(parser->post, html);
    post_footer(parser->post, html);
}

/*---------------------------------------------------------------------------*/

static bool_t i_c_keyword(const char_t *keyw)
{
    const char_t *KEYWORDS[] = {"ifndef", "endif", "define", "include", "return", "static", "extern", "void", "const", "typedef", "enum", "struct", "union", "if", "else", "for", "while", "do", "switch", "case", "break", "default", "continue", "sizeof", "template", "typename", "new", "delete", "register"};
    uint32_t i, n = sizeof(KEYWORDS) / sizeof(char_t *);
    for (i = 0; i < n; ++i)
        if (str_equ_c(keyw, KEYWORDS[i]) == TRUE)
            return TRUE;
    return FALSE;
}

/*---------------------------------------------------------------------------*/

static void i_OnWebContent(DocParser *parser, Event *e)
{
    hcont_t type = (hcont_t)event_type(e);
    Stream *html = event_params(e, Stream);
    cassert_no_null(parser);
    cassert_no_null(parser->doc);
    switch (type)
    {
    case ekPAGE_TITLE:
        web_h1(html, &parser->doc->h1, parser);
        break;

    case ekPAGE_META_DESC:
        web_meta_desc(html, &parser->doc->desc, parser);
        break;

    case ekPAGE_TRANSLATE:
    {
        uint32_t i, level = post_level(parser->post);
        const char_t *lang = event_sender(e, char_t);
        for (i = 0; i < level; ++i)
            stm_writef(html, "../");
        stm_writef(html, lang);
        stm_writef(html, "/");
        stm_writef(html, tc(parser->doc->packname));
        stm_writef(html, "/");
        stm_writef(html, tc(parser->doc->name));
        stm_writef(html, ".html");
        break;
    }

    case ekPAGE_CONTENT:
        i_web(html, parser);
        break;

    case ekLNAV_CONTENT:
    {
        LNav *lnav = event_sender(e, LNav);
        loader_web_lnav(parser->loader, parser->section, parser->doc, lnav, html);
        break;
    }

    case ekCODE_IDENTIFIER:
    {
        const char_t *keyw = event_sender(e, char_t);
        Identifier *result = event_result(e, Identifier);
        if (i_c_keyword(keyw) == TRUE)
        {
            result->type = ekID_KEYWORD;
            result->has_link = FALSE;
            result->has_title = FALSE;
        }
        else
        {
            parser->scode_doc = loader_keyword(parser->loader, keyw, &parser->scode_block);
            if (parser->scode_doc != NULL)
            {
                switch (parser->scode_block->type)
                {
                case ekTYPE:
                case ekENUM:
                case ekSTRUCT:
                    result->type = ekID_TYPE;
                    break;

                case ekCONST:
                case ekENUMV:
                    result->type = ekID_CONSTANT;
                    break;

                case ekFUNC:
                    result->type = ekID_FUNCTION;
                    break;

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
                case ekPARAG:
                case ekIMG:
                case ekIMG2:
                case ekTABLE:
                case ekROW:
                case ekLI:
                case ekLILI:
                case ekBQ:
                case ekCODE:
                case ekCODEFILE:
                case ekMATH:
                case ekSMEMBER:
                case ekFPAR:
                case ekFRET:
                case ekFCODE:
                case ekFNOTE:
                case ekWARN:
                case ekNOTAG:
                case ekNOTOC:
                default:
                    cassert_default(parser->scode_block->type);
                }

                result->has_link = TRUE;
                result->has_title = TRUE;
            }
            else
            {
                result->type = ekID_OTHER;
                result->has_link = FALSE;
                result->has_title = FALSE;
            }
        }
        break;
    }

    case ekCODE_LINK:
        if (str_equ_c(doc_packname(parser->scode_doc), doc_packname(parser->doc)) == TRUE)
            stm_printf(html, "%s.html#", doc_name(parser->scode_doc));
        else
            stm_printf(html, "../%s/%s.html#", doc_packname(parser->scode_doc), doc_name(parser->scode_doc));

        stm_writef(html, tc(parser->scode_block->ref));
        break;

    case ekCODE_TITLE:
        web_tags(html, parser->scode_block, parser);
        break;

    case ekCSS_CONTENT:
    case ekJS_CONTENT:
    default:
        cassert_default(type);
    }
}

/*---------------------------------------------------------------------------*/

void doc_web(const Doc *doc, WSite *site, const Config *config, const uint32_t lang_id, const Loader *loader, const char_t *docpath, const WebSec *section, const uint32_t menu_id)
{
    DocParser parser;
    cassert_no_null(doc);
    bmem_zero(&parser, DocParser);
    parser.config = config;
    parser.loader = loader;
    parser.doc = doc;
    parser.lang_id = lang_id;
    parser.section = section;
    parser.respack = loader_respack(loader);
    parser.post = wsite_get_post(site);
    parser.scode = wsite_get_scode(site);
    parser.listener = listener(&parser, i_OnWebContent, DocParser);
    parser.docpath = docpath;
    parser.status = html5_status_create();
    wsite_begin_page(site, tc(doc->name), menu_id, parser.listener, parser.status);
    nlog_html5(parser.status, "");
    listener_destroy(&parser.listener);
    html5_status_destroy(&parser.status);
}

/*---------------------------------------------------------------------------*/

bool_t doc_secnums(const Doc *doc)
{
    cassert_no_null(doc);
    return doc->with_secnum;
}
