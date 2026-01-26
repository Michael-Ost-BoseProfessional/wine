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

// This thunking layer passes char* pointers directly between PE and Unix code.
// This only works when both sides have the same pointer size (64-bit).
// If you need 32-bit PE support, all char* params must be changed to uint64_t and 
// strings must be copied between layers.
C_ASSERT(sizeof(char*) == sizeof(uint64_t));
// Likewise for passing these int types between layers
C_ASSERT(sizeof(jack_options_t) == sizeof(uint32_t));
C_ASSERT(sizeof(jack_status_t) == sizeof(uint32_t));
C_ASSERT(sizeof(jack_nframes_t) == sizeof(uint32_t));

WINE_DEFAULT_DEBUG_CHANNEL(jack);

#define UNIX_CALL(func, params) WINE_UNIX_CALL(func, params)

static BOOL sInitialized = FALSE;

/***********************************************************************
 *           DllMain
 */
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(instance);
        if (__wine_init_unix_call()) {
            ERR("Failed to initialize unix call interface\n");
            return FALSE;
        }
        sInitialized = TRUE;
        TRACE("winejack loaded\n");
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

/***********************************************************************
 *           jack_activate
 */
int WINAPI jack_activate(UINT64 client)
{
    struct jack_activate_params params = {
        .client = client,
        .result = -1
    };

    if (!sInitialized || !client) return -1;

    TRACE("client=0x%llx\n", (unsigned long long)client);

    UNIX_CALL(unix_jack_activate_id, &params);

    return params.result;
}

/***********************************************************************
 *           jack_client_close
 */
int WINAPI jack_client_close(UINT64 client)
{
    struct jack_client_close_params params = {
        .client = client,
        .result = -1
    };

    if (!sInitialized || !client) return -1;

    TRACE("client=0x%llx\n", (unsigned long long)client);

    UNIX_CALL(jack_client_close_id, &params);

    return params.result;
}

/***********************************************************************
 *           jack_client_open
 */ 
UINT64 WINAPI jack_client_open(const char *client_name, jack_options_t options, jack_status_t *status)
{
    struct jack_client_open_params params = {
        .client_name = client_name,
        .options = options,
        .status = 0,
        .client = 0
    };    
    NTSTATUS nts;

    if (!sInitialized)
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
 *           jack_get_ports
 */
const char** WINAPI jack_get_ports(UINT64 client, const char* port_name_pattern,
                                   const char* type_name_pattern, unsigned long flags)
{
    struct jack_get_ports_params params = {
        .client = client,
        .port_name_pattern = port_name_pattern,
        .type_name_pattern = type_name_pattern,
        .flags = flags,
        .ports_buffer = 0,
    };

    if (!sInitialized || !client) return 0;

    TRACE("client=0x%llx, name_pat=%s, type_pat=%s, flags=0x%lx\n",
          (unsigned long long)client, port_name_pattern, type_name_pattern, flags);

    UNIX_CALL(jack_get_ports_id, &params);

    return params.ports_buffer;
}

/***********************************************************************
 *           jack_get_sample_rate
 */
UINT32 WINAPI jack_get_sample_rate(UINT64 client)
{
    struct jack_get_sample_rate_params params = {
        .client = client,
        .sample_rate = 0
    };

    if (!sInitialized || !client) return 0;

    TRACE("client=0x%llx\n", 
        client);
   
    params.sample_rate = UNIX_CALL(jack_get_sample_rate_id, &params);

    return params.sample_rate;
}

/***********************************************************************
 *           jack_set_process_callback
 */
int WINAPI jack_set_process_callback(UINT64 client, void* process_callback, void* arg)
{
    struct jack_set_process_callback_params params = {
        .client = client,
        .process_callback = process_callback,
        .arg = arg,
        result = 0
    };

    if (!sInitialized || !client) return 0;

    TRACE("client=0x%llx, callback=%p, arg=%p\n",
          (unsigned long long)client, params.process_callback, params.arg);

    UNIX_CALL(jack_set_process_callback_id, &params);

    return params.result;
}

/***********************************************************************
 *           jack_on_shutdown
 */
int WINAPI jack_on_shutdown(UINT64 client, void* shutdown_callback, void* arg)
{
    struct jack_on_shutdown_params params = {
        .client = client,
        .shutdown_callback = shutdown_callback,
        .arg = arg
    };

    if (!sInitialized || !client) return 0;

    TRACE("client=0x%llx, callback=%p, arg=%p\n",
          (unsigned long long)client, params.shutdown_callback, params.arg);

    UNIX_CALL(jack_on_shutdown_id, &params);
}

/***********************************************************************
 *           jack_connect
 */
int WINAPI jack_connect(UINT64 client, const char* source_port, const char* destination_port)
{
    struct jack_connect_params params = {
        .client = client,
        .source_port = source_port,
        .destination_port = destination_port,
        .result = 0
    };

    if (!sInitialized || !client) return -1;

    TRACE("client=0x%llx, src=%s, dst=%s\n",
          (unsigned long long)client, source_port, destination_port);

    UNIX_CALL(jack_connect_id, &params);

    return params.result;
}

/***********************************************************************
 *           jack_disconnect
 */
int WINAPI jack_disconnect(UINT64 client, const char *source_port, const char *destination_port)
{
    struct jack_disconnect_params params = {
        .client = client,
        .source_port = source_port,
        .destination_port = destination_port,
        .result = 0
    };

    if (!sInitialized || !client) return -1;

    TRACE("client=0x%llx, src=%s, dst=%s\n",
          (unsigned long long)client, source_port, destination_port);

    UNIX_CALL(jack_disconnect_id, &params);

    return params.result;
}

/***********************************************************************
 *           jack_port_get_buffer
 */
UINT64 WINAPI jack_port_get_buffer(UINT64 port, jack_nframes_t nframes)
{
    struct jack_port_get_buffer_params params = {
        .port = port,
        .nframes = nframes,
        .buffer = 0
    };

    if (!sInitialized || !client) return 0;

    TRACE("port=0x%llx, nframes=%d\n", 
        port, nframes);

    UNIX_CALL(jack_port_get_buffer_id, &params);

    return params.buffer;
}

/***********************************************************************
 *           jack_port_name
 */
UINT64 WINAPI jack_port_name(UINT64 port)
{
    struct jack_port_name_params params = {
        .port = port,
        .name = 0
    };
    NTSTATUS nts;

    if (!sInitialized || !port) return FALSE;

    TRACE("port=0x%llx\n", port);

    nts = UNIX_CALL(jack_port_name_id, &params);

    return params.name;
}

/***********************************************************************
 *           jack_port_register
 */
UINT64 WINAPI jack_port_register(UINT64 client, const char *port_name,
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

    if (!sInitialized || !client) return 0;

    TRACE("client=0x%llx, name=%s, type=%s, flags=0x%lx\n",
          (unsigned long long)client, port_name, port_type, flags);

    UNIX_CALL(jack_port_register_id, &params);

    return params.port;
}

/***********************************************************************
 *           jack_free
 */
void WINAPI jack_free(void* ptr)
{
    struct jack_free_params params = {
        .ptr = ptr
    };

    if (!sInitialized || !ptr) return;

    TRACE("ptr=%p\n", 
        ptr);

    UNIX_CALL(jack_free_id, &params);
}
