/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: nlib.c
 *
 */

/* Commons for NAppGUI Utilities */

#include "nlib.h"

/* https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797 */
const char_t *kASCII_BLACK = "\033[0;30m";
const char_t *kASCII_RED = "\033[0;31m";
const char_t *kASCII_GREEN = "\033[0;32m";
const char_t *kASCII_YELLOW = "\033[0;33m";
const char_t *kASCII_BLUE = "\033[0;34m";
const char_t *kASCII_MAGENTA = "\033[0;35m";
const char_t *kASCII_CYAN = "\033[0;36m";
const char_t *kASCII_WHITE = "\033[0;37m";
const char_t *kASCII_RESET = "\033[0m";
const char_t *kASCII_BG_RED = "\033[41m";

const char_t *kASCII_OK = "\033[0;32m[OK]\033[0m";
const char_t *kASCII_WARN = "\033[0;33m[WARN]\033[0m";
const char_t *kASCII_FAIL = "\033[0;31m[FAIL]\033[0m";
const char_t *kASCII_ERROR = "\033[0;31m";
const char_t *kASCII_PATH = "\033[0;34m";
const char_t *kASCII_TARGET = "\033[0;35m";
const char_t *kASCII_VERSION = "\033[0;36m";
const char_t *kASCII_SCHED = "\033[0;32m[SCHED]\033[0m";
const char_t *kASCII_SCHED_FAIL = "\033[0;31m[SCHED]\033[0m";
const char_t *kASCII_SCHED_WARN = "\033[0;33m[SCHED]\033[0m";
const char_t *kASCII_BOOT = "\033[0;32m[NBOOT]\033[0m";
const char_t *kASCII_BOOT_FAIL = "\033[0;31m[NBOOT]\033[0m";
