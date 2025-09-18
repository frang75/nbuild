/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: host.h
 *
 */

/* Build host (runner) */

#include "nbuild.hxx"

void host_dbind(void);

bool_t host_check_config(const ArrSt(Host) *hosts);

const char_t *host_name(const Host *host);

const char_t *host_type(const Host *host);

const char_t *host_vbox_uuid(const Host *host);

const char_t *host_vbox_host(const Host *host);

const char_t *host_utm_uuid(const Host *host);

const char_t *host_utm_host(const Host *host);

const char_t *host_vmware_path(const Host *host);

const char_t *host_vmware_host(const Host *host);

const char_t *host_macos_host(const Host *host);

const char_t *host_macos_volume(const Host *host);

const Login *host_login(const Host *host);

void host_localhost(ArrSt(Host) *hosts, const ArrSt(uint32_t) *ips);

const Host *host_match_job(const ArrSt(Host) *hosts, const Job *job);

const Host *host_by_name(const ArrSt(Host) *hosts, const char_t *name);

const Host *host_macos_alive(const ArrSt(Host) *hosts, const char_t *macos_host);

macos_t host_macos_version(const Host *host);

bool_t host_run_build(const Host *host, const Drive *drive, const Job *job, const char_t *project, const WorkPaths *wpaths, const uint32_t repo_vers, const char_t *flowid, const uint32_t runner_id, String **cmake_log, String **build_log, String **install_log, String **warns, String **errors, uint32_t *nwarns, uint32_t *nerrors, String **error_msg);

bool_t host_run_test(const Host *host, const Job *job, const ArrSt(Target) *tests, const WorkPaths *wpaths, const uint32_t repo_vers, const char_t *flowid, const uint32_t runner_id, String **cmake_log, String **build_log, String **install_log, String **warns, String **errors, uint32_t *nwarns, uint32_t *nerrors, String **error_msg);
