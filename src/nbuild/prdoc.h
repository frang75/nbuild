/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: prdoc.h
 *
 */

/* Process project documentation */

#include "nbuild.hxx"

void prdoc_dbind(void);

bool_t prdoc_generate(const Global *global, const Login *drive, const char_t *project_vers, const uint32_t repo_vers, const uint32_t doc_repo_vers, const WorkPaths *wpaths, Report *report);

bool_t prdoc_buildrep_generate(const Global *global, const ArrSt(Job) *jobs, const Login *drive, const char_t *project_vers, const uint32_t repo_vers, const WorkPaths *wpaths, const Report *report);
