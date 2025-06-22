/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: network.c
 *
 */

/* nbuild network */

#include "network.h"
#include "host.h"
#include <core/arrst.h>
#include <core/dbind.h>
#include <core/stream.h>
#include <core/strings.h>
#include <osbs/bproc.h>
#include <osbs/bsocket.h>
#include <osbs/osbs.h>
#include <sewer/cassert.h>
#include <sewer/unicode.h>

#define READ_BUFFER_SIZE 1024

/*---------------------------------------------------------------------------*/

void network_dbind(void)
{
    dbind(Login, String *, ip);
    dbind(Login, String *, user);
    dbind(Login, String *, pass);
    dbind(Login, uint8_t, platform);
    dbind(Login, bool_t, localhost);
    dbind(Login, bool_t, use_sshpass);
    dbind(Drive, String *, name);
    dbind(Drive, String *, path);
    dbind(Drive, Login, login);
    host_dbind();
    dbind(Network, Drive, drive);
    dbind(Network, ArrSt(Host) *, hosts);
}

/*---------------------------------------------------------------------------*/

static ___INLINE bool_t i_localhost(const char_t *name, const ArrSt(uint32_t) *ips)
{
    uint32_t ip;

    if (str_equ_c(name, "localhost") == TRUE)
        return TRUE;

    ip = bsocket_str_ip(name);
    arrst_foreach_const(ip2, ips, uint32_t)
        if (*ip2 == ip)
            return TRUE;
    arrst_end()

    return FALSE;
}

/*---------------------------------------------------------------------------*/

void network_localhost(Login *login, const ArrSt(uint32_t) *ips)
{
    cassert_no_null(login);
    login->localhost = i_localhost(tc(login->ip), ips);
}

/*---------------------------------------------------------------------------*/

static ArrSt(uint32_t) *i_parse_ipconfig(Stream *stm)
{
    ArrSt(uint32_t) *ips = arrst_create(uint32_t);

    stm_lines(line, stm)
        if (str_str(line, "IPv4 Address") != NULL)
        {
            String *str_ip = NULL;
            if (str_split(line, ":", NULL, &str_ip) == TRUE)
            {
                String *ip_trim = str_trim(tc(str_ip));
                uint32_t ip = bsocket_url_ip(tc(ip_trim), NULL);
                if (ip != 0)
                    arrst_append(ips, ip, uint32_t);
                str_destroy(&ip_trim);
            }

            str_destroy(&str_ip);
        }
    stm_next(line, stm)

    return ips;
}

/*---------------------------------------------------------------------------*/

static ArrSt(uint32_t) *i_parse_ifconfig(Stream *stm)
{
    ArrSt(uint32_t) *ips = arrst_create(uint32_t);

    stm_lines(line, stm)
        const char_t *inet = str_str(line, "inet ");
        if (inet != NULL)
        {
            char_t str_ip[32];
            uint32_t i = 0;

            /* Jump 'inet' */
            inet += 4;

            /* Jump initial spaces */
            while (*inet == ' ')
                inet++;

            /* Read the a.c.b.c ip address */
            while (i < (32 - 1))
            {
                str_ip[i] = *inet;
                inet++;
                i++;

                if (unicode_isspace(*inet) == TRUE)
                    break;
            }

            str_ip[i] = '\0';

            {
                uint32_t ip = bsocket_url_ip(str_ip, NULL);
                if (ip != 0)
                    arrst_append(ips, ip, uint32_t);
            }
        }
    stm_next(line, stm)

    return ips;
}

/*---------------------------------------------------------------------------*/

ArrSt(uint32_t) *network_local_ips(void)
{
    const char_t *cmd = NULL;
    Stream *stm = NULL;
    ArrSt(uint32_t) *ips = NULL;

    if (osbs_platform() == ekWINDOWS)
        cmd = "ipconfig";
    else if (osbs_platform() == ekLINUX)
        cmd = "ifconfig";

    if (cmd != NULL)
    {
        Proc *proc = bproc_exec(cmd, NULL);
        if (proc != NULL)
        {
            byte_t buffer[READ_BUFFER_SIZE];
            uint32_t rsize;
            stm = stm_memory(READ_BUFFER_SIZE);
            while (bproc_read(proc, buffer, READ_BUFFER_SIZE, &rsize, NULL) == TRUE)
                stm_write(stm, buffer, rsize);
            bproc_close(&proc);
        }
    }

    if (stm != NULL)
    {
        if (osbs_platform() == ekWINDOWS)
            ips = i_parse_ipconfig(stm);
        else if (osbs_platform() == ekLINUX)
            ips = i_parse_ifconfig(stm);

        stm_close(&stm);
    }

    return ips;
}
