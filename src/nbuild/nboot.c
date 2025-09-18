/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: nboot.c
 *
 */

/* nbuild booter */

#include "nboot.h"
#include "host.h"
#include <nlib/nlib.h>
#include <nlib/ssh.h>
#include <core/strings.h>
#include <core/stream.h>
#include <osbs/btime.h>
#include <osbs/bthread.h>
#include <osbs/log.h>
#include <sewer/cassert.h>

/*---------------------------------------------------------------------------*/

static bool_t i_boot_metal(runstate_t *state)
{
    cassert_no_null(state);
    /* We cannot boot a 'metal' host that is power off */
    *state = ekRUNSTATE_UNREACHABLE;
    return FALSE;
}

/*---------------------------------------------------------------------------*/

static bool_t i_ping_with_timeout(const Login *login, const uint32_t timeout_seconds)
{
    uint64_t st = btime_now() / 1000000;
    for (;;)
    {
        if (ssh_ping(tc(login->ip)) == TRUE)
        {
            /* Win7 runners receive ping before they are ready, improve */
            if (login->platform == ekWINDOWS)
                bthread_sleep(15 * 1000);

            return TRUE;
        }

        {
            uint64_t now = btime_now() / 1000000;
            if (now - st >= timeout_seconds)
                return FALSE;
        }

        bthread_sleep(500);
    }

    /* return FALSE;*/
}

/*---------------------------------------------------------------------------*/

static bool_t i_boot_vbox(const Host *host, const ArrSt(Host) *hosts, runstate_t *state)
{
    const char_t *name = host_name(host);
    const char_t *vbox_uuid = host_vbox_uuid(host);
    const char_t *vbox_host = host_vbox_host(host);
    const Host *phost = host_by_name(hosts, vbox_host);
    cassert_no_null(state);
    if (phost != NULL)
    {
        const Login *login = host_login(host);
        const Login *plogin = host_login(phost);
        bool_t ok = FALSE;
        log_printf("%s Booting '%s%s%s'-'%s%s%s' from '%s%s%s'-'%s%s%s'", kASCII_BOOT, kASCII_PATH, name, kASCII_RESET, kASCII_TARGET, tc(login->ip), kASCII_RESET, kASCII_PATH, vbox_host, kASCII_RESET, kASCII_TARGET, tc(plogin->ip), kASCII_RESET);
        ok = ssh_vbox_start(plogin, vbox_uuid);
        if (ok == TRUE)
        {
            ok = i_ping_with_timeout(login, 60 * 5);
            if (ok == TRUE)
            {
                *state = ekRUNSTATE_VBOX_WAKE_UP;
                return TRUE;
            }
            else
            {
                *state = ekRUNSTATE_VBOX_TIMEOUT;
                return FALSE;
            }
        }
        else
        {
            if (ssh_ping(tc(plogin->ip)) == TRUE)
            {
                if (ssh_vbox_check(plogin) == TRUE)
                    *state = ekRUNSTATE_VBOX_HOST_VBOXMANAGE;
                else
                    *state = ekRUNSTATE_VBOX_HOST_SSH;
            }
            else
            {
                *state = ekRUNSTATE_VBOX_HOST_DOWN;
            }
            return FALSE;
        }
    }
    else
    {
        log_printf("%s Host '%s' referenced by '%s' doesn't exists in network", kASCII_FAIL, vbox_host, name);
        *state = ekRUNSTATE_UNREACHABLE;
        return FALSE;
    }
}

/*---------------------------------------------------------------------------*/

/*
 * UTM 4.6.4 (latest) doesn't support remote 'utmctl' calls via SSH (recheck in future utm releases).
 * We use a launchd script to wake up a UTM virtual machine.
 */
static Stream *i_utm_launchd_script(const char_t *utm_uuid)
{
    Stream *stm = stm_memory(1024);
    stm_writef(stm, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    stm_writef(stm, "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n");
    stm_writef(stm, "<plist version=\"1.0\">\n");
    stm_writef(stm, "<dict>\n");
    stm_writef(stm, "    <key>Label</key>\n");
    stm_writef(stm, "    <string>com.example.onetimejob</string>\n");
    stm_writef(stm, "    \n");
    stm_writef(stm, "    <key>ProgramArguments</key>\n");
    stm_writef(stm, "    <array>\n");
    stm_writef(stm, "        <string>/Applications/UTM.app/Contents/MacOS/utmctl</string>\n");
    stm_writef(stm, "        <string>start</string>\n");
    stm_printf(stm, "        <string>%s</string>\n", utm_uuid);
    stm_writef(stm, "    </array>\n");
    stm_writef(stm, "    \n");
    stm_writef(stm, "    <key>StartInterval</key>\n");
    stm_writef(stm, "    <integer>1</integer>\n");
    stm_writef(stm, "    \n");
    stm_writef(stm, "    <key>RunAtLoad</key>\n");
    stm_writef(stm, "    <true/>\n");
    stm_writef(stm, "    \n");
    stm_writef(stm, "    <key>AbandonProcessGroup</key>\n");
    stm_writef(stm, "    <true/>\n");
    stm_writef(stm, "</dict>\n");
    stm_writef(stm, "</plist>\n");
    return stm;
}

/*---------------------------------------------------------------------------*/

static bool_t i_utm_start(const Login *plogin, const char_t *utm_host, const char_t *utm_uuid)
{
    bool_t ok = TRUE;
    const char_t *script_path = "/tmp";
    const char_t *script_name = "utm_launch.plist";
    Stream *stm = NULL;
    cassert_no_null(plogin);

    if (plogin->platform != ekMACOS)
    {
        log_printf("%s UTM virtual machines ONLY supported in macOS hosts", kASCII_BOOT_FAIL);
        ok = FALSE;
    }

    if (ok == TRUE)
    {
        stm = i_utm_launchd_script(utm_uuid);
        if (stm == NULL)
        {
            log_printf("%s Error creating launchd UTM script", kASCII_BOOT_FAIL);
            ok = FALSE;
        }
    }

    if (ok == TRUE)
    {
        if (ssh_to_file(plogin, script_path, script_name, stm) == FALSE)
        {
            log_printf("%s Error copying '%s' into '%s%s::%s%s'", kASCII_BOOT_FAIL, script_name, kASCII_TARGET, utm_host, tc(plogin->ip), kASCII_RESET);
            ok = FALSE;
        }
    }

    /*
     * We load a script to launchd to start the virtual machine, and instantly unload the script
     * to prevent the same machine from being launched again.
     */
    if (ok == TRUE)
    {
        String *pathname = str_path(plogin->platform, "%s/%s", script_path, script_name);
        ssh_launchd_load(plogin, tc(pathname));
        bthread_sleep(2000);
        ssh_launchd_unload(plogin, tc(pathname));
        str_destroy(&pathname);
    }

    if (stm != NULL)
        stm_close(&stm);
    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_boot_utm(const Host *host, const ArrSt(Host) *hosts, runstate_t *state)
{
    const char_t *name = host_name(host);
    const char_t *utm_uuid = host_utm_uuid(host);
    const char_t *utm_host = host_utm_host(host);
    const Host *phost = host_by_name(hosts, utm_host);
    cassert_no_null(state);
    if (phost != NULL)
    {
        const Login *login = host_login(host);
        const Login *plogin = host_login(phost);
        bool_t ok = FALSE;
        log_printf("%s Booting '%s%s%s'-'%s%s%s' from '%s%s%s'-'%s%s%s'", kASCII_BOOT, kASCII_PATH, name, kASCII_RESET, kASCII_TARGET, tc(login->ip), kASCII_RESET, kASCII_PATH, utm_host, kASCII_RESET, kASCII_TARGET, tc(plogin->ip), kASCII_RESET);
        ok = i_utm_start(plogin, utm_host, utm_uuid);
        if (ok == TRUE)
        {
            ok = i_ping_with_timeout(login, 60 * 5);
            if (ok == TRUE)
            {
                *state = ekRUNSTATE_UTM_WAKE_UP;
                return TRUE;
            }
            else
            {
                *state = ekRUNSTATE_UTM_TIMEOUT;
                return FALSE;
            }
        }
        else
        {
            if (ssh_ping(tc(plogin->ip)) == TRUE)
            {
                /*
                if (ssh_vbox_check(plogin) == TRUE)
                    *state = ekRUNSTATE_UTM_HOST_UTMCTL;
                else*/
                *state = ekRUNSTATE_UTM_HOST_SSH;
            }
            else
            {
                *state = ekRUNSTATE_UTM_HOST_DOWN;
            }
            return FALSE;
        }
    }
    else
    {
        log_printf("%s Host '%s' referenced by '%s' doesn't exists in network", kASCII_FAIL, utm_host, name);
        *state = ekRUNSTATE_UNREACHABLE;
        return FALSE;
    }
}

/*---------------------------------------------------------------------------*/

static bool_t i_vmware_start(const Login *plogin, const char_t *vmware_path)
{
    bool_t ok = TRUE;
    cassert_no_null(plogin);

    if (plogin->platform != ekMACOS)
    {
        log_printf("%s VMware virtual machines ONLY supported in macOS hosts", kASCII_BOOT_FAIL);
        ok = FALSE;
    }

    if (ok == TRUE)
        ok = ssh_vmware_start(plogin, vmware_path);

    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_boot_vmware(const Host *host, const ArrSt(Host) *hosts, runstate_t *state)
{
    const char_t *name = host_name(host);
    const char_t *vmware_path = host_vmware_path(host);
    const char_t *vmware_host = host_vmware_host(host);
    const Host *phost = host_by_name(hosts, vmware_host);
    cassert_no_null(state);
    if (phost != NULL)
    {
        const Login *login = host_login(host);
        const Login *plogin = host_login(phost);
        bool_t ok = FALSE;
        log_printf("%s Booting '%s%s%s'-'%s%s%s' from '%s%s%s'-'%s%s%s'", kASCII_BOOT, kASCII_PATH, name, kASCII_RESET, kASCII_TARGET, tc(login->ip), kASCII_RESET, kASCII_PATH, vmware_host, kASCII_RESET, kASCII_TARGET, tc(plogin->ip), kASCII_RESET);
        ok = i_vmware_start(plogin, vmware_path);
        if (ok == TRUE)
        {
            ok = i_ping_with_timeout(login, 60 * 5);
            if (ok == TRUE)
            {
                *state = ekRUNSTATE_VMWARE_WAKE_UP;
                return TRUE;
            }
            else
            {
                *state = ekRUNSTATE_VMWARE_TIMEOUT;
                return FALSE;
            }
        }
        else
        {
            if (ssh_ping(tc(plogin->ip)) == TRUE)
            {
                /*
                if (ssh_vbox_check(plogin) == TRUE)
                    *state = ekRUNSTATE_UTM_HOST_UTMCTL;
                else*/
                *state = ekRUNSTATE_VMWARE_HOST_SSH;
            }
            else
            {
                *state = ekRUNSTATE_VMWARE_HOST_DOWN;
            }
            return FALSE;
        }
    }
    else
    {
        log_printf("%s Host '%s' referenced by '%s' doesn't exists in network", kASCII_FAIL, vmware_host, name);
        *state = ekRUNSTATE_UNREACHABLE;
        return FALSE;
    }
}

/*---------------------------------------------------------------------------*/

static bool_t i_macos_can_boot_direct(const macos_t from_macOS, const macos_t to_macos)
{
    /*
     * Bigsur and higher can't be booted from Catalina and lower
     * Because Apple changes in file system
     */

    /* BigSur and higher can boot any macOS volume */
    if (from_macOS >= ekMACOS_BIGSUR)
        return TRUE;

    /* Catalina and lower can be booted from any macOS volume */
    if (to_macos <= ekMACOS_CATALINA)
        return TRUE;

    return FALSE;
}

/*---------------------------------------------------------------------------*/

static String *i_macos_disk_from_volume(const Login *login, const char_t *volume)
{
    Stream *stm = ssh_diskutil_list(login);
    String *disk = NULL;
    if (stm != NULL)
    {
        /*
         * Apple 'diskutil' can enclose the volume name between U+2068 and U+2069 codepoints, for bidirectional text control.
         * U+2068: First Strong Isolate (FSI). Used to mark the beginning of a range of text that should be treated as an "isolate".
         * U+2069: Pop Directional Isolate (PDI). Used to mark the end of an "isolated" range.
         *
         * These characters are not blanks and must be taken into account when looking for the volume token.
         */
        String *volume_alias1 = str_printf("/Volumes/%s", volume);
        String *volume_alias2 = str_printf("\xE2\x81\xA8%s\xE2\x81\xA9", volume);
        String *volume_alias3 = str_printf("\xE2\x81\xA8/Volumes/%s\xE2\x81\xA9", volume);
        bool_t found_volume = FALSE;
        uint32_t pos_disk = 0;
        const char_t *token = stm_read_trim(stm);
        while (!str_empty_c(token) && disk == NULL)
        {
            if (found_volume == TRUE)
            {
                pos_disk += 1;
                if (str_is_prefix(token, "disk") == TRUE)
                {
                    if (pos_disk == 3)
                    {
                        disk = str_c(token);
                    }
                    else
                    {
                        found_volume = FALSE;
                        pos_disk = 0;
                    }
                }
            }
            else
            {
                if (str_equ_c(token, volume) == TRUE || str_equ_c(token, tc(volume_alias1)) == TRUE || str_equ_c(token, tc(volume_alias2)) == TRUE || str_equ_c(token, tc(volume_alias3)) == TRUE)
                {
                    found_volume = TRUE;
                }
            }

            token = stm_read_trim(stm);
        }

        str_destroy(&volume_alias1);
        str_destroy(&volume_alias2);
        str_destroy(&volume_alias3);
        stm_close(&stm);
    }

    return disk;
}

/*---------------------------------------------------------------------------*/

/* Reboot a mac using another boot volume connected at same machine */
static bool_t i_boot_from_bless(const Host *from_host, const Host *to_host, runstate_t *state)
{
    bool_t ok = TRUE;
    /* The boot volume actually running */
    const Login *from_login = host_login(from_host);
    const char_t *from_name = host_name(from_host);
    /* The volume name we want to boot */
    const Login *to_login = host_login(to_host);
    const char_t *to_name = host_name(to_host);
    const char_t *volume = host_macos_volume(to_host);
    /* The device disk to mount */
    String *disk = NULL;
    /* The volume path to mount */
    String *volume_path = str_printf("/Volumes/%s", volume);

    cassert_no_null(state);
    log_printf("%s Booting '%s%s%s'-'%s%s%s' from '%s%s%s'-'%s%s%s'", kASCII_BOOT, kASCII_PATH, to_name, kASCII_RESET, kASCII_TARGET, tc(to_login->ip), kASCII_RESET, kASCII_PATH, from_name, kASCII_RESET, kASCII_TARGET, tc(from_login->ip), kASCII_RESET);

    /* We use 'diskutil list' to get the device associated with the volume */
    if (ok == TRUE)
    {
        disk = i_macos_disk_from_volume(from_login, volume);
        if (disk == NULL)
            ok = FALSE;

        if (ok == FALSE)
            log_printf("%s macOS disk from volume", kASCII_FAIL);
    }

    /*
     * We use 'diskutil mount' to mount the device.
     * If we use external (usb) boot devices, they are mounted when we open a graphic session.
     * But, normally, they are not present in a ssh session.
     * We have to explicitly mount it.
     */
    if (ok == TRUE)
    {
        String *device_path = str_printf("/dev/%s", tc(disk));
        ok = ssh_mount(from_login, tc(device_path), tc(volume_path));
        str_destroy(&device_path);
        if (ok == FALSE)
            log_printf("%s macOS mount", kASCII_FAIL);
    }

    /*
     * We change the boot volume using
     * sudo bless --mount /Volumes/VOLUME_NAME --setBoot --nextonly
     */
    if (ok == TRUE)
    {
        ok = ssh_bless(from_login, tc(volume_path));
        if (ok == FALSE)
            log_printf("%s macOS bless", kASCII_FAIL);
    }

    /*
     * And just reboot
     */
    if (ok == TRUE)
    {
        ssh_reboot(from_login);
        if (ok == FALSE)
            log_printf("%s macOS reboot", kASCII_FAIL);
    }

    str_destopt(&disk);
    str_destroy(&volume_path);

    /*
     * And now we wait for the new boot partition.
     * In iMacs it can take about two minutes
     */
    if (ok == TRUE)
    {
        if (i_ping_with_timeout(to_login, 60 * 5) == TRUE)
        {
            *state = ekRUNSTATE_MACOS_WAKE_UP;
            return TRUE;
        }
        else
        {
            *state = ekRUNSTATE_MACOS_TIMEOUT;
            return FALSE;
        }
    }
    else
    {
        *state = ekRUNSTATE_MACOS_CANT_BOOT_FROM_VOLUME;
        return FALSE;
    }
}

/*---------------------------------------------------------------------------*/

static bool_t i_boot_macos(const Host *host, const ArrSt(Host) *hosts, runstate_t *state)
{
    /* const char_t *name = host_name(host);*/
    const char_t *macos_host = host_macos_host(host);
    /* Look for some running macOS volume in a Mac computer */
    const Host *alive_host = host_macos_alive(hosts, macos_host);

    if (alive_host != NULL)
    {
        macos_t alive_os = host_macos_version(alive_host);
        macos_t host_os = host_macos_version(host);
        if (alive_os != ekMACOS_NO && host_os != ekMACOS_NO)
        {
            if (i_macos_can_boot_direct(alive_os, host_os) == TRUE)
            {
                return i_boot_from_bless(alive_host, host, state);
            }
            else
            {
                /* TODO: Try to restore to default volume */
                *state = ekRUNSTATE_MACOS_NOT_BOOTABLE;
                return FALSE;
            }
        }
        else
        {
            *state = ekRUNSTATE_MACOS_UNKNOWN_VERSION;
            return FALSE;
        }
    }
    else
    {
        *state = ekRUNSTATE_UNREACHABLE;
        return FALSE;
    }
}

/*---------------------------------------------------------------------------*/

bool_t nboot_boot(const Host *host, const ArrSt(Host) *hosts, runstate_t *state)
{
    const Login *login = host_login(host);
    /* const char_t *name = host_name(host);*/
    const char_t *type = host_type(host);
    cassert_no_null(state);
    cassert_no_null(login);
    if (ssh_ping(tc(login->ip)) == TRUE)
    {
        *state = ekRUNSTATE_ALREADY_RUNNING;
        return TRUE;
    }

    /* The host is not accesible. We can try boot it */
    if (str_equ_c(type, "metal") == TRUE)
    {
        return i_boot_metal(state);
    }
    /* The host is a VirtualBox machine */
    else if (str_equ_c(type, "vbox") == TRUE)
    {
        return i_boot_vbox(host, hosts, state);
    }
    /* The host is a UTM virtual machine */
    else if (str_equ_c(type, "utm") == TRUE)
    {
        return i_boot_utm(host, hosts, state);
    }
    /* The host is a VMware virtual machine */
    else if (str_equ_c(type, "vmware") == TRUE)
    {
        return i_boot_vmware(host, hosts, state);
    }
    /* The host is a macOS volume */
    else if (str_equ_c(type, "macos") == TRUE)
    {
        return i_boot_macos(host, hosts, state);
    }

    return FALSE;
}

/*---------------------------------------------------------------------------*/

bool_t nboot_shutdown(const Host *host, const ArrSt(Host) *hosts, const runstate_t state)
{
    unref(hosts);
    switch (state)
    {
    case ekRUNSTATE_NOT_INIT:
        return FALSE;

    /* If host was running before 'nboot' we don't shutdown the host */
    case ekRUNSTATE_ALREADY_RUNNING:
        return FALSE;

    /* VBOX Host unreachable */
    case ekRUNSTATE_VBOX_HOST_DOWN:
    case ekRUNSTATE_VBOX_HOST_SSH:
    case ekRUNSTATE_VBOX_HOST_VBOXMANAGE:
        return FALSE;

    /* If VBox was booting by nboot, we shutdown it */
    case ekRUNSTATE_VBOX_WAKE_UP:
    case ekRUNSTATE_VBOX_TIMEOUT:
    {
        const Login *login = host_login(host);
        return ssh_shutdown(login);
    }

    /* UTM Host unreachable */
    case ekRUNSTATE_UTM_HOST_DOWN:
    case ekRUNSTATE_UTM_HOST_SSH:
    case ekRUNSTATE_UTM_HOST_UTMCTL:
        return FALSE;

    /* If UTM was booting by nboot, we shutdown it */
    case ekRUNSTATE_UTM_WAKE_UP:
    case ekRUNSTATE_UTM_TIMEOUT:
    {
        const Login *login = host_login(host);
        return ssh_shutdown(login);
    }

    /* VMware Host unreachable */
    case ekRUNSTATE_VMWARE_HOST_DOWN:
    case ekRUNSTATE_VMWARE_HOST_SSH:
    case ekRUNSTATE_VMWARE_HOST_VMRUN:
        return FALSE;

    /* If VMware was booting by nboot, we shutdown it */
    case ekRUNSTATE_VMWARE_WAKE_UP:
    case ekRUNSTATE_VMWARE_TIMEOUT:
    {
        const Login *login = host_login(host);
        return ssh_shutdown(login);
    }

    case ekRUNSTATE_MACOS_UNKNOWN_VERSION:
        return FALSE;

    case ekRUNSTATE_MACOS_NOT_BOOTABLE:
        return FALSE;

    case ekRUNSTATE_MACOS_WAKE_UP:
    case ekRUNSTATE_MACOS_CANT_BOOT_FROM_VOLUME:
    case ekRUNSTATE_MACOS_TIMEOUT:
        return FALSE;

    /* We can't shutdown an unreachable host */
    case ekRUNSTATE_UNREACHABLE:
        return FALSE;

    default:
        cassert_default(state);
    }

    return FALSE;
}
