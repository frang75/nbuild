/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: nbuild.hxx
 *
 */

/* NAppGUI Build */

#ifndef __NBUILD_HXX__
#define __NBUILD_HXX__

#include <nlib/nlib.hxx>

#define CMAKE_MIN_VERSION "2.8.12"

typedef enum _macos_t
{
    ekMACOS_UNKNOWN = 0,
    ekMACOS_LEOPARD,
    ekMACOS_SNOW_LEOPARD,
    ekMACOS_LION,
    ekMACOS_MOUNTAIN_LION,
    ekMACOS_MAVERICKS,
    ekMACOS_YOSEMITE,
    ekMACOS_EL_CAPITAN,
    ekMACOS_SIERRA,
    ekMACOS_HIGH_SIERRA,
    ekMACOS_MOJAVE,
    ekMACOS_CATALINA,
    ekMACOS_BIG_SUR,
    ekMACOS_MONTEREY,
    ekMACOS_VENTURA,
    ekMACOS_SONOMA,
    ekMACOS_SEQUOIA
} macos_t;

typedef enum _runstate_t
{
    ekRUNSTATE_NOT_INIT,
    ekRUNSTATE_ALREADY_RUNNING,
    ekRUNSTATE_VBOX_HOST_DOWN,
    ekRUNSTATE_VBOX_HOST_SSH,
    ekRUNSTATE_VBOX_HOST_VBOXMANAGE,
    ekRUNSTATE_VBOX_WAKE_UP,
    ekRUNSTATE_VBOX_TIMEOUT,
    ekRUNSTATE_UTM_HOST_DOWN,
    ekRUNSTATE_UTM_HOST_SSH,
    ekRUNSTATE_UTM_HOST_UTMCTL,
    ekRUNSTATE_UTM_WAKE_UP,
    ekRUNSTATE_UTM_TIMEOUT,
    ekRUNSTATE_VMWARE_HOST_DOWN,
    ekRUNSTATE_VMWARE_HOST_SSH,
    ekRUNSTATE_VMWARE_HOST_VMRUN,
    ekRUNSTATE_VMWARE_WAKE_UP,
    ekRUNSTATE_VMWARE_TIMEOUT,
    ekRUNSTATE_MACOS_UNKNOWN_VERSION,
    ekRUNSTATE_MACOS_NOT_BOOTABLE,
    ekRUNSTATE_MACOS_WAKE_UP,
    ekRUNSTATE_MACOS_CANT_BOOT_FROM_VOLUME,
    ekRUNSTATE_MACOS_TIMEOUT,
    ekRUNSTATE_UNREACHABLE
} runstate_t;

typedef struct _workpaths_t WorkPaths;
typedef struct _report_t Report;
typedef struct _revent_t REvent;
typedef struct _rstate_t RState;
typedef struct _workflow_t Workflow;
typedef struct _workflows_t Workflows;
typedef struct _global_t Global;
typedef struct _drive_t Drive;
typedef struct _target_t Target;
typedef struct _job_t Job;
typedef struct _host_t Host;
typedef struct _network_t Network;
typedef struct _sjob_t SJob;

/* Full set of directories that nbuild will work with during its execution. */
struct _workpaths_t
{
    String *tmp_path; /* Main temporal path in master node 'nbuild_master_tmp/flowid' */
    String *tmp_src;  /* Temporal source code processing 'nbuild_master_tmp/flowid/src' */
    String *tmp_test; /* Temporal tests code processing 'nbuild_master_tmp/flowid/test' */
    String *tmp_ndoc; /* Temporal ndoc generator files 'nbuild_master_tmp/flowid/ndoc_out' */
    String *tmp_nrep; /* Temporal ndoc web report files 'nbuild_master_tmp/flowid/ndoc_rep' */

    String *drive_path;    /* Main path storage in drive 'drive/flowid/repo_vers' */
    String *drive_inf;     /* drive reports and logs 'drive/flowid/repo_vers/inf' */
    String *drive_doc;     /* drive documentation 'drive/flowid-DOC/doc_repo_vers' */
    String *drive_rep;     /* drive reports sources 'drive/flowid-REP' */
    String *drive_rep_web; /* drive reports websites 'drive/flowid-REPWEB/repo_vers' */
};

struct _rstate_t
{
    bool_t done;
    uint32_t loop_id;
    Date date;
    int32_t seconds;
    const char_t *error_msg;
};

struct _drive_t
{
    String *name;
    String *path;
    Login login;
};

struct _network_t
{
    Drive drive;
    ArrSt(Host) *hosts;
};

struct _global_t
{
    String *project;
    String *description;
    uint32_t start_year;
    String *author;
    ArrPt(String) *license;
    String *flowid;
    String *repo_url;
    String *repo_branch;
    String *repo_user;
    String *repo_pass;
    String *doc_repo_url;
    String *doc_repo_user;
    String *doc_repo_pass;
    String *doc_url;
    String *web_report_repo_url;
    String *web_report_repo_user;
    String *web_report_repo_pass;
    String *hosting_url;
    String *hosting_user;
    String *hosting_pass;
    bool_t hosting_cert;
    String *hosting_docpath;
    String *hosting_buildpath;
};

struct _target_t
{
    String *name;
    String *dest;
    String *url;
    String *exec;
    bool_t legal;
    bool_t format;
    bool_t analyzer;
    uint32_t repo_vers;
};

struct _job_t
{
    uint32_t id;
    uint32_t priority;
    String *name;
    String *config;
    String *generator;
    String *opts;
    ArrPt(String) *tags;
};

struct _sjob_t
{
    const Job *job;
    uint32_t id;
};

DeclSt(Target);
DeclSt(Job);
DeclSt(SJob);
ArrStFuncs(Host);

#endif
