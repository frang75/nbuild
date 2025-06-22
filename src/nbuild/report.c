/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: report.c
 *
 */

/* Build report */

#include "report.h"
#include "host.h"
#include <nlib/nlib.h>
#include <encode/base64.h>
#include <core/arrst.h>
#include <core/arrpt.h>
#include <core/buffer.h>
#include <core/date.h>
#include <core/dbind.h>
#include <core/regex.h>
#include <core/strings.h>
#include <core/stream.h>
#include <osbs/log.h>
#include <sewer/cassert.h>
#include <sewer/blib.h>
#include <sewer/bstd.h>

typedef struct _rloop_t RLoop;
typedef struct _rtarget_t RTarget;
typedef struct _rdoc_t RDoc;
typedef struct _rstep_t RStep;
typedef struct _rjob_t RJob;

struct _rloop_t
{
    Date init;
    Date end;
    String *log;
};

struct _revent_t
{
    String *name;
    uint32_t loop_id;
    Date init;
    Date end;
    int32_t seconds;
    String *error_msg;
};

struct _rtarget_t
{
    REvent event;
    bool_t legal;
    bool_t format;
    bool_t analyzer;
};

struct _rdoc_t
{
    REvent event;
    REvent ndoc_event;
    REvent ebook_es_event;
    REvent ebook_en_event;
    REvent copy_event;
    REvent upload_event;
    uint32_t doc_repo_vers;
    String *hosting_url;
    String *stdout_;
    String *stderr_;
    String *warns;
    String *errors;
    bool_t in_cache;
    uint32_t ret;
    uint32_t nwarns;
    uint32_t nerrors;
};

struct _rstep_t
{
    REvent event;
    String *name;
    String *cmake_log;
    String *build_log;
    String *install_log;
    String *warns;
    String *errors;
    uint32_t nwarns;
    uint32_t nerrors;
};

struct _rjob_t
{
    uint32_t priority;
    String *name;
    String *hostname;
    String *generator;
    ArrSt(RStep) *steps;
};

struct _report_t
{
    String *repo_url;
    uint32_t repo_vers;
    uint32_t loop_id;
    ArrSt(RLoop) *loops;
    ArrSt(RTarget) *targets;
    ArrSt(RTarget) *tests;
    ArrSt(RDoc) *docs;
    ArrSt(RJob) *jobs;
    REvent build_file;
    REvent src_tar;
    REvent test_tar;
};

DeclSt(RLoop);
DeclSt(REvent);
DeclSt(RTarget);
DeclSt(RDoc);
DeclSt(RStep);
DeclSt(RJob);

/*---------------------------------------------------------------------------*/

void report_dbind(void)
{
    dbind(Date, int16_t, year);
    dbind(Date, uint8_t, month);
    dbind(Date, uint8_t, wday);
    dbind(Date, uint8_t, mday);
    dbind(Date, uint8_t, hour);
    dbind(Date, uint8_t, minute);
    dbind(Date, uint8_t, second);
    dbind(RLoop, Date, init);
    dbind(RLoop, Date, end);
    dbind(RLoop, String *, log);
    dbind(REvent, String *, name);
    dbind(REvent, uint32_t, loop_id);
    dbind(REvent, Date, init);
    dbind(REvent, Date, end);
    dbind(REvent, int32_t, seconds);
    dbind(REvent, String *, error_msg);
    dbind(RTarget, REvent, event);
    dbind(RTarget, bool_t, legal);
    dbind(RTarget, bool_t, format);
    dbind(RTarget, bool_t, analyzer);
    dbind(RDoc, REvent, event);
    dbind(RDoc, REvent, ndoc_event);
    dbind(RDoc, REvent, ebook_es_event);
    dbind(RDoc, REvent, ebook_en_event);
    dbind(RDoc, REvent, copy_event);
    dbind(RDoc, REvent, upload_event);
    dbind(RDoc, uint32_t, doc_repo_vers);
    dbind(RDoc, String *, hosting_url);
    dbind(RDoc, String *, stdout_);
    dbind(RDoc, String *, stderr_);
    dbind(RDoc, String *, warns);
    dbind(RDoc, String *, errors);
    dbind(RDoc, bool_t, in_cache);
    dbind(RDoc, uint32_t, ret);
    dbind(RDoc, uint32_t, nwarns);
    dbind(RDoc, uint32_t, nerrors);
    dbind(RStep, REvent, event);
    dbind(RStep, String *, name);
    dbind(RStep, String *, cmake_log);
    dbind(RStep, String *, build_log);
    dbind(RStep, String *, install_log);
    dbind(RStep, String *, warns);
    dbind(RStep, String *, errors);
    dbind(RStep, uint32_t, nwarns);
    dbind(RStep, uint32_t, nerrors);
    dbind(RJob, uint32_t, priority);
    dbind(RJob, String *, name);
    dbind(RJob, String *, hostname);
    dbind(RJob, String *, generator);
    dbind(RJob, ArrSt(RStep) *, steps);
    dbind(Report, String *, repo_url);
    dbind(Report, uint32_t, repo_vers);
    dbind(Report, uint32_t, loop_id);
    dbind(Report, ArrSt(RLoop) *, loops);
    dbind(Report, ArrSt(RTarget) *, targets);
    dbind(Report, ArrSt(RTarget) *, tests);
    dbind(Report, ArrSt(RDoc) *, docs);
    dbind(Report, ArrSt(RJob) *, jobs);
    dbind(Report, REvent, build_file);
    dbind(Report, REvent, src_tar);
    dbind(Report, REvent, test_tar);
    dbind_default(REvent, uint32_t, loop_id, UINT32_MAX);
}

/*---------------------------------------------------------------------------*/

static int i_target_cmp(const RTarget *target, const char_t *name)
{
    cassert_no_null(target);
    return str_cmp(target->event.name, name);
}

/*---------------------------------------------------------------------------*/

static int i_doc_cmp(const RDoc *doc, const uint32_t *doc_repo_vers)
{
    cassert_no_null(doc);
    cassert_no_null(doc_repo_vers);
    return (int)doc->doc_repo_vers - *cast_const(doc_repo_vers, uint32_t);
}

/*---------------------------------------------------------------------------*/

static int i_job_cmp(const RJob *job, const char_t *name)
{
    cassert_no_null(job);
    return str_cmp(job->name, name);
}

/*---------------------------------------------------------------------------*/

static bool_t i_is_done(const REvent *event)
{
    cassert_no_null(event);
    return event->seconds > 0;
}

/*---------------------------------------------------------------------------*/

static RTarget *i_get_target(ArrSt(RTarget) *targets, const char_t *name)
{
    RTarget *target = arrst_search(targets, i_target_cmp, name, NULL, RTarget, char_t);
    if (target == NULL)
    {
        target = arrst_new(targets, RTarget);
        dbind_init(target, RTarget);
        str_upd(&target->event.name, name);
        target->event.seconds = -1;
    }
    return target;
}

/*---------------------------------------------------------------------------*/

static RDoc *i_get_doc(ArrSt(RDoc) *docs, const uint32_t doc_repo_vers)
{
    RDoc *doc = arrst_search(docs, i_doc_cmp, &doc_repo_vers, NULL, RDoc, uint32_t);
    if (doc == NULL)
    {
        doc = arrst_new(docs, RDoc);
        dbind_init(doc, RDoc);
        doc->doc_repo_vers = doc_repo_vers;
    }
    return doc;
}

/*---------------------------------------------------------------------------*/

static void i_add_job(ArrSt(RJob) *jobs, const char_t *name, const char_t *generator, const uint32_t priority, const bool_t with_tests)
{
    RJob *job = arrst_search(jobs, i_job_cmp, name, NULL, RJob, char_t);
    if (job == NULL)
    {
        job = arrst_new(jobs, RJob);
        dbind_init(job, RJob);
        str_upd(&job->name, name);
        str_upd(&job->generator, generator);
        job->priority = priority;

        /* Steps (ready for configurable pipelines) */
        {
            RStep *step = arrst_new(job->steps, RStep);
            dbind_init(step, RStep);
            str_upd(&step->name, "build");
        }

        if (with_tests == TRUE)
        {
            RStep *step = arrst_new(job->steps, RStep);
            dbind_init(step, RStep);
            str_upd(&step->name, "test");
        }
    }
}

/*---------------------------------------------------------------------------*/

static RJob *i_get_job(ArrSt(RJob) *jobs, const char_t *name, uint32_t *id)
{
    return arrst_search(jobs, i_job_cmp, name, id, RJob, char_t);
}

/*---------------------------------------------------------------------------*/

static RJob *i_get_job_if_pending(ArrSt(RJob) *jobs, const char_t *name, uint32_t *id)
{
    RJob *job = i_get_job(jobs, name, id);
    RStep *step = NULL;
    cassert_no_null(job);
    step = arrst_first(job->steps, RStep);
    if (i_is_done(&step->event) == FALSE)
        return job;
    return NULL;
}

/*---------------------------------------------------------------------------*/

static void i_event_init(REvent *event, const Report *report)
{
    cassert_no_null(report);
    event->init = date_system();
    event->seconds = -1;
    event->loop_id = report->loop_id;
}

/*---------------------------------------------------------------------------*/

static void i_event_end(REvent *event, const bool_t ok, String **error_msg)
{
    cassert_no_null(event);
    cassert_no_null(error_msg);
    event->end = date_system();

    event->seconds = (int32_t)date_ellapsed_seconds(&event->init, &event->end);
    cassert(event->seconds >= 0);
    if (event->seconds == 0)
        event->seconds = 1;

    if (ok == TRUE)
    {
        cassert(*error_msg == NULL);
        str_upd(&event->error_msg, "");
    }
    else
    {
        cassert(*error_msg != NULL);
        cassert(str_empty(*error_msg) == FALSE);
        str_upd(&event->error_msg, tc(*error_msg));
        str_destroy(error_msg);
    }
}

/*---------------------------------------------------------------------------*/

static void i_event_state(const REvent *event, RState *state)
{
    cassert_no_null(event);
    cassert_no_null(state);

    /* The event has done */
    if (event->seconds > 0)
    {
        state->done = TRUE;
        state->date = event->end;
    }
    else
    {
        state->done = FALSE;
        state->date = event->init;
    }

    state->seconds = event->seconds;
    state->loop_id = event->loop_id;

    if (str_empty(event->error_msg) == TRUE)
        state->error_msg = NULL;
    else
        state->error_msg = tc(event->error_msg);
}

/*---------------------------------------------------------------------------*/

void report_init(Report *report, const char_t *repo_url, const uint32_t repo_vers)
{
    cassert_no_null(report);
    cassert(str_empty(report->repo_url) == TRUE);
    cassert(report->repo_vers == 0);
    cassert(report->loop_id == 0);
    str_upd(&report->repo_url, repo_url);
    report->repo_vers = repo_vers;
}

/*---------------------------------------------------------------------------*/

void report_loop_incr(Report *report)
{
    cassert_no_null(report);
    report->loop_id += 1;
}

/*---------------------------------------------------------------------------*/

void report_loop_init(Report *report)
{
    RLoop *loop = NULL;
    cassert_no_null(report);
    cassert(arrst_size(report->loops, RLoop) == report->loop_id);
    loop = arrst_new(report->loops, RLoop);
    dbind_init(loop, RLoop);
    loop->init = date_system();
}

/*---------------------------------------------------------------------------*/

void report_loop_end(Report *report, const char_t *logfile)
{
    RLoop *loop = NULL;
    cassert_no_null(report);
    cassert(arrst_size(report->loops, RLoop) == report->loop_id + 1);
    loop = arrst_last(report->loops, RLoop);
    cassert(date_is_null(&loop->end) == TRUE);
    loop->end = date_system();
    str_destroy(&loop->log);
    loop->log = b64_encode_from_file(logfile, NULL);
}

/*---------------------------------------------------------------------------*/

uint32_t report_loop_current(const Report *report)
{
    cassert_no_null(report);
    return report->loop_id;
}

/*---------------------------------------------------------------------------*/

uint32_t report_loop_seconds(const Report *report, const uint32_t loop_id)
{
    const RLoop *loop = NULL;
    int32_t seconds = 0;
    cassert_no_null(report);
    if (loop_id == UINT32_MAX)
        loop = arrst_last_const(report->loops, RLoop);
    else
        loop = arrst_get_const(report->loops, loop_id, RLoop);

    seconds = (int32_t)date_ellapsed_seconds(&loop->init, &loop->end);
    cassert(seconds >= 0);
    return (uint32_t)seconds;
}

/*---------------------------------------------------------------------------*/

void report_event_state(Report *report, const REvent *event, RState *state)
{
    unref(report);
    i_event_state(event, state);
}

/*---------------------------------------------------------------------------*/

void report_event_init(Report *report, REvent *event)
{
    i_event_init(event, report);
}

/*---------------------------------------------------------------------------*/

void report_event_end(Report *report, REvent *event, const bool_t ok, String **error_msg)
{
    unref(report);
    i_event_end(event, ok, error_msg);
}

/*---------------------------------------------------------------------------*/

REvent *report_target_event(Report *report, const char_t *name)
{
    RTarget *target = NULL;
    cassert_no_null(report);
    target = i_get_target(report->targets, name);
    cassert_no_null(target);
    return &target->event;
}

/*---------------------------------------------------------------------------*/

REvent *report_test_event(Report *report, const char_t *name)
{
    RTarget *target = NULL;
    cassert_no_null(report);
    target = i_get_target(report->tests, name);
    cassert_no_null(target);
    return &target->event;
}

/*---------------------------------------------------------------------------*/

REvent *report_src_tar_event(Report *report)
{
    cassert_no_null(report);
    return &report->src_tar;
}

/*---------------------------------------------------------------------------*/

REvent *report_test_tar_event(Report *report)
{
    cassert_no_null(report);
    return &report->test_tar;
}

/*---------------------------------------------------------------------------*/

void report_target_set(Report *report, const char_t *name, const bool_t with_legal, const bool_t with_format, const bool_t with_analyzer)
{
    RTarget *target = NULL;
    cassert_no_null(report);
    target = i_get_target(report->targets, name);
    cassert_no_null(target);
    target->legal = with_legal;
    target->format = with_format;
    target->analyzer = with_analyzer;
}

/*---------------------------------------------------------------------------*/

void report_test_set(Report *report, const char_t *name, const bool_t with_legal, const bool_t with_format, const bool_t with_analyzer)
{
    RTarget *target = NULL;
    cassert_no_null(report);
    target = i_get_target(report->tests, name);
    cassert_no_null(target);
    target->legal = with_legal;
    target->format = with_format;
    target->analyzer = with_analyzer;
}

/*---------------------------------------------------------------------------*/

void report_build_file_state(Report *report, RState *state)
{
    cassert_no_null(report);
    i_event_state(&report->build_file, state);
}

/*---------------------------------------------------------------------------*/

void report_build_file_init(Report *report)
{
    cassert_no_null(report);
    i_event_init(&report->build_file, report);
}

/*---------------------------------------------------------------------------*/

void report_build_file_end(Report *report, const bool_t ok, String **error_msg)
{
    cassert_no_null(report);
    i_event_end(&report->build_file, ok, error_msg);
}

/*---------------------------------------------------------------------------*/

static REvent *i_doc_event(Report *report, const uint32_t doc_repo_vers, const char_t *evtype)
{
    RDoc *rdoc = NULL;
    cassert_no_null(report);
    rdoc = i_get_doc(report->docs, doc_repo_vers);
    cassert_no_null(rdoc);
    if (str_equ_c(evtype, "doc") == TRUE)
        return &rdoc->event;
    else if (str_equ_c(evtype, "ndoc") == TRUE)
        return &rdoc->ndoc_event;
    else if (str_equ_c(evtype, "es") == TRUE)
        return &rdoc->ebook_es_event;
    else if (str_equ_c(evtype, "en") == TRUE)
        return &rdoc->ebook_en_event;
    else if (str_equ_c(evtype, "copy") == TRUE)
        return &rdoc->copy_event;
    else if (str_equ_c(evtype, "upload") == TRUE)
        return &rdoc->upload_event;
    cassert(FALSE);
    return NULL;
}

/*---------------------------------------------------------------------------*/

void report_doc_state(Report *report, const uint32_t doc_repo_vers, RState *state)
{
    REvent *event = i_doc_event(report, doc_repo_vers, "doc");
    i_event_state(event, state);
}

/*---------------------------------------------------------------------------*/

void report_doc_init(Report *report, const uint32_t doc_repo_vers)
{
    REvent *event = i_doc_event(report, doc_repo_vers, "doc");
    i_event_init(event, report);
}

/*---------------------------------------------------------------------------*/

void report_doc_end(Report *report, const uint32_t doc_repo_vers, const bool_t ok, String **error_msg)
{
    REvent *event = i_doc_event(report, doc_repo_vers, "doc");
    i_event_end(event, ok, error_msg);
}

/*---------------------------------------------------------------------------*/

void report_doc_ndoc_state(Report *report, const uint32_t doc_repo_vers, RState *state)
{
    REvent *event = i_doc_event(report, doc_repo_vers, "ndoc");
    i_event_state(event, state);
}

/*---------------------------------------------------------------------------*/

void report_doc_ndoc_init(Report *report, const uint32_t doc_repo_vers)
{
    REvent *event = i_doc_event(report, doc_repo_vers, "ndoc");
    i_event_init(event, report);
}

/*---------------------------------------------------------------------------*/

void report_doc_ndoc_end(Report *report, const uint32_t doc_repo_vers, const bool_t ok, String **error_msg)
{
    REvent *event = i_doc_event(report, doc_repo_vers, "ndoc");
    i_event_end(event, ok, error_msg);
}

/*---------------------------------------------------------------------------*/

void report_doc_ebook_state(Report *report, const uint32_t doc_repo_vers, const char_t *lang, RState *state)
{
    REvent *event = i_doc_event(report, doc_repo_vers, lang);
    i_event_state(event, state);
}

/*---------------------------------------------------------------------------*/

void report_doc_ebook_init(Report *report, const uint32_t doc_repo_vers, const char_t *lang)
{
    REvent *event = i_doc_event(report, doc_repo_vers, lang);
    i_event_init(event, report);
}

/*---------------------------------------------------------------------------*/

void report_doc_ebook_end(Report *report, const uint32_t doc_repo_vers, const char_t *lang, const bool_t ok, String **error_msg)
{
    REvent *event = i_doc_event(report, doc_repo_vers, lang);
    i_event_end(event, ok, error_msg);
}

/*---------------------------------------------------------------------------*/

void report_doc_copy_state(Report *report, const uint32_t doc_repo_vers, RState *state)
{
    REvent *event = i_doc_event(report, doc_repo_vers, "copy");
    i_event_state(event, state);
}

/*---------------------------------------------------------------------------*/

void report_doc_copy_init(Report *report, const uint32_t doc_repo_vers)
{
    REvent *event = i_doc_event(report, doc_repo_vers, "copy");
    i_event_init(event, report);
}

/*---------------------------------------------------------------------------*/

void report_doc_copy_end(Report *report, const uint32_t doc_repo_vers, const bool_t ok, String **error_msg)
{
    REvent *event = i_doc_event(report, doc_repo_vers, "copy");
    i_event_end(event, ok, error_msg);
}

/*---------------------------------------------------------------------------*/

void report_doc_upload_state(Report *report, const uint32_t doc_repo_vers, RState *state)
{
    REvent *event = i_doc_event(report, doc_repo_vers, "upload");
    i_event_state(event, state);
}

/*---------------------------------------------------------------------------*/

void report_doc_upload_init(Report *report, const uint32_t doc_repo_vers)
{
    REvent *event = i_doc_event(report, doc_repo_vers, "upload");
    i_event_init(event, report);
}

/*---------------------------------------------------------------------------*/

void report_doc_upload_end(Report *report, const uint32_t doc_repo_vers, const bool_t ok, String **error_msg)
{
    REvent *event = i_doc_event(report, doc_repo_vers, "upload");
    i_event_end(event, ok, error_msg);
}

/*---------------------------------------------------------------------------*/

void report_doc(Report *report, const uint32_t doc_repo_vers, String **hosting_url, String **stdout_b64, String **stderr_b64, String **warns_b64, String **errors_b64, const bool_t in_cache, const uint32_t ndoc_ret, const uint32_t nwarns, const uint32_t nerrors)
{
    RDoc *rdoc = NULL;
    cassert_no_null(report);
    rdoc = i_get_doc(report->docs, doc_repo_vers);
    cassert_no_null(rdoc);
    cassert_no_null(hosting_url);
    cassert_no_null(stdout_b64);
    cassert_no_null(stderr_b64);
    str_destroy(&rdoc->hosting_url);
    str_destroy(&rdoc->stdout_);
    str_destroy(&rdoc->stderr_);
    str_destroy(&rdoc->warns);
    str_destroy(&rdoc->errors);
    rdoc->in_cache = in_cache;
    rdoc->hosting_url = *hosting_url;
    rdoc->stdout_ = *stdout_b64;
    rdoc->stderr_ = *stderr_b64;
    rdoc->warns = *warns_b64;
    rdoc->errors = *errors_b64;
    rdoc->in_cache = in_cache;
    rdoc->ret = ndoc_ret;
    rdoc->nwarns = nwarns;
    rdoc->nerrors = nerrors;
    *hosting_url = NULL;
    *stdout_b64 = NULL;
    *stderr_b64 = NULL;
    *warns_b64 = NULL;
    *errors_b64 = NULL;
}

/*---------------------------------------------------------------------------*/

static RStep *i_get_step(ArrSt(RJob) *jobs, const uint32_t job_id, const char_t *step_id)
{
    RJob *job = arrst_get(jobs, job_id, RJob);
    cassert_no_null(job);
    arrst_foreach(step, job->steps, RStep)
        if (str_equ(step->name, step_id) == TRUE)
            return step;
    arrst_end()
    return NULL;
}

/*---------------------------------------------------------------------------*/

static REvent *i_get_event(ArrSt(RJob) *jobs, const uint32_t job_id, const char_t *step_id)
{
    RStep *step = i_get_step(jobs, job_id, step_id);
    cassert_no_null(step);
    return &step->event;
}

/*---------------------------------------------------------------------------*/

void report_job_state(Report *report, const uint32_t job_id, const char_t *step_id, RState *state)
{
    REvent *event = NULL;
    cassert_no_null(report);
    event = i_get_event(report->jobs, job_id, step_id);
    i_event_state(event, state);
}

/*---------------------------------------------------------------------------*/

void report_job_init(Report *report, const uint32_t job_id, const char_t *step_id)
{
    REvent *event = NULL;
    cassert_no_null(report);
    event = i_get_event(report->jobs, job_id, step_id);
    i_event_init(event, report);
}

/*---------------------------------------------------------------------------*/

void report_job_end(Report *report, const uint32_t job_id, const char_t *step_id, const bool_t ok, String **error_msg)
{
    REvent *event = NULL;
    cassert_no_null(report);
    event = i_get_event(report->jobs, job_id, step_id);
    i_event_end(event, ok, error_msg);
}

/*---------------------------------------------------------------------------*/

void report_job(Report *report, const uint32_t job_id, const char_t *step_id, const char_t *hostname, String **cmake_log, String **build_log, String **install_log, String **warns, String **errors, const uint32_t nwarns, const uint32_t nerrors)
{
    RJob *job = NULL;
    RStep *step = NULL;
    cassert_no_null(report);
    job = arrst_get(report->jobs, job_id, RJob);
    cassert_no_null(job);
    cassert(str_empty(job->hostname) || str_equ(job->hostname, hostname));
    cassert_no_null(cmake_log);
    cassert_no_null(build_log);
    cassert_no_null(install_log);
    cassert_no_null(warns);
    cassert_no_null(errors);
    str_upd(&job->hostname, hostname);
    step = i_get_step(report->jobs, job_id, step_id);
    cassert_no_null(step);

    if (*cmake_log != NULL)
    {
        str_destroy(&step->cmake_log);
        step->cmake_log = *cmake_log;
        *cmake_log = NULL;
    }

    if (*build_log != NULL)
    {
        str_destroy(&step->build_log);
        step->build_log = *build_log;
        *build_log = NULL;
    }

    if (*install_log != NULL)
    {
        str_destroy(&step->install_log);
        step->install_log = *install_log;
        *install_log = NULL;
    }

    if (*warns != NULL)
    {
        str_destroy(&step->warns);
        step->warns = *warns;
        *warns = NULL;
    }

    if (*errors != NULL)
    {
        str_destroy(&step->errors);
        step->errors = *errors;
        *errors = NULL;
    }

    step->nerrors = nerrors;
    step->nwarns = nwarns;
}

/*---------------------------------------------------------------------------*/

bool_t report_job_can_test(const Report *report, const uint32_t job_id)
{
    const RJob *job = NULL;
    const RStep *bstep = NULL;
    const RStep *tstep = NULL;
    uint32_t nsteps = 0;
    cassert_no_null(report);
    job = arrst_get_const(report->jobs, job_id, RJob);
    nsteps = arrst_size(job->steps, RStep);
    cassert(nsteps == 1 || nsteps == 2);
    /* No tests defined */
    if (nsteps == 1)
        return FALSE;

    bstep = arrst_get_const(job->steps, 0, RStep);
    tstep = arrst_get_const(job->steps, 1, RStep);

    /* Test is already done */
    if (i_is_done(&tstep->event) == TRUE)
        return FALSE;

    /* Build step is not done */
    if (i_is_done(&bstep->event) == FALSE)
        return FALSE;

    /* Build step fail */
    if (str_empty(bstep->event.error_msg) == FALSE)
        return FALSE;

    /* Build step with errors */
    if (bstep->nerrors > 0)
        return FALSE;

    return TRUE;
}

/*---------------------------------------------------------------------------*/

const char_t *report_job_host(const Report *report, const SJob *sjob)
{
    const RJob *rjob = NULL;
    cassert_no_null(report);
    cassert_no_null(sjob);
    rjob = arrst_get_const(report->jobs, sjob->id, RJob);
    cassert_no_null(rjob);
    return tc(rjob->hostname);
}

/*---------------------------------------------------------------------------*/

static bool_t i_block_jobs(const REvent *event, const uint32_t loop_id)
{
    cassert_no_null(event);
    cassert(event->loop_id == UINT32_MAX || event->loop_id <= loop_id);
    if (i_is_done(event) == FALSE)
        return TRUE;
    if (event->loop_id == loop_id)
        return TRUE;
    return FALSE;
}

/*---------------------------------------------------------------------------*/

bool_t report_can_start_jobs(const Report *report, const uint32_t doc_repo_vers)
{
    cassert_no_null(report);
    /*
     * Build jobs can start only if all events related with
     * 'sources' and 'documentation' were done if previous loops
     * */
    arrst_foreach_const(target, report->targets, RTarget)
        if (i_block_jobs(&target->event, report->loop_id) == TRUE)
            return FALSE;
    arrst_end()

    arrst_foreach_const(path, report->tests, RTarget)
        if (i_block_jobs(&path->event, report->loop_id) == TRUE)
            return FALSE;
    arrst_end()

    if (i_block_jobs(&report->build_file, report->loop_id) == TRUE)
        return FALSE;

    if (i_block_jobs(&report->src_tar, report->loop_id) == TRUE)
        return FALSE;

    if (doc_repo_vers != UINT32_MAX)
    {
        const RDoc *rdoc = i_get_doc(report->docs, doc_repo_vers);
        cassert_no_null(rdoc);
        if (i_block_jobs(&rdoc->event, report->loop_id) == TRUE)
            return FALSE;
    }

    return TRUE;
}

/*---------------------------------------------------------------------------*/

void report_force_jobs(Report *report, const char_t *job_pattern, const ArrSt(Job) *jobs, ArrSt(SJob) *seljobs, const bool_t with_tests)
{
    RegEx *regex = regex_create(job_pattern);
    cassert_no_null(report);
    arrst_clear(seljobs, NULL, SJob);

    /* Check that all jobs are in report */
    arrst_foreach_const(job, jobs, Job)
        i_add_job(report->jobs, tc(job->name), tc(job->generator), job->priority, with_tests);
    arrst_end()

    /* Select all jobs to be done. Doesn't matter if have be done in previous loops.  */
    arrst_foreach_const(job, jobs, Job)
        if (regex_match(regex, tc(job->name)) == TRUE)
        {
            uint32_t id = UINT32_MAX;
            SJob *sjob = arrst_new(seljobs, SJob);
            RJob *rjob = i_get_job(report->jobs, tc(job->name), &id);
            cassert_no_null(rjob);
            cassert(id == job_i);

            /* Clean a previous done timestamp */
            arrst_foreach(step, rjob->steps, RStep)
                i_event_init(&step->event, report);
            arrst_end()

            sjob->job = job;
            sjob->id = id;
        }
    arrst_end()

    regex_destroy(&regex);
}

/*---------------------------------------------------------------------------*/

void report_select_jobs(Report *report, const ArrSt(Job) *jobs, ArrSt(SJob) *seljobs, const bool_t with_tests)
{
    uint32_t i = 1, max_priority = 50;
    cassert_no_null(report);
    arrst_clear(seljobs, NULL, SJob);

    /* Check that all jobs are in report */
    arrst_foreach_const(job, jobs, Job)
        i_add_job(report->jobs, tc(job->name), tc(job->generator), job->priority, with_tests);
    arrst_end()

    /* Select all jobs to be done in this loop */
    for (i = 1; i <= max_priority; ++i)
    {
        arrst_foreach_const(job, jobs, Job)
            if (job->priority == i)
            {
                uint32_t id = UINT32_MAX;
                RJob *rjob = i_get_job_if_pending(report->jobs, tc(job->name), &id);
                /* We found a job with the higher priority which is not done */
                if (rjob != NULL)
                {
                    SJob *sjob = arrst_new(seljobs, SJob);
                    sjob->job = job;
                    sjob->id = id;
                }
            }
        arrst_end()

        /* We have pending jobs with the higher priority. Don't continue the search */
        if (arrst_size(seljobs, SJob) > 0)
            break;
    }
}

/*---------------------------------------------------------------------------*/

void report_log(const Report *report, const Global *global, const uint32_t repo_vers)
{
    uint32_t ne = 0, nw = 0;
    uint32_t seconds = report_loop_seconds(report, UINT32_MAX);
    log_printf("%s-%s%d%s", tc(global->project), kASCII_VERSION, repo_vers, kASCII_RESET);
    log_printf("Loop: %s%d%s (%d seconds)", kASCII_VERSION, report->loop_id, kASCII_RESET, seconds);

    if (nw > 0)
    {
        log_printf(" ");
        log_printf("%s There are WARNINGS!", kASCII_WARN);
    }

    if (ne > 0)
    {
        if (nw == 0)
            log_printf(" ");
        log_printf("%s There are ERRORS!", kASCII_FAIL);
    }
}

/*---------------------------------------------------------------------------*/

static void i_stm_date(Stream *stm, const Date *d)
{
    char_t date[64];
    cassert_no_null(d);
    blib_strftime(date, sizeof(date), "%Y %b %d", d->year, d->month, d->mday, d->wday, d->hour, d->minute, d->second);
    stm_writef(stm, date);
}

/*---------------------------------------------------------------------------*/

static void i_stm_datetime(Stream *stm, const Date *d, const bool_t paragraph)
{
    char_t date[64];
    cassert_no_null(d);
    blib_strftime(date, sizeof(date), "%b %d %H:%M:%S", d->year, d->month, d->mday, d->wday, d->hour, d->minute, d->second);
    if (paragraph == TRUE)
        stm_printf(stm, "p.%s\n", date);
    else
        stm_writef(stm, date);
}

/*---------------------------------------------------------------------------*/

static void i_stm_seconds(Stream *stm, const uint32_t seconds, const bool_t paragraph)
{
    uint32_t h = 0, m = 0, s = seconds;
    h = s / 3600;
    s = s % 3600;
    m = s / 60;
    s = s % 60;
    if (paragraph == TRUE)
        stm_printf(stm, "p.%02d:%02d:%02d\n", h, m, s);
    else
        stm_printf(stm, "%02d:%02d:%02d", h, m, s);
}

/*---------------------------------------------------------------------------*/

static uint32_t i_target_errors(const Report *report, const char_t *src_name)
{
    /* TODO: Compute the total errors of a target in all runners */
    unref(report);
    unref(src_name);
    return 0;
}

/*---------------------------------------------------------------------------*/

static uint32_t i_target_warnings(const Report *report, const char_t *src_name)
{
    /* TODO: Compute the total warnings of a target in all runners */
    unref(report);
    unref(src_name);
    return 0;
}

/*---------------------------------------------------------------------------*/

static void i_stm_mark(Stream *stm, const bool_t mark)
{
    if (mark == TRUE)
        stm_writef(stm, "p.✓\n");
    else
        stm_writef(stm, "p.⍉\n");
}

/*---------------------------------------------------------------------------*/

static uint32_t i_num_tasks_in_loop(const Report *report, const uint32_t loop_id)
{
    uint32_t n = 0;
    cassert_no_null(report);
    arrst_foreach_const(target, report->targets, RTarget)
        if (target->event.loop_id == loop_id)
            n += 1;
    arrst_end()

    arrst_foreach_const(target, report->tests, RTarget)
        if (target->event.loop_id == loop_id)
            n += 1;
    arrst_end()

    arrst_foreach_const(doc, report->docs, RDoc)
        if (doc->event.loop_id == loop_id)
            n += 1;
    arrst_end()

    arrst_foreach_const(job, report->jobs, RJob)
        const RStep *step = arrst_first_const(job->steps, RStep);
        cassert_no_null(step);
        if (step->event.loop_id == loop_id)
            n += 1;
    arrst_end()

    if (report->build_file.loop_id == loop_id)
        n += 1;

    if (report->src_tar.loop_id == loop_id)
        n += 1;

    return n;
}

/*---------------------------------------------------------------------------*/

const RDoc *i_rdoc_in_loop(const Report *report, const uint32_t loop_id)
{
    cassert_no_null(report);
    arrst_foreach_const(doc, report->docs, RDoc)
        if (doc->event.loop_id == loop_id)
            return doc;
    arrst_end()
    return NULL;
}

/*---------------------------------------------------------------------------*/

static void i_error_icon(Stream *stm, const bool_t show, const char_t *link_header, const uint32_t nerrors, const uint32_t nwarns)
{
    unref(link_header);
    if (show == TRUE)
    {
        if (nerrors > 0)
            stm_writef(stm, "img(error.png,16,.05).\n");
        else if (nwarns > 0)
            stm_writef(stm, "img(warning.png,16,.05).\n");
        else
            stm_writef(stm, "img(ok.png,16,.05).\n");
    }
    else
    {
        stm_writef(stm, "p.⍉\n");
    }
}

/*---------------------------------------------------------------------------*/

static bool_t i_job_done(const RJob *job)
{
    const RStep *step = NULL;
    cassert_no_null(job);
    step = arrst_first_const(job->steps, RStep);
    cassert_no_null(step);
    return i_is_done(&step->event);
}

/*---------------------------------------------------------------------------*/

static uint32_t i_jobs_done(const ArrSt(RJob) *jobs)
{
    uint32_t n = 0;
    arrst_foreach_const(job, jobs, RJob)
        if (i_job_done(job) == TRUE)
            n += 1;
    arrst_end()
    return n;
}

/*---------------------------------------------------------------------------*/

static bool_t i_icon_tag(const ArrPt(String) *tags, const char_t *icon)
{
    arrpt_foreach_const(tag, tags, String)
        if (str_str(tc(tag), icon) != NULL)
            return TRUE;
    arrpt_end()
    return FALSE;
}

/*---------------------------------------------------------------------------*/

static const char_t *i_job_icon(const RJob *rjob, const ArrSt(Job) *jobs)
{
    cassert_no_null(rjob);
    arrst_foreach_const(job, jobs, Job)
        if (str_equ(rjob->name, tc(job->name)) == TRUE)
        {
            if (i_icon_tag(job->tags, "ubuntu") == TRUE)
                return "ubuntu_logo.png";

            if (i_icon_tag(job->tags, "raspos") == TRUE)
                return "raspbian.png";

            if (str_equ(job->generator, "Visual Studio 17 2022") == TRUE || i_icon_tag(job->tags, "msvc2022") == TRUE)
                return "vs2022.png";

            if (str_equ(job->generator, "Visual Studio 16 2019") == TRUE || i_icon_tag(job->tags, "msvc2019") == TRUE)
                return "vs2019.png";

            if (str_equ(job->generator, "Visual Studio 15 2017") == TRUE || i_icon_tag(job->tags, "msvc2017") == TRUE)
                return "vs2017.png";

            if (str_equ(job->generator, "Visual Studio 14 2015") == TRUE || i_icon_tag(job->tags, "msvc2015") == TRUE)
                return "vs2015.png";

            if (str_equ(job->generator, "Visual Studio 12 2013") == TRUE || i_icon_tag(job->tags, "msvc2013") == TRUE)
                return "vs2013.png";

            if (str_equ(job->generator, "Visual Studio 11 2012") == TRUE || i_icon_tag(job->tags, "msvc2012") == TRUE)
                return "vs2012.png";

            if (str_equ(job->generator, "Visual Studio 10 2010") == TRUE || i_icon_tag(job->tags, "msvc2010") == TRUE)
                return "vs2010.png";

            if (str_equ(job->generator, "Visual Studio 9 2008") == TRUE)
                return "vs2008.png";

            if (str_equ(job->generator, "Visual Studio 8 2005") == TRUE)
                return "vs2005.png";

            if (str_equ(job->generator, "MinGW Makefiles") == TRUE)
                return "mingw.png";

            if (i_icon_tag(job->tags, "sequoia") == TRUE)
                return "sequoia.png";

            if (i_icon_tag(job->tags, "sonoma") == TRUE)
                return "sonoma.png";

            if (i_icon_tag(job->tags, "ventura") == TRUE)
                return "ventura.png";

            if (i_icon_tag(job->tags, "monterey") == TRUE)
                return "monterey.png";

            if (i_icon_tag(job->tags, "bigsur") == TRUE)
                return "bigsur.png";

            if (i_icon_tag(job->tags, "catalina") == TRUE)
                return "catalina.png";

            if (i_icon_tag(job->tags, "mojave") == TRUE)
                return "mojave.png";

            if (i_icon_tag(job->tags, "high_sierra") == TRUE)
                return "high_sierra.png";

            if (i_icon_tag(job->tags, "sierra") == TRUE)
                return "sierra.png";

            if (i_icon_tag(job->tags, "el_capitan") == TRUE)
                return "el_capitan.png";

            if (i_icon_tag(job->tags, "yosemite") == TRUE)
                return "yosemite.png";

            if (i_icon_tag(job->tags, "mavericks") == TRUE)
                return "mavericks.png";

            if (i_icon_tag(job->tags, "mountain_lion") == TRUE)
                return "mountain_lion.png";

            if (i_icon_tag(job->tags, "lion") == TRUE)
                return "lion.png";

            if (i_icon_tag(job->tags, "snow_leopard") == TRUE)
                return "snow_leopard.png";

            return "ubuntu20_logo.png";
        }

    arrst_end()
    cassert(FALSE);
    return NULL;
}

/*---------------------------------------------------------------------------*/

static const char_t *i_job_bgcolor(const uint32_t priority)
{
    switch (priority % 6)
    {
    case 0:
        return "#FFF5F4";
    case 1:
        return "#F2F6FF";
    case 2:
        return "#FEFFF4";
    case 3:
        return "#FCEEFF";
    case 4:
        return "#F3FFF4";
    case 5:
        return "#F2F6FF";
    }

    return "";
}

/*---------------------------------------------------------------------------*/

Stream *report_ndoc_page(const Report *report, const ArrSt(Job) *jobs, const Global *global, const char_t *project_vers)
{
    Stream *stm = stm_memory(1024);
    uint32_t nloops = UINT32_MAX;
    uint32_t njobs = UINT32_MAX;
    cassert_no_null(report);
    cassert_no_null(global);

    nloops = arrst_size(report->loops, RLoop);
    njobs = i_jobs_done(report->jobs);

    /* h1. Header */
    stm_printf(stm, "h1.r%d, v%s", report->repo_vers, project_vers);

    if (arrst_size(report->loops, RLoop) > 0)
    {
        const RLoop *loop = arrst_first_const(report->loops, RLoop);
        cassert_no_null(loop);
        stm_writef(stm, ", ");
        i_stm_date(stm, &loop->init);
        stm_printf(stm, " (%d)", njobs);
    }

    stm_writef(stm, "\n");
    stm_writef(stm, "notoc.\n");
    stm_writef(stm, "nosecnum.\n");
    stm_writef(stm, "\n");

    /* Link to nbuild doc */
    stm_printf(stm, "ep.Learn more about <b>nbuild</b> CI/CD <l>https://nappgui.com/en/nbuild/nbuild.html''here</l>\n");

    /* h2. Summary */
    stm_writef(stm, "h2.Summary\n");
    stm_writef(stm, "\n");

    /* Targets table */
    if (arrst_size(report->targets, RTarget) > 0)
    {
        stm_writef(stm, "table(targetssummary,,open,no).Source code package\n");
        stm_writef(stm, "row.\n");
        stm_writef(stm, "p.Target\n");
        stm_writef(stm, "p.Analyzer\n");
        stm_writef(stm, "p.Format\n");
        stm_writef(stm, "p.Legal\n");
        stm_writef(stm, "p.Time\n");
        stm_writef(stm, "p.Date\n");
        stm_writef(stm, "p.Loop\n");

        arrst_foreach_const(target, report->targets, RTarget)
            uint32_t nerrors = i_target_errors(report, tc(target->event.name));
            uint32_t nwarns = i_target_warnings(report, tc(target->event.name));
            const char_t *link_header = "";
            stm_writef(stm, "row.\n");
            stm_printf(stm, "p(left).%s\n", tc(target->event.name));
            i_error_icon(stm, target->analyzer, link_header, nerrors, nwarns);
            i_stm_mark(stm, target->format);
            i_stm_mark(stm, target->legal);
            stm_printf(stm, "p.%ds\n", target->event.seconds);
            i_stm_datetime(stm, &target->event.init, TRUE);
            stm_printf(stm, "p.<lh>Lp%d</lh>\n", target->event.loop_id);
        arrst_end()

        stm_writef(stm, "table.\n");
        stm_writef(stm, "\n");
    }

    /* Test table */
    if (arrst_size(report->tests, RTarget) > 0)
    {
        stm_writef(stm, "table(testssummary,,open,no).Test package\n");
        stm_writef(stm, "row.\n");
        stm_writef(stm, "p.Target\n");
        stm_writef(stm, "p.Analyzer\n");
        stm_writef(stm, "p.Format\n");
        stm_writef(stm, "p.Legal\n");
        stm_writef(stm, "p.Time\n");
        stm_writef(stm, "p.Date\n");
        stm_writef(stm, "p.Loop\n");

        arrst_foreach_const(test, report->tests, RTarget)
            uint32_t nerrors = i_target_errors(report, tc(test->event.name));
            uint32_t nwarns = i_target_warnings(report, tc(test->event.name));
            const char_t *link_header = "";
            stm_writef(stm, "row.\n");
            stm_printf(stm, "p(left).%s\n", tc(test->event.name));
            i_error_icon(stm, test->analyzer, link_header, nerrors, nwarns);
            i_stm_mark(stm, test->format);
            i_stm_mark(stm, test->legal);
            stm_printf(stm, "p.%ds\n", test->event.seconds);
            i_stm_datetime(stm, &test->event.init, TRUE);
            stm_printf(stm, "p.<lh>Lp%d</lh>\n", test->event.loop_id);
        arrst_end()

        stm_writef(stm, "table.\n");
        stm_writef(stm, "\n");
    }

    /* Documentation table */
    if (arrst_size(report->docs, RDoc) > 0)
    {
        stm_writef(stm, "table(docssummary,,open,no).Documentation\n");
        stm_writef(stm, "row.\n");
        stm_writef(stm, "p.\n");
        stm_writef(stm, "p.Url\n");
        stm_writef(stm, "p.Time\n");
        stm_writef(stm, "p.ndoc\n");
        stm_writef(stm, "p.en.pdf\n");
        stm_writef(stm, "p.es.pdf\n");
        stm_writef(stm, "p.Copy\n");
        stm_writef(stm, "p.Upload\n");
        stm_writef(stm, "p.Date\n");
        stm_writef(stm, "p.Loop\n");

        arrst_forback_const(doc, report->docs, RDoc)
            char_t link_header[32];
            bstd_sprintf(link_header, sizeof(link_header), "Lp%d", doc->event.loop_id);
            stm_writef(stm, "row.\n");
            i_error_icon(stm, TRUE, link_header, doc->nerrors, doc->nwarns);
            stm_printf(stm, "p.<l>%s/docs/r%d/en/home/web/home.html''r%d</l>\n", tc(global->doc_url), doc->doc_repo_vers, doc->doc_repo_vers);
            stm_printf(stm, "p.%ds\n", doc->event.seconds);
            stm_printf(stm, "p.%ds\n", doc->ndoc_event.seconds);
            stm_printf(stm, "p.%ds\n", doc->ebook_en_event.seconds);
            stm_printf(stm, "p.%ds\n", doc->ebook_es_event.seconds);
            stm_printf(stm, "p.%ds\n", doc->copy_event.seconds);
            stm_printf(stm, "p.%ds\n", doc->upload_event.seconds);
            i_stm_datetime(stm, &doc->event.init, TRUE);
            stm_printf(stm, "p.<lh>Lp%d</lh>\n", doc->event.loop_id);
        arrst_end()

        stm_writef(stm, "table.\n");
        stm_writef(stm, "\n");
    }

    /* Jobs table */
    if (njobs > 0)
    {
        stm_writef(stm, "table(jobssummary,,open,no).Build jobs\n");
        stm_writef(stm, "row.\n");
        stm_writef(stm, "p.\n");
        stm_writef(stm, "p.Name\n");
        stm_writef(stm, "p.Build\n");
        stm_writef(stm, "p.Test\n");
        stm_writef(stm, "p.Runner\n");
        stm_writef(stm, "p.Generator\n");
        stm_writef(stm, "p.T1\n");
        stm_writef(stm, "p.T2\n");
        stm_writef(stm, "p.Date\n");
        stm_writef(stm, "p.Loop\n");
        arrst_foreach_const(job, report->jobs, RJob)
            if (i_job_done(job) == TRUE)
            {
                const RStep *bstep = NULL;
                const RStep *tstep = NULL;
                const char_t *icon = i_job_icon(job, jobs);
                const char_t *link_header = tc(job->name);
                const char_t *bgcolor = i_job_bgcolor(job->priority);
                uint32_t nsteps = arrst_size(job->steps, RStep);
                cassert(nsteps == 1 || nsteps == 2);
                bstep = arrst_get_const(job->steps, 0, RStep);
                tstep = (nsteps == 2) ? arrst_get_const(job->steps, 1, RStep) : NULL;

                stm_printf(stm, "row(%s).\n", bgcolor);
                stm_printf(stm, "img(%s,16,.05).\n", icon);
                stm_printf(stm, "p.<lh>%s</lh>\n", tc(job->name));

                /* Build errors  */
                if (str_empty(bstep->event.error_msg) == TRUE)
                    i_error_icon(stm, TRUE, link_header, bstep->nerrors, bstep->nwarns);
                else
                    i_error_icon(stm, TRUE, link_header, 1, 0);

                /* Test errors  */
                if (tstep != NULL && i_is_done(&tstep->event) == TRUE)
                {
                    if (str_empty(tstep->event.error_msg) == TRUE)
                        i_error_icon(stm, TRUE, link_header, tstep->nerrors, tstep->nwarns);
                    else
                        i_error_icon(stm, TRUE, link_header, 1, 0);
                }
                else
                {
                    stm_writef(stm, "p.⍉\n");
                }

                stm_printf(stm, "p.%s\n", tc(job->hostname));
                stm_printf(stm, "p.%s\n", tc(job->generator));
                stm_printf(stm, "p.%ds\n", bstep->event.seconds);

                if (tstep != NULL && i_is_done(&tstep->event) == TRUE)
                    stm_printf(stm, "p.%ds\n", tstep->event.seconds);
                else
                    stm_writef(stm, "p.⍉\n");

                i_stm_datetime(stm, &bstep->event.init, TRUE);
                stm_printf(stm, "p.<lh>Lp%d</lh>\n", bstep->event.loop_id);
            }
        arrst_end()
        stm_writef(stm, "table.\n");
        stm_writef(stm, "\n");
    }

    /* Loops table */
    if (nloops > 0)
    {
        stm_printf(stm, "table(loops,,open,no).<b>%d</b> loops executed over '%d'\n", nloops, report->repo_vers);
        stm_writef(stm, "row.\n");
        stm_writef(stm, "p.Loop ID\n");
        stm_writef(stm, "p.Time\n");
        stm_writef(stm, "p.Begin\n");
        stm_writef(stm, "p.End\n");
        stm_writef(stm, "p.Tasks\n");

        arrst_forback_const(loop, report->loops, RLoop)
            int32_t seconds = (int32_t)date_ellapsed_seconds(&loop->init, &loop->end);
            uint32_t ntasks = i_num_tasks_in_loop(report, loop_i);
            cassert(seconds >= 0);
            if (ntasks > 0 || loop_i == loop_total - 1)
            {
                stm_writef(stm, "row.\n");
                stm_printf(stm, "p.<lh>Lp%d</lh>\n", loop_i);
                i_stm_seconds(stm, (uint32_t)seconds, TRUE);
                i_stm_datetime(stm, &loop->init, TRUE);
                i_stm_datetime(stm, &loop->end, TRUE);
                stm_printf(stm, "p.%d\n", ntasks);
            }
        arrst_end()

        stm_writef(stm, "table.\n");
        stm_writef(stm, "\n");
    }
    else
    {
        stm_writef(stm, "p.<b>No loops in this build.</b>\n");
    }

    /* Jobs details */
    arrst_foreach_const(job, report->jobs, RJob)
        if (i_job_done(job) == TRUE)
        {
            const RStep *bstep = NULL;
            const RStep *tstep = NULL;
            uint32_t nsteps = arrst_size(job->steps, RStep);
            cassert(nsteps == 1 || nsteps == 2);
            bstep = arrst_get_const(job->steps, 0, RStep);
            tstep = (nsteps == 2) ? arrst_get_const(job->steps, 1, RStep) : NULL;

            stm_printf(stm, "h2.%s\n", tc(job->name));

            /* Execution errors */
            if (str_empty(bstep->event.error_msg) == FALSE)
            {
                stm_printf(stm, "code(text,,1,open).Job execution error\n");
                stm_writef(stm, tc(bstep->event.error_msg));
                stm_writef(stm, "\n");
                stm_writef(stm, "code.\n");
            }

            /* Build errors and warnings */
            if (bstep->nerrors > 0)
            {
                stm_printf(stm, "code(text,,1,open).Build <b>%d</b> errors\n", bstep->nerrors);
                stm_writef(stm, tc(bstep->errors));
                stm_writef(stm, "code.\n");
            }

            if (bstep->nwarns > 0)
            {
                stm_printf(stm, "code(text,,1,open).Build <b>%d</b> warnings\n", bstep->nwarns);
                stm_writef(stm, tc(bstep->warns));
                stm_writef(stm, "code.\n");
            }

            /* Test errors and warnings */
            if (tstep != NULL)
            {
                if (str_empty(tstep->event.error_msg) == FALSE)
                {
                    stm_printf(stm, "code(text,,1,open).Test job execution error\n");
                    stm_writef(stm, tc(tstep->event.error_msg));
                    stm_writef(stm, "\n");
                    stm_writef(stm, "code.\n");
                }

                if (tstep->nerrors > 0)
                {
                    Buffer *buffer = b64_decode_from_str(tstep->errors);
                    stm_printf(stm, "code(ansi,,1,open).Test <b>%d</b> errors\n", tstep->nerrors);
                    stm_write(stm, buffer_const(buffer), buffer_size(buffer));
                    stm_writef(stm, "code.\n");
                    buffer_destroy(&buffer);
                }

                if (tstep->nwarns > 0)
                {
                    Buffer *buffer = b64_decode_from_str(tstep->warns);
                    stm_printf(stm, "code(ansi,,1,open).Test <b>%d</b> warnings\n", tstep->nwarns);
                    stm_write(stm, buffer_const(buffer), buffer_size(buffer));
                    stm_writef(stm, "code.\n");
                    buffer_destroy(&buffer);
                }
            }

            /* Build logs */
            if (str_empty(bstep->cmake_log) == FALSE)
            {
                stm_writef(stm, "code(text,,1,close).Build cmake log\n");
                stm_writef(stm, tc(bstep->cmake_log));
                stm_writef(stm, "code.\n");
            }

            if (str_empty(bstep->build_log) == FALSE)
            {
                stm_writef(stm, "code(text,,1,close).Build log\n");
                stm_writef(stm, tc(bstep->build_log));
                stm_writef(stm, "code.\n");
            }

            if (str_empty(bstep->install_log) == FALSE)
            {
                stm_writef(stm, "code(text,,1,close).Install log\n");
                stm_writef(stm, tc(bstep->install_log));
                stm_writef(stm, "code.\n");
            }

            if (tstep != NULL)
            {
                if (str_empty(tstep->cmake_log) == FALSE)
                {
                    stm_writef(stm, "code(text,,1,close).Test cmake log\n");
                    stm_writef(stm, tc(tstep->cmake_log));
                    stm_writef(stm, "code.\n");
                }

                if (str_empty(tstep->build_log) == FALSE)
                {
                    stm_writef(stm, "code(text,,1,close).Test build log\n");
                    stm_writef(stm, tc(tstep->build_log));
                    stm_writef(stm, "code.\n");
                }

                if (str_empty(tstep->install_log) == FALSE)
                {
                    Buffer *buffer = b64_decode_from_str(tstep->install_log);
                    stm_writef(stm, "code(ansi,,1,close).Test run log\n");
                    stm_write(stm, buffer_const(buffer), buffer_size(buffer));
                    stm_writef(stm, "code.\n");
                    buffer_destroy(&buffer);
                }
            }
        }
    arrst_end()

    /* Loop details */
    arrst_foreach_const(loop, report->loops, RLoop)
        uint32_t ntasks = i_num_tasks_in_loop(report, loop_i);
        if (ntasks > 0 || loop_i == loop_total - 1)
        {
            const RDoc *rdoc = i_rdoc_in_loop(report, loop_i);
            stm_printf(stm, "h2.Lp%d\n", loop_i);

            if (rdoc != NULL)
            {
                if (rdoc->nerrors > 0)
                {
                    Buffer *buffer = b64_decode_from_str(rdoc->errors);
                    const byte_t *data = buffer_const(buffer);
                    uint32_t size = buffer_size(buffer);
                    stm_printf(stm, "code(ansi,,1,open).Documentation '%d' errors\n", rdoc->doc_repo_vers);
                    stm_write(stm, data, size);
                    stm_writef(stm, "code.\n");
                    buffer_destroy(&buffer);
                }

                if (rdoc->nwarns > 0)
                {
                    Buffer *buffer = b64_decode_from_str(rdoc->warns);
                    const byte_t *data = buffer_const(buffer);
                    uint32_t size = buffer_size(buffer);
                    stm_printf(stm, "code(ansi,,1,open).Documentation '%d' warnings\n", rdoc->doc_repo_vers);
                    stm_write(stm, data, size);
                    stm_writef(stm, "code.\n");
                    buffer_destroy(&buffer);
                }
            }

            /* Loop log */
            if (str_empty(loop->log) == FALSE)
            {
                const char_t *state = ntasks > 0 ? "close" : "open";
                Buffer *buffer = b64_decode_from_str(loop->log);
                const byte_t *data = buffer_const(buffer);
                uint32_t size = buffer_size(buffer);
                int32_t seconds = (int32_t)date_ellapsed_seconds(&loop->init, &loop->end);
                stm_printf(stm, "code(ansi,,1,%s).", state);

                /* Loop summary line */
                cassert(seconds >= 0);
                stm_printf(stm, "%d tasks completed in %d sec on ", ntasks, seconds);
                i_stm_datetime(stm, &loop->init, FALSE);
                stm_writef(stm, "\n");
                stm_write(stm, data, size);
                stm_writef(stm, "code.\n");
                buffer_destroy(&buffer);
            }

            stm_writef(stm, "\n");
        }
    arrst_end()

    return stm;
}

/*---------------------------------------------------------------------------*/

void report_state_log(const RState *state, const char_t *msg)
{
    char_t date[128];
    const Date *d;
    cassert_no_null(state);
    d = &state->date;

    if (date_is_null(d) == FALSE)
        blib_strftime(date, sizeof(date), "%y %b %d %H:%M:%S", d->year, d->month, d->mday, d->wday, d->hour, d->minute, d->second);
    else
        str_copy_c(date, sizeof(date), "No date");

    if (state->done == TRUE)
    {
        if (str_empty_c(state->error_msg) == TRUE)
            log_printf("%s %s. %s (%d seconds) Loop: %s%d%s", kASCII_OK, msg, date, state->seconds, kASCII_VERSION, state->loop_id, kASCII_RESET);
        else
            log_printf("%s %s. %s '%s' Loop: %d", kASCII_FAIL, msg, date, state->error_msg, state->loop_id);
    }
}
