/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: report.h
 *
 */

/* Build report */

#include "nbuild.hxx"

void report_dbind(void);

void report_init(Report *report, const char_t *repo_url, const uint32_t repo_vers);

void report_loop_incr(Report *report);

void report_loop_init(Report *report);

void report_loop_end(Report *report, const char_t *logfile);

uint32_t report_loop_current(const Report *report);

uint32_t report_loop_seconds(const Report *report, const uint32_t loop_id);

void report_event_state(Report *report, const REvent *event, RState *state);

void report_event_init(Report *report, REvent *event);

void report_event_end(Report *report, REvent *event, const bool_t ok, String **error_msg);

REvent *report_target_event(Report *report, const char_t *name);

REvent *report_test_event(Report *report, const char_t *name);

REvent *report_src_tar_event(Report *report);

REvent *report_test_tar_event(Report *report);

void report_target_set(Report *report, const char_t *name, const bool_t with_legal, const bool_t with_format, const bool_t with_analyzer);

void report_test_set(Report *report, const char_t *name, const bool_t with_legal, const bool_t with_format, const bool_t with_analyzer);

void report_build_file_state(Report *report, RState *state);

void report_build_file_init(Report *report);

void report_build_file_end(Report *report, const bool_t ok, String **error_msg);

void report_doc_state(Report *report, const uint32_t doc_repo_vers, RState *state);

void report_doc_init(Report *report, const uint32_t doc_repo_vers);

void report_doc_end(Report *report, const uint32_t doc_repo_vers, const bool_t ok, String **error_msg);

void report_doc_ndoc_state(Report *report, const uint32_t doc_repo_vers, RState *state);

void report_doc_ndoc_init(Report *report, const uint32_t doc_repo_vers);

void report_doc_ndoc_end(Report *report, const uint32_t doc_repo_vers, const bool_t ok, String **error_msg);

void report_doc_ebook_state(Report *report, const uint32_t doc_repo_vers, const char_t *lang, RState *state);

void report_doc_ebook_init(Report *report, const uint32_t doc_repo_vers, const char_t *lang);

void report_doc_ebook_end(Report *report, const uint32_t doc_repo_vers, const char_t *lang, const bool_t ok, String **error_msg);

void report_doc_copy_state(Report *report, const uint32_t doc_repo_vers, RState *state);

void report_doc_copy_init(Report *report, const uint32_t doc_repo_vers);

void report_doc_copy_end(Report *report, const uint32_t doc_repo_vers, const bool_t ok, String **error_msg);

void report_doc_upload_state(Report *report, const uint32_t doc_repo_vers, RState *state);

void report_doc_upload_init(Report *report, const uint32_t doc_repo_vers);

void report_doc_upload_end(Report *report, const uint32_t doc_repo_vers, const bool_t ok, String **error_msg);

void report_doc(Report *report, const uint32_t doc_repo_vers, String **hosting_url, String **stdout_b64, String **stderr_b64, String **warns_b64, String **errors_b64, const bool_t in_cache, const uint32_t ndoc_ret, const uint32_t nwarns, const uint32_t nerrors);

void report_job_state(Report *report, const uint32_t job_id, const char_t *step_id, RState *state);

void report_job_init(Report *report, const uint32_t job_id, const char_t *step_id);

void report_job_end(Report *report, const uint32_t job_id, const char_t *step_id, const bool_t ok, String **error_msg);

void report_job(Report *report, const uint32_t job_id, const char_t *step_id, const char_t *hostname, String **cmake_log, String **build_log, String **install_log, String **warns, String **errors, const uint32_t nwarns, const uint32_t nerrors);

bool_t report_job_can_test(const Report *report, const uint32_t job_id);

const char_t *report_job_host(const Report *report, const SJob *sjob);

bool_t report_can_start_jobs(const Report *report, const uint32_t doc_repo_vers);

void report_force_jobs(Report *report, const char_t *job_pattern, const ArrSt(Job) *jobs, ArrSt(SJob) *seljobs, const bool_t with_tests);

void report_select_jobs(Report *report, const ArrSt(Job) *jobs, ArrSt(SJob) *seljobs, const bool_t with_tests);

void report_log(const Report *report, const Global *global, const uint32_t repo_vers);

Stream *report_ndoc_page(const Report *report, const ArrSt(Job) *jobs, const Global *global, const char_t *project_vers);

void report_state_log(const RState *state, const char_t *msg);
