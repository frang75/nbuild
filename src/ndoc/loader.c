/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: loader.c
 *
 */

/* Document loader */

#include "loader.h"
#include "doc.h"
#include "latex.h"
#include "nlog.h"
#include "web.h"
#include "res_ndoc.h"
#include <html5/html5.h>
#include <html5/header.h>
#include <html5/nav.h>
#include <html5/rcol.h>
#include <html5/wsite.h>
#include <nlib/ssh.h>
#include <draw2d/color.h>
#include <encode/json.h>
#include <core/arrpt.h>
#include <core/arrst.h>
#include <core/dbind.h>
#include <core/event.h>
#include <core/heap.h>
#include <core/hfile.h>
#include <core/respack.h>
#include <core/stream.h>
#include <core/strings.h>
#include <osbs/bfile.h>
#include <osbs/btime.h>
#include <sewer/bmem.h>
#include <sewer/bstd.h>
#include <sewer/cassert.h>
#include <sewer/ptr.h>
#include <sewer/sewer.h>

typedef struct pkgroup_t PkGroup;
typedef struct pkgconf_t PkgConf;
typedef struct _pack_t Pack;

struct pkgroup_t
{
    ArrPt(String) *title;
    ArrPt(String) *docs;
};

struct pkgconf_t
{
    ArrSt(PkGroup) *web;
    ArrSt(PkGroup) *ebook;
};

struct _pack_t
{
    String *name;
    bool_t is_landing;
    PkgConf *conf;
    ArrPt(Doc) *docs;
};

struct _loader_t
{
    const Config *config;
    uint32_t lang_id;
    ResPack *respack;
    String *docpath;
    ArrSt(Pack) *packs;
    ArrSt(DIndex) *types;
    ArrSt(DIndex) *functions;
    ArrSt(DIndex) *sections;
};

DeclSt(PkGroup);
DeclSt(Pack);

/*---------------------------------------------------------------------------*/

static void i_remove_pack(Pack *pack)
{
    str_destroy(&pack->name);
    dbind_destopt(&pack->conf, PkgConf);

    if (pack->docs != NULL)
        arrpt_destroy(&pack->docs, doc_destroy, Doc);
}

/*---------------------------------------------------------------------------*/

static void i_remove_dindex(DIndex *dindex)
{
    str_destroy(&dindex->name);
}

/*---------------------------------------------------------------------------*/

void loader_dbind(void)
{
    dbind(PkGroup, ArrPt(String) *, title);
    dbind(PkGroup, ArrPt(String) *, docs);
    dbind(PkgConf, ArrSt(PkGroup) *, web);
    dbind(PkgConf, ArrSt(PkGroup) *, ebook);
}

/*---------------------------------------------------------------------------*/

static int i_dindex(const DIndex *idx1, const DIndex *idx2)
{
    return str_cmp(idx1->name, tc(idx2->name));
}

/*---------------------------------------------------------------------------*/

static int i_dindex_key(const DIndex *idx1, const char_t *key)
{
    return str_cmp(idx1->name, key);
}

/*---------------------------------------------------------------------------*/

const Doc *loader_doc(const Loader *loader, const char_t *docname)
{
    cassert_no_null(loader);
    arrst_foreach(pack, loader->packs, Pack)
        if (pack->is_landing == FALSE)
        {
            arrpt_foreach(doc, pack->docs, Doc)
                if (str_equ_c(docname, doc_name(doc)) == TRUE)
                    return doc;
            arrpt_end()
        }
    arrst_end()
    return NULL;
}

/*---------------------------------------------------------------------------*/

const Doc *loader_section(const Loader *loader, const char_t *name, const Doc *curdoc, const Block **block)
{
    /* TODO: Improve: Section aliases */
    /* const DIndex *dindex = arrst_bsearch(loader->sections, i_dindex_key, name, NULL, DIndex, char_t);*/

    const DIndex *dindex = NULL;
    arrst_foreach(section, loader->sections, DIndex)
        if (str_equ(section->name, name) == TRUE)
        {
            if (dindex == NULL || section->doc == curdoc)
                dindex = section;
        }
    arrst_end()

    if (dindex != NULL && (dindex->type == ekH2 || dindex->type == ekH3))
    {
        *block = doc_block(dindex->doc, dindex->block_id);
        return dindex->doc;
    }
    return NULL;
}

/*---------------------------------------------------------------------------*/

const Doc *loader_type(const Loader *loader, const char_t *name, const Block **block)
{
    const DIndex *dindex = arrst_bsearch(loader->types, i_dindex_key, name, NULL, DIndex, char_t);
    if (dindex != NULL && (dindex->type == ekTYPE || dindex->type == ekENUM || dindex->type == ekENUMV || dindex->type == ekSTRUCT || dindex->type == ekCONST))
    {
        *block = doc_type(dindex->doc, dindex->block_id, dindex->child_id);
        return dindex->doc;
    }
    return NULL;
}

/*---------------------------------------------------------------------------*/

const Doc *loader_func(const Loader *loader, const char_t *name, const Block **block)
{
    const DIndex *dindex = arrst_bsearch(loader->functions, i_dindex_key, name, NULL, DIndex, char_t);
    if (dindex != NULL && (dindex->type == ekFUNC))
    {
        *block = doc_func(dindex->doc, dindex->block_id);
        return dindex->doc;
    }
    return NULL;
}

/*---------------------------------------------------------------------------*/

const Doc *loader_keyword(const Loader *loader, const char_t *name, const Block **block)
{
    const DIndex *dindex = arrst_bsearch(loader->types, i_dindex_key, name, NULL, DIndex, char_t);
    if (dindex != NULL)
    {
        if (dindex->type == ekTYPE || dindex->type == ekENUM || dindex->type == ekENUMV || dindex->type == ekSTRUCT || dindex->type == ekCONST)
        {
            *block = doc_type(dindex->doc, dindex->block_id, dindex->child_id);
            return dindex->doc;
        }
    }

    dindex = arrst_bsearch(loader->functions, i_dindex_key, name, NULL, DIndex, char_t);
    if (dindex != NULL)
    {
        if (dindex->type == ekFUNC)
        {
            *block = doc_func(dindex->doc, dindex->block_id);
            return dindex->doc;
        }
    }

    return NULL;
}

/*---------------------------------------------------------------------------*/

static Pack *i_get_pack(ArrSt(Pack) *packs, const char_t *packname)
{
    arrst_foreach(pack, packs, Pack)
        if (str_equ(pack->name, packname) == TRUE)
            return pack;
    arrst_end()
    return NULL;
}

/*---------------------------------------------------------------------------*/

static Pack *i_prev_pack(const Loader *loader, const Pack *pack)
{
    arrst_foreach(section, loader->config->web_sections, WebSec)
        arrpt_foreach(ipack, section->packs, String)
            if (str_equ(ipack, tc(pack->name)) == TRUE)
            {
                if (ipack_i > 0)
                {
                    const String *pname = arrpt_get(section->packs, ipack_i - 1, String);
                    return i_get_pack(loader->packs, tc(pname));
                }
                else
                {
                    return NULL;
                }
            }
        arrpt_end()
    arrst_end()
    return NULL;
}

/*---------------------------------------------------------------------------*/

static Pack *i_next_pack(const Loader *loader, const Pack *pack)
{
    arrst_foreach(section, loader->config->web_sections, WebSec)
        arrpt_foreach(ipack, section->packs, String)
            if (str_equ(ipack, tc(pack->name)) == TRUE)
            {
                if (ipack_i < ipack_total - 1)
                {
                    const String *pname = arrpt_get(section->packs, ipack_i + 1, String);
                    return i_get_pack(loader->packs, tc(pname));
                }
                else
                {
                    return NULL;
                }
            }
        arrpt_end()
    arrst_end()
    return NULL;
}

/*---------------------------------------------------------------------------*/

const Doc *loader_prev_doc(const Loader *loader, const Doc *doc)
{
    cassert_no_null(loader);
    arrst_foreach(lpack, loader->packs, Pack)
        if (lpack->is_landing == FALSE)
        {
            arrpt_foreach(pdoc, lpack->docs, Doc)
                if (pdoc == doc)
                {
                    if (pdoc_i > 0)
                    {
                        return arrpt_get(lpack->docs, pdoc_i - 1, Doc);
                    }
                    else
                    {
                        Pack *pack = i_prev_pack(loader, lpack);
                        if (pack != NULL)
                        {
                            if (arrpt_size(pack->docs, Doc) > 0)
                            {
                                return arrpt_last(pack->docs, Doc);
                            }
                        }
                    }
                }
            arrpt_end()
        }
    arrst_end()
    return NULL;
}

/*---------------------------------------------------------------------------*/

const Doc *loader_next_doc(const Loader *loader, const Doc *doc)
{
    cassert_no_null(loader);
    arrst_foreach(lpack, loader->packs, Pack)
        if (lpack->is_landing == FALSE)
        {
            arrpt_foreach(pdoc, lpack->docs, Doc)
                if (pdoc == doc)
                {
                    uint32_t n = arrpt_size(lpack->docs, Doc);
                    if (pdoc_i < n - 1)
                    {
                        return arrpt_get(lpack->docs, pdoc_i + 1, Doc);
                    }
                    else
                    {
                        Pack *pack = i_next_pack(loader, lpack);
                        if (pack != NULL)
                        {
                            if (arrpt_size(pack->docs, Doc) > 0)
                            {
                                return arrpt_get(pack->docs, 0, Doc);
                            }
                        }
                    }
                }
            arrpt_end()
        }
    arrst_end()
    return NULL;
}

/*---------------------------------------------------------------------------*/

const char_t *loader_float_type(const Loader *loader, const char_t *name)
{
    char_t tname[128];
    const DIndex *dindex = NULL;
    bstd_sprintf(tname, sizeof(tname), "%sf", name);
    /* Still the array is not sorted.... Change by set */
    dindex = arrst_search(loader->types, i_dindex_key, tname, NULL, DIndex, char_t);
    if (dindex && (dindex->type == ekTYPE || dindex->type == ekSTRUCT || dindex->type == ekCONST))
        return tc(dindex->name);
    return NULL;
}

/*---------------------------------------------------------------------------*/

const char_t *loader_double_type(const Loader *loader, const char_t *name)
{
    char_t tname[128];
    const DIndex *dindex = NULL;
    bstd_sprintf(tname, sizeof(tname), "%sd", name);
    /* Still the array is not sorted.... Change by set */
    dindex = arrst_search(loader->types, i_dindex_key, tname, NULL, DIndex, char_t);
    if (dindex && (dindex->type == ekTYPE || dindex->type == ekSTRUCT || dindex->type == ekCONST))
        return tc(dindex->name);
    return NULL;
}

/*---------------------------------------------------------------------------*/

static bool_t is_landing_pack(const PkgConf *conf)
{
    cassert_no_null(conf);
    if (arrst_size(conf->web, PkGroup) == 1)
    {
        const PkGroup *group = arrst_get(conf->web, 0, PkGroup);
        if (arrpt_size(group->docs, String) == 1)
        {
            const String *str = arrpt_get(group->docs, 0, String);
            if (str_equ(str, "__LANDING__") == TRUE)
                return TRUE;
        }
    }

    return FALSE;
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

static bool_t i_pack(
    Pack *pack,
    const char_t *docpath,
    const char_t *src_repo_url,
    const char_t *src_repo_user,
    const char_t *src_repo_pass,
    const uint32_t src_repo_vers,
    const char_t *packname,
    const Config *config,
    const uint32_t lang_id,
    Loader *loader)
{
    String *cfile = str_cpath("%s/%s/config.json", docpath, packname);
    Stream *sconf = stm_from_file(tc(cfile), NULL);
    ArrPt(String) *pack_images = arrpt_create(String);
    bool_t ok = FALSE;
    bmem_zero(pack, Pack);

    /* Get a list of all images in this pack */
    {
        String *image_path = str_cpath("%s/%s/img", docpath, packname);
        ArrSt(DirEntry) *entries = hfile_dir_list(tc(image_path), FALSE, NULL);
        arrst_foreach_const(entry, entries, DirEntry)
            String *image_file = str_copy(entry->name);
            arrpt_append(pack_images, image_file, String);
        arrst_end()
        arrst_destroy(&entries, hfile_dir_entry_remove, DirEntry);
        str_destroy(&image_path);
    }

    if (sconf != NULL)
    {
        pack->conf = json_read(sconf, NULL, PkgConf);
        if (pack->conf != NULL)
        {
            pack->name = str_c(packname);
            if (is_landing_pack(pack->conf) == TRUE)
            {
                pack->is_landing = TRUE;
            }
            else
            {
                String *imgpath = str_cpath("%s/%s/img", docpath, packname);
                pack->is_landing = FALSE;
                pack->docs = arrpt_create(Doc);

                arrst_foreach(group, pack->conf->web, PkGroup)
                    arrpt_foreach(docname, group->docs, String)
                        if (i_get_doc(pack->docs, tc(docname)) == NULL)
                        {
                            Doc *doc = doc_create(docpath, src_repo_url, src_repo_user, src_repo_pass, src_repo_vers, packname, tc(docname), tc(imgpath), config, lang_id, pack_images, loader);
                            arrpt_append(pack->docs, doc, Doc);
                        }
                    arrpt_end()
                arrst_end()

                arrst_foreach(group, pack->conf->ebook, PkGroup)
                    arrpt_foreach(docname, group->docs, String)
                        if (i_get_doc(pack->docs, tc(docname)) == NULL)
                        {
                            Doc *doc = doc_create(docpath, src_repo_url, src_repo_user, src_repo_pass, src_repo_vers, packname, tc(docname), tc(imgpath), config, lang_id, pack_images, loader);
                            arrpt_append(pack->docs, doc, Doc);
                        }
                    arrpt_end()
                arrst_end()

                str_destroy(&imgpath);
            }

            ok = TRUE;
        }
        else
        {
            String *msg = str_printf("Processing '%s'.", tc(cfile));
            nlog_error(&msg);
        }

        stm_close(&sconf);
    }
    else
    {
        String *msg = str_printf("Loading '%s'.", tc(cfile));
        nlog_error(&msg);
    }

    arrpt_foreach_const(image, pack_images, String)
        String *msg = str_cpath("Non used image %s/%s", packname, tc(image));
        nlog_warn(&msg);
    arrpt_end()

    arrpt_destroy(&pack_images, str_destroy, String);
    str_destroy(&cfile);
    return ok;
}

/*---------------------------------------------------------------------------*/

DIndex *loader_section_index(Loader *loader)
{
    cassert_no_null(loader);
    return arrst_new(loader->sections, DIndex);
}

/*---------------------------------------------------------------------------*/

DIndex *loader_type_index(Loader *loader)
{
    cassert_no_null(loader);
    return arrst_new(loader->types, DIndex);
}

/*---------------------------------------------------------------------------*/

DIndex *loader_function_index(Loader *loader)
{
    cassert_no_null(loader);
    return arrst_new(loader->functions, DIndex);
}

/*---------------------------------------------------------------------------*/

ResPack *loader_respack(const Loader *loader)
{
    return loader->respack;
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
    if (lang_id < arrpt_size(strs, String))
    {
        const String *str = arrpt_get_const(strs, lang_id, String);
        return tc(str);
    }

    return "";
}

/*---------------------------------------------------------------------------*/

Loader *loader_run(
    const char_t *docpath,
    const char_t *src_repo_url,
    const char_t *src_repo_user,
    const char_t *src_repo_pass,
    const uint32_t src_repo_vers,
    const Config *config,
    const uint32_t lang_id)
{
    Loader *loader = heap_new0(Loader);
    loader->config = config;
    loader->lang_id = lang_id;
    loader->respack = res_ndoc_respack(i_lang(config, lang_id));
    loader->docpath = str_c(docpath);
    loader->packs = arrst_create(Pack);
    loader->types = arrst_create(DIndex);
    loader->functions = arrst_create(DIndex);
    loader->sections = arrst_create(DIndex);

    arrst_foreach(section, config->web_sections, WebSec)
        arrpt_foreach(pack, section->packs, String)
            Pack packobj;
            if (i_pack(&packobj, docpath, src_repo_url, src_repo_user, src_repo_pass, src_repo_vers, tc(pack), config, lang_id, loader) == TRUE)
                arrst_append(loader->packs, packobj, Pack);
        arrpt_end()
    arrst_end()

    arrst_sort(loader->types, i_dindex, DIndex);
    arrst_sort(loader->functions, i_dindex, DIndex);
    arrst_sort(loader->sections, i_dindex, DIndex);
    return loader;
}

/*---------------------------------------------------------------------------*/

static void i_compare_pack(const Pack *pack1, const Pack *pack2, const char_t *lang1, const char_t *lang2)
{
    Doc **docs1 = arrpt_all(pack1->docs, Doc);
    Doc **docs2 = arrpt_all(pack2->docs, Doc);
    uint32_t i, n = arrpt_size(pack1->docs, Doc);
    cassert(n == arrpt_size(pack2->docs, Doc));

    arrst_foreach(conf, pack1->conf->web, PkGroup)
        arrpt_foreach(title, conf->title, String)
            if (str_empty(title) == TRUE)
            {
                String *msg = str_printf("[%s]-Empty Web Title.", tc(pack1->name));
                nlog_warn(&msg);
            }
        arrpt_end()
    arrst_end()

    /*
    arrst_foreach(conf, pack1->conf->ebook, PkGroup)
    arrpt_foreach(title, conf->title, String)
        if (str_empty(title) == TRUE)
            log_printf("[%s]-Empty eBook Title.", tc(pack1->name));
    arrpt_end()
    arrst_end()
 */

    for (i = 0; i < n; ++i)
    {
        cassert(str_equ_c(doc_name(docs1[i]), doc_name(docs2[i])) == TRUE);
        doc_compare(docs1[i], docs2[i], lang1, lang2);
    }
}

/*---------------------------------------------------------------------------*/

void loader_compare(const Loader *loader1, const Loader *loader2)
{
    uint32_t i, n = arrst_size(loader1->packs, Pack);
    const Pack *pack1 = arrst_all(loader1->packs, Pack);
    const Pack *pack2 = arrst_all(loader2->packs, Pack);
    const char_t *lang1 = i_lang(loader1->config, loader1->lang_id);
    const char_t *lang2 = i_lang(loader2->config, loader2->lang_id);
    cassert(n = arrst_size(loader2->packs, Pack));
    for (i = 0; i < n; ++i, pack1++, pack2++)
    {
        cassert(str_equ(pack1->name, tc(pack2->name)) == TRUE);
        cassert(pack1->is_landing == pack2->is_landing);
        if (pack1->is_landing == FALSE)
            i_compare_pack(pack1, pack2, lang1, lang2);
    }
}

/*---------------------------------------------------------------------------*/

void loader_destroy(Loader **loader)
{
    cassert_no_null(loader);
    cassert_no_null(*loader);
    str_destroy(&(*loader)->docpath);
    respack_destroy(&(*loader)->respack);
    arrst_destroy(&(*loader)->packs, i_remove_pack, Pack);
    arrst_destroy(&(*loader)->types, i_remove_dindex, DIndex);
    arrst_destroy(&(*loader)->functions, i_remove_dindex, DIndex);
    arrst_destroy(&(*loader)->sections, i_remove_dindex, DIndex);
    heap_delete(loader, Loader);
}

/*---------------------------------------------------------------------------*/

void loader_latex(const Loader *loader, const char_t *texpath, const char_t *project_vers)
{
    bool_t ok = TRUE;
    String *output_dir = NULL;
    String *outres_dir = NULL;
    Stream *latex = NULL;
    cassert_no_null(loader);
    cassert_no_null(loader->config);

    /* LaTeX output directory */
    if (ok == TRUE)
    {
        output_dir = str_cpath("%s/%s", texpath, i_lang(loader->config, loader->lang_id));
        outres_dir = str_cpath("%s/res", tc(output_dir));
        ok = hfile_dir_create(tc(outres_dir), NULL);
        if (ok == FALSE)
        {
            String *msg = str_printf("Creating '%s' dir.", tc(output_dir));
            nlog_error(&msg);
        }
    }

    /* .tex file */
    if (ok == TRUE)
    {
        String *str = str_cpath("%s/ndoc_%s.tex", tc(output_dir), i_lang(loader->config, loader->lang_id));
        latex = stm_to_file(tc(str), NULL);
        if (latex == NULL)
        {
            String *msg = str_printf("Creating '%s'.", tc(str));
            nlog_error(&msg);
            ok = FALSE;
        }
        str_destroy(&str);
    }

    /* Functions and types tokens */
    latex_tokens(tc(output_dir), loader->config, loader->lang_id, loader);

    /* Preamble */
    if (ok == TRUE)
        ok = latex_preamble(latex, project_vers, loader->config, loader->lang_id, loader->respack, tc(loader->docpath), tc(outres_dir));

    if (ok == TRUE)
    {
        stm_writef(latex, "\n\n");
        stm_writef(latex, "\\makeindex\n");
        stm_writef(latex, "\\begin{document}\n");
        /* OJO!!!!! BABEL english */
        stm_printf(latex, "\\begin{otherlanguage}{%s}\n\n", respack_text(loader->respack, TEXT_17));
        stm_writef(latex, "\\pagenumbering{gobble}\n");
    }

    if (ok == TRUE)
    {
        ok = latex_cover(latex, loader->config, loader->lang_id, tc(loader->docpath), tc(outres_dir));
        stm_writef(latex, "\\cleardoublepage\n");
        stm_writef(latex, "\\maketitle\n");
        stm_writef(latex, "\\cleardoublepage\n");
        stm_writef(latex, "\\dominitoc\n");
        stm_writef(latex, "\\tableofcontents\n");
        stm_writef(latex, "\\pagenumbering{arabic}\n");
        stm_writef(latex, "\\setcounter{page}{1}\n");
        stm_writef(latex, "\n% Book contents starts here!\n");
    }

    if (ok == TRUE)
    {
        arrst_foreach(part, loader->config->ebook_parts, EBPart)

            if (part->build == FALSE)
                continue;

            if (arrpt_size(part->packs, String) > 0 && !str_equ_nocase(i_str(part->title, loader->lang_id), "home"))
                latex_part(latex, i_str(part->title, loader->lang_id));

            arrpt_foreach(packname, part->packs, String)
                Pack *pack = i_get_pack(loader->packs, tc(packname));
                String *prespath = str_cpath("%s/%s", tc(outres_dir), tc(pack->name));
                String *pdocpath = str_cpath("%s/%s", tc(loader->docpath), tc(pack->name));
                cassert_no_null(pack->conf);
                if (hfile_dir_create(tc(prespath), NULL) == TRUE)
                {
                    bool_t write_funcs = loader->config->ebook_funcs_mode == 1;
                    arrst_foreach(group, pack->conf->ebook, PkGroup)
                        doc_latex(group->docs, pack->docs, i_str(group->title, loader->lang_id), latex, loader->config, loader->lang_id, loader, tc(pdocpath), tc(prespath), TRUE, write_funcs);
                        stm_writef(latex, "\n\n");
                    arrpt_end()
                }
                else
                {
                    String *msg = str_printf("Creating '%s' dir.", tc(prespath));
                    nlog_error(&msg);
                }

                str_destroy(&pdocpath);
                str_destroy(&prespath);
            arrpt_end()
        arrst_end()
    }

    /* Generate the complete library reference at the end of the book */
    if (ok == TRUE && loader->config->ebook_funcs_mode == 2)
    {
        latex_part(latex, respack_text(loader->respack, TEXT_26));

        arrst_foreach(part, loader->config->ebook_parts, EBPart)

            if (part->build == FALSE)
                continue;

            arrpt_foreach(packname, part->packs, String)
                Pack *pack = i_get_pack(loader->packs, tc(packname));
                String *prespath = str_cpath("%s/%s", tc(outres_dir), tc(pack->name));
                String *pdocpath = str_cpath("%s/%s", tc(loader->docpath), tc(pack->name));
                cassert_no_null(pack->conf);
                if (hfile_dir_create(tc(prespath), NULL) == TRUE)
                {
                    arrst_foreach(group, pack->conf->ebook, PkGroup)
                        doc_latex(group->docs, pack->docs, i_str(group->title, loader->lang_id), latex, loader->config, loader->lang_id, loader, tc(pdocpath), tc(prespath), FALSE, TRUE);
                        stm_writef(latex, "\n\n");
                    arrst_end()
                }
                else
                {
                    String *msg = str_printf("Creating '%s' dir.", tc(prespath));
                    nlog_error(&msg);
                }

                str_destroy(&pdocpath);
                str_destroy(&prespath);
            arrpt_end()
        arrst_end()
    }

    if (ok == TRUE)
    {
        stm_writef(latex, "\n");
        stm_writef(latex, "\\cleardoublepage\n");
        stm_writef(latex, "\\printindex\n");
        stm_writef(latex, "\\end{otherlanguage}\n");
        stm_writef(latex, "\\end{document}\n");
    }

    /* Compile pdftex */
    if (ok == TRUE)
    {
    }

    ptr_destopt(stm_close, &latex, Stream);
    str_destopt(&output_dir);
    str_destopt(&outres_dir);
}

/*---------------------------------------------------------------------------*/

static void i_OnWSite(Loader *loader, Event *e)
{
    hcont_t type = (hcont_t)event_type(e);
    cassert_no_null(loader);
    if (type == ekCSS_CONTENT)
    {
        CSS *css = event_params(e, CSS);
        web_css(css, loader->config);
    }
    else
    {
        cassert(type == ekJS_CONTENT);
    }
}

/*---------------------------------------------------------------------------*/

static String *i_url(const Loader *loader, const char_t *url)
{
    String *left, *right;
    if (str_split(url, "''", &left, &right) == TRUE)
    {
        String *rurl;
        if (str_equ(left, "lp") == TRUE)
        {
            const Doc *doc = loader_doc(loader, tc(right));
            if (doc != NULL)
                rurl = str_printf("../%s/%s.html", doc_packname(doc), tc(right));
            else
                rurl = str_printf("Unknown/%s", tc(right));
        }
        else
        {
            Pack *pack = i_get_pack(loader->packs, tc(left));
            if (pack != NULL && pack->is_landing == TRUE)
            {
                rurl = str_printf("../%s/web/%s.html", tc(pack->name), tc(right));
            }
            else
            {
                String *msg = str_printf("Unknown url format '%s'.", url);
                nlog_warn(&msg);
                rurl = str_c("");
            }
        }

        str_destroy(&left);
        str_destroy(&right);
        return rurl;
    }

    str_destroy(&right);
    return left;
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

void loader_web(const Loader *loader, WSite *site, const char_t *project_vers)
{
    Date date;
    Html5Status *status = NULL;
    cassert_no_null(loader);
    cassert_no_null(loader->config);

    btime_date(&date);
    status = html5_status_create();

    /* Configure site comments */
    {
        String *comment = str_printf("%s %s %s", tc(loader->config->project_name), project_vers, i_str(loader->config->project_brief, loader->lang_id));
        wsite_clear_comments(site);
        wsite_comment(site, &comment);
        wsite_comment_c(site, respack_text(loader->respack, TEXT_01));
        comment = str_printf("© %d-%d %s (%s)", loader->config->project_start_year, date.year, tc(loader->config->project_author), tc(loader->config->project_email));
        wsite_comment(site, &comment);
        comment = str_printf("%s %s", i_str(loader->config->project_license, loader->lang_id), i_str(loader->config->project_license_url, loader->lang_id));
        wsite_comment(site, &comment);
    }

    /* Footer */
    {
        Stream *stm = stm_memory(1024);
        String *str = NULL;
        stm_printf(stm, "%d-%d %s", loader->config->project_start_year, date.year, tc(loader->config->project_name));
        stm_printf(stm, "-%s", project_vers);
        stm_printf(stm, " (%d %s)<br/>\n", date.mday, i_month(loader->respack, date.month));
        stm_printf(stm, "%s <br/>\n", respack_text(loader->respack, TEXT_01));
        stm_printf(stm, "© <a href='mailto:%s' rel='author'>%s</a> <a rel='license' href='%s'>%s</a>.<br/>\n", tc(loader->config->project_email), tc(loader->config->project_author), i_str(loader->config->project_license_url, loader->lang_id), i_str(loader->config->project_license, loader->lang_id));
        stm_printf(stm, "<a href='%s'>%s</a>.<br/>\n", i_str(loader->config->project_legal_url, loader->lang_id), respack_text(loader->respack, TEXT_16));
        str = stm_str(stm);
        wsite_footer(site, &str);
        stm_close(&stm);
    }

    /* Update header */
    {
        Header *header = wsite_gheader(site);
        header_text(header, i_str(loader->config->project_brief, loader->lang_id));
    }

    /* Update navigation */
    {
        Nav *nav = wsite_get_nav(site);
        nav_clear_items(nav);
        arrst_foreach(section, loader->config->web_sections, WebSec)
            String *url = i_url(loader, i_str(section->url, loader->lang_id));
            if (str_equ_c(i_str(section->menu, loader->lang_id), "HOME"))
            {
                Header *header = wsite_gheader(site);
                header_home(header, tc(url));
                nav_set_home(nav, tc(url));
            }
            else if (str_empty_c(i_str(section->menu, loader->lang_id)) == FALSE)
            {
                nav_add_item(nav, i_str(section->menu, loader->lang_id), tc(url), i_str(section->hover, loader->lang_id));
            }
            str_destroy(&url);
        arrst_end()
    }

    {
        Listener *listener = listener(loader, i_OnWSite, Loader);
        wsite_begin_lang(site, i_lang(loader->config, loader->lang_id), i_str(loader->config->project_brief, loader->lang_id), listener, status);
        nlog_html5(status, "");
        listener_destroy(&listener);
    }

    /*{
        RCol *rcol = wsite_get_rcol(site);
        Lang *lang = arrst_get(loader->config->langs, loader->lang_id, Lang);
        rcol_remove_banners(rcol);
        if (str_equ(lang->lang, "es") == TRUE)
            rcol_add_banner(rcol, "cover_web_es.png", "Descargar el eBook", "https://nappgui.com/ebook/nappgui_es.pdf");
        else
            rcol_add_banner(rcol, "cover_web_en.png", "Download the eBook", "https://nappgui.com/ebook/nappgui_en.pdf");
    }*/

    /* Create packs & documents */
    arrst_foreach(section, loader->config->web_sections, WebSec)
        if (section->build == TRUE)
        {
            arrpt_foreach(packname, section->packs, String)
                Pack *pack = i_get_pack(loader->packs, tc(packname));
                if (pack != NULL)
                {
                    String *docpath = str_cpath("%s/%s", tc(loader->docpath), tc(packname));
                    wsite_begin_pack(site, tc(pack->name), status);
                    nlog_html5(status, "");

                    if (pack->is_landing == TRUE)
                    {
                        const char_t *except[] = {"gen"};
                        wsite_copy_landing(site, tc(docpath), except, 1);
                    }
                    else
                    {
                        arrpt_foreach(doc, pack->docs, Doc)
                            doc_web(doc, site, loader->config, loader->lang_id, loader, tc(docpath), section, section_i - 1);
                        arrpt_end()
                    }

                    wsite_end_pack(site);
                    str_destroy(&docpath);
                }
            arrpt_end()
        }
    arrst_end()

    wsite_end_lang(site);
    html5_status_destroy(&status);
}

/*---------------------------------------------------------------------------*/

void loader_types_to_latex(const Loader *loader, Stream *latex)
{
    cassert_no_null(loader);
    stm_writef(latex, "        ");
    stm_writef(latex, "type,\n");
    stm_writef(latex, "        ");
    stm_writef(latex, "dtype");

    arrst_foreach(index, loader->types, DIndex)
        if (index->type == ekTYPE || index->type == ekENUM || index->type == ekSTRUCT)
        {
            stm_writef(latex, ",\n");
            stm_writef(latex, "        ");
            str_writef(latex, index->name);
        }
    arrst_end()
}

/*---------------------------------------------------------------------------*/

void loader_constants_to_latex(const Loader *loader, Stream *latex)
{
    stm_writef(latex, "        ");
    stm_writef(latex, "NULL");
    cassert_no_null(loader);
    arrst_foreach(index, loader->types, DIndex)
        if (index->type == ekCONST || index->type == ekENUMV)
        {
            stm_writef(latex, ",\n");
            stm_writef(latex, "        ");
            if (str_str(tc(index->name), "::") != NULL)
            {
                String *right;
                str_split(tc(index->name), "::", NULL, &right);
                str_writef(latex, right);
                str_destroy(&right);
            }
            else
            {
                str_writef(latex, index->name);
            }
        }
    arrst_end()
}

/*---------------------------------------------------------------------------*/

void loader_funtions_to_latex(const Loader *loader, Stream *latex)
{
    stm_writef(latex, "        ");
    stm_writef(latex, "i_create");
    cassert_no_null(loader);
    arrst_foreach(index, loader->functions, DIndex)
        if (index->type == ekFUNC)
        {
            stm_writef(latex, ",\n");
            stm_writef(latex, "        ");
            if (str_str(tc(index->name), "::") != NULL)
            {
                String *right;
                str_split(tc(index->name), "::", NULL, &right);
                str_writef(latex, right);
                str_destroy(&right);
            }
            else
            {
                str_writef(latex, index->name);
            }
        }
    arrst_end()
}

/*---------------------------------------------------------------------------*/

void loader_web_lnav(const Loader *loader, const WebSec *section, const Doc *doc, LNav *lnav, Stream *html)
{
    cassert_no_null(loader);
    cassert_no_null(section);
    arrpt_foreach(packname, section->packs, String)
        Pack *pack = i_get_pack(loader->packs, tc(packname));
        arrst_foreach(group, pack->conf->web, PkGroup)
            const String *fstr = arrpt_get(group->docs, 0, String);
            const Doc *fdoc = i_get_doc(pack->docs, tc(fstr));
            web_part(html, i_str(group->title, loader->lang_id), tc(pack->name), doc, fdoc, lnav);
            arrpt_foreach(gdoc, group->docs, String)
                const Doc *pdoc = i_get_doc(pack->docs, tc(gdoc));
                web_doc_lnav(html, doc, pdoc, lnav);
            arrpt_end()
        arrst_end()
    arrpt_end()
}

/*---------------------------------------------------------------------------*/

const Lang *loader_lang(const Loader *loader)
{
    cassert_no_null(loader);
    cassert_no_null(loader->config);
    return arrst_get_const(loader->config->langs, loader->lang_id, Lang);
}
