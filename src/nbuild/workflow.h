/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: workflow.h
 *
 */

/* Workflows */

#include "nbuild.hxx"

void workflow_dbind(void);

Workflows *workflow_create(const char_t *workflow_file);

String *workflow_run(Workflows *workflows, const Network *network, const char_t *forced_jobs, const char_t *logfile, const char_t *tmppath);
