/*
 * Wine JACK driver - PE-side wrapper functions
 *
 * Copyright 2026 Michael Ost / Bose Professional
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
// TODO: fix these includes
#define _STATUS_SUCCESS             ((NTSTATUS) 0x00000000)
#define _STATUS_INVALID_PARAMETER   ((NTSTATUS) 0xC000000D)

#include "wine/debug.h"
#include "wine/unixlib.h"
#include "winejack_unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(jack);

#define UNIX_CALL(func, params) WINE_UNIX_CALL(func, params)

static BOOL initialized = FALSE;

/***********************************************************************
 *           DllMain
 */
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(instance);
        if (__wine_init_unix_call())
        {
            ERR("Failed to initialize unix call interface\n");
            return FALSE;
        }
        initialized = TRUE;
        TRACE("winejack loaded\n");
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

/***********************************************************************
 *           jack_client_open
 */
UINT64 WINAPI jack_client_open(const char *client_name, int options, int *status)
{
    struct jack_client_open_params params = {
        .client_name = client_name,
        .options = options,
        .status = 0,
        .client = 0
    };
    NTSTATUS nts;

    if (!initialized)
    {
        ERR("winejack not initialized\n");
        if (status) *status = 0x01; // JACK_STATUS_FAILURE
        return 0;
    }

    TRACE("name=%s, options=0x%x\n", client_name, options);

    nts = UNIX_CALL(jack_client_open_id, &params);

    if (status) *status = params.status;

    if (nts != _STATUS_SUCCESS)
    {
        ERR("unix call failed: 0x%lx\n", nts);
        return 0;
    }

    return params.client;
}

/***********************************************************************
 *           wine_jack_client_close
 */
int WINAPI wine_jack_client_close(UINT64 client)
{
    struct jack_client_close_params params = {
        .client = client,
        .result = -1
    };

    if (!initialized || !client) return -1;

    TRACE("client=0x%llx\n", (unsigned long long)client);

    UNIX_CALL(jack_client_close_id, &params);

    return params.result;
}

#if LATER
/***********************************************************************
 *           WineJack_Activate
 */
int WINAPI WineJack_Activate(UINT64 client)
{
    struct jack_activate_params params = {
        .client = client,
        .result = -1
    };

    if (!initialized || !client) return -1;

    TRACE("client=0x%llx\n", (unsigned long long)client);

    UNIX_CALL(unix_jack_activate, &params);

    return params.result;
}

/***********************************************************************
 *           WineJack_Deactivate
 */
int WINAPI WineJack_Deactivate(UINT64 client)
{
    struct jack_deactivate_params params = {
        .client = client,
        .result = -1
    };

    if (!initialized || !client) return -1;

    TRACE("client=0x%llx\n", (unsigned long long)client);

    UNIX_CALL(unix_jack_deactivate, &params);

    return params.result;
}

/***********************************************************************
 *           WineJack_Connect
 */
int WINAPI WineJack_Connect(UINT64 client, const char *source_port, const char *destination_port)
{
    struct jack_connect_params params = {
        .client = client,
        .source_port = source_port,
        .destination_port = destination_port,
        .result = -1
    };

    if (!initialized || !client) return -1;

    TRACE("client=0x%llx, src=%s, dst=%s\n",
          (unsigned long long)client, source_port, destination_port);

    UNIX_CALL(unix_jack_connect, &params);

    return params.result;
}

/***********************************************************************
 *           WineJack_Disconnect
 */
int WINAPI WineJack_Disconnect(UINT64 client, const char *source_port, const char *destination_port)
{
    struct jack_disconnect_params params = {
        .client = client,
        .source_port = source_port,
        .destination_port = destination_port,
        .result = -1
    };

    if (!initialized || !client) return -1;

    TRACE("client=0x%llx, src=%s, dst=%s\n",
          (unsigned long long)client, source_port, destination_port);

    UNIX_CALL(unix_jack_disconnect, &params);

    return params.result;
}

/***********************************************************************
 *           WineJack_PortRegister
 */
UINT64 WINAPI WineJack_PortRegister(UINT64 client, const char *port_name,
                                     const char *port_type, unsigned long flags,
                                     unsigned long buffer_size)
{
    struct jack_port_register_params params = {
        .client = client,
        .port_name = port_name,
        .port_type = port_type,
        .flags = flags,
        .buffer_size = buffer_size,
        .port = 0
    };

    if (!initialized || !client) return 0;

    TRACE("client=0x%llx, name=%s, type=%s, flags=0x%lx\n",
          (unsigned long long)client, port_name, port_type, flags);

    UNIX_CALL(unix_jack_port_register, &params);

    return params.port;
}

/***********************************************************************
 *           WineJack_PortUnregister
 */
int WINAPI WineJack_PortUnregister(UINT64 client, UINT64 port)
{
    struct jack_port_unregister_params params = {
        .client = client,
        .port = port,
        .result = -1
    };

    if (!initialized || !client) return -1;

    TRACE("client=0x%llx, port=0x%llx\n",
          (unsigned long long)client, (unsigned long long)port);

    UNIX_CALL(unix_jack_port_unregister, &params);

    return params.result;
}

/***********************************************************************
 *           WineJack_PortName
 */
BOOL WINAPI WineJack_PortName(UINT64 port, char *buffer, size_t buffer_size)
{
    struct jack_port_name_params params = {
        .port = port,
        .buffer_size = buffer_size,
        .buffer = buffer
    };
    NTSTATUS nts;

    if (!initialized || !port || !buffer || !buffer_size) return FALSE;

    nts = UNIX_CALL(unix_jack_port_name, &params);

    return (nts == STATUS_SUCCESS);
}

/***********************************************************************
 *           WineJack_GetPorts
 */
unsigned int WINAPI WineJack_GetPorts(UINT64 client, const char *port_name_pattern,
                                       const char *type_name_pattern, unsigned long flags,
                                       char *ports_buffer, size_t buffer_size)
{
    struct jack_get_ports_params params = {
        .client = client,
        .port_name_pattern = port_name_pattern,
        .type_name_pattern = type_name_pattern,
        .flags = flags,
        .buffer_size = buffer_size,
        .ports_buffer = ports_buffer,
        .count = 0
    };

    if (!initialized || !client) return 0;

    TRACE("client=0x%llx, name_pat=%s, type_pat=%s, flags=0x%lx\n",
          (unsigned long long)client, port_name_pattern, type_name_pattern, flags);

    UNIX_CALL(unix_jack_get_ports, &params);

    return params.count;
}

/***********************************************************************
 *           WineJack_PortByName
 */
UINT64 WINAPI WineJack_PortByName(UINT64 client, const char *port_name)
{
    struct jack_port_by_name_params params = {
        .client = client,
        .port_name = port_name,
        .port = 0
    };

    if (!initialized || !client) return 0;

    TRACE("client=0x%llx, name=%s\n", (unsigned long long)client, port_name);

    UNIX_CALL(unix_jack_port_by_name, &params);

    return params.port;
}

/***********************************************************************
 *           WineJack_GetSampleRate
 */
UINT32 WINAPI WineJack_GetSampleRate(UINT64 client)
{
    struct jack_get_sample_rate_params params = {
        .client = client,
        .sample_rate = 0
    };

    if (!initialized || !client) return 0;

    UNIX_CALL(unix_jack_get_sample_rate, &params);

    return params.sample_rate;
}

/***********************************************************************
 *           WineJack_GetBufferSize
 */
UINT32 WINAPI WineJack_GetBufferSize(UINT64 client)
{
    struct jack_get_buffer_size_params params = {
        .client = client,
        .buffer_size = 0
    };

    if (!initialized || !client) return 0;

    UNIX_CALL(unix_jack_get_buffer_size, &params);

    return params.buffer_size;
}

/***********************************************************************
 *           WineJack_GetClientName
 */
BOOL WINAPI WineJack_GetClientName(UINT64 client, char *buffer, size_t buffer_size)
{
    struct jack_get_client_name_params params = {
        .client = client,
        .buffer_size = buffer_size,
        .buffer = buffer
    };
    NTSTATUS nts;

    if (!initialized || !client || !buffer || !buffer_size) return FALSE;

    nts = UNIX_CALL(unix_jack_get_client_name, &params);

    return (nts == STATUS_SUCCESS);
}
#endif