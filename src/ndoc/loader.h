/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: loader.h
 *
 */

/* Document loader */

#include "ndoc.hxx"

void loader_dbind(void);

const Doc *loader_doc(const Loader *loader, const char_t *docname);

const Doc *loader_section(const Loader *loader, const char_t *name, const Doc *curdoc, const Block **block);

const Doc *loader_type(const Loader *loader, const char_t *name, const Block **block);

const Doc *loader_func(const Loader *loader, const char_t *name, const Block **block);

const Doc *loader_keyword(const Loader *loader, const char_t *name, const Block **block);

const Doc *loader_prev_doc(const Loader *loader, const Doc *doc);

const Doc *loader_next_doc(const Loader *loader, const Doc *doc);

const char_t *loader_float_type(const Loader *loader, const char_t *name);

const char_t *loader_double_type(const Loader *loader, const char_t *name);

DIndex *loader_section_index(Loader *loader);

DIndex *loader_type_index(Loader *loader);

DIndex *loader_function_index(Loader *loader);

ResPack *loader_respack(const Loader *loader);

Loader *loader_run(
    const char_t *docpath,
    const char_t *src_repo_url,
    const char_t *src_repo_user,
    const char_t *src_repo_pass,
    const uint32_t src_repo_vers,
    const Config *config,
    const uint32_t lang_id);

void loader_compare(const Loader *loader1, const Loader *loader2);

void loader_destroy(Loader **loader);

void loader_latex(const Loader *loader, const char_t *texpath, const char_t *project_vers);

void loader_web(const Loader *loader, WSite *site, const char_t *project_vers);

void loader_types_to_latex(const Loader *loader, Stream *latex);

void loader_constants_to_latex(const Loader *loader, Stream *latex);

void loader_funtions_to_latex(const Loader *loader, Stream *latex);

void loader_web_lnav(const Loader *loader, const WebSec *section, const Doc *doc, LNav *lnav, Stream *html);

const Lang *loader_lang(const Loader *loader);
