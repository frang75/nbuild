/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: doc.h
 *
 */

/* Document */

#include "ndoc.hxx"

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
    Loader *loader);

void doc_compare(const Doc *doc1, const Doc *doc2, const char_t *lang1, const char_t *lang2);

void doc_destroy(Doc **doc);

const char_t *doc_name(const Doc *doc);

const char_t *doc_packname(const Doc *doc);

const char_t *doc_refid(const Doc *doc, const char_t *refname, btype_t *type);

uint32_t doc_section(const Doc *doc, const char_t *secname);

const Block *doc_h1(const Doc *doc);

const Block *doc_block(const Doc *doc, const uint32_t index);

const Block *doc_type(const Doc *doc, const uint32_t index, const uint32_t child_id);

const Block *doc_func(const Doc *doc, const uint32_t index);

const char_t *doc_block_param(const Block *block, const uint32_t index);

real32_t doc_real_param(const Block *block, const uint32_t index);

uint32_t doc_int_param(const Block *block, const uint32_t index);

bool_t doc_is_real_type(const char_t *type, const Loader *loader);

const char_t *doc_float_type(const char_t *type, const Loader *loader);

const char_t *doc_double_type(const char_t *type, const Loader *loader);

const char_t *doc_real_type(const char_t *type, const Loader *loader);

String *doc_template_constant(const char_t *type, const char_t *name);

String *doc_float_constant(const char_t *type, const char_t *name);

String *doc_double_constant(const char_t *type, const char_t *name);

String *doc_real_constant(const char_t *name);

String *doc_template_func(const char_t *base, const char_t *fname);

String *doc_float_func(const char_t *base, const char_t *fname);

String *doc_double_func(const char_t *base, const char_t *fname);

String *doc_real_func(const char_t *base, const char_t *fname);

String *doc_template_struct(const char_t *sname);

String *doc_float_struct(const char_t *sname);

String *doc_double_struct(const char_t *sname);

void doc_func_params(const Block *func, const Block **fret, const Block **fparams, uint32_t *nparams);

void doc_latex(const ArrPt(String) *names, const ArrPt(Doc) *docs, const char_t *title, Stream *latex, const Config *config, const uint32_t lang_id, const Loader *loader, const char_t *docpath, const char_t *prespath, const bool_t write_sections, const bool_t write_funcs);

void doc_web(const Doc *doc, WSite *site, const Config *config, const uint32_t lang_id, const Loader *loader, const char_t *docpath, const WebSec *section, const uint32_t menu_id);

bool_t doc_secnums(const Doc *doc);
