/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: sched.h
 *
 */

/* nbuild scheduler */

#include "nbuild.hxx"

void sched_start(const Global *global, ArrSt(SJob) *seljobs, const ArrSt(Host) *hosts, const Drive *drive, const ArrSt(Target) *tests, const WorkPaths *wpaths, const char_t *flowid, const uint32_t repo_vers, Report *report);
