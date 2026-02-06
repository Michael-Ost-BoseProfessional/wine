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

WINE_DEFAULT_DEBUG_CHANNEL(winejack);

static BOOL sInitialized = FALSE;

#define MAX_WINE_THREADS 50
static HANDLE sThreadHandles[MAX_WINE_THREADS];
static INT sThreadHandleCount = 0;

static NTSTATUS WINAPI pe_buffer_size_callback(void *args, ULONG len);
static NTSTATUS WINAPI pe_create_thread_callback(void *args, ULONG len);
static NTSTATUS WINAPI pe_process_callback(void *args, ULONG len);
static NTSTATUS WINAPI pe_sample_rate_callback(void *args, ULONG len);
static NTSTATUS WINAPI pe_shutdown_callback(void *args, ULONG len);

/***********************************************************************
 *           DllMain
 */
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    int i;

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    {
        struct process_attach_params attachParams;
        NTSTATUS status;

        DisableThreadLibraryCalls(instance);

        status = __wine_init_unix_call();
        if (status) {
            ERR("Failed to initialize unix call interface: 0x%lx\n", status);
            return FALSE;
        }

        // Pass PE callbacks to the linux side that it will invoke
        attachParams.pe_buffer_size_callback = (uint64_t) pe_buffer_size_callback;
        attachParams.pe_create_thread_callback = (uint64_t) pe_create_thread_callback;
        attachParams.pe_process_callback = (uint64_t) pe_process_callback;
        attachParams.pe_sample_rate_callback = (uint64_t) pe_sample_rate_callback;
        attachParams.pe_shutdown_callback = (uint64_t) pe_shutdown_callback;
        status = WINE_UNIX_CALL(process_attach_id, &attachParams);
        if (status) {
            ERR("Failed to register process callback: 0x%lx\n", status);
            return FALSE;
        }

        sInitialized = TRUE;
        TRACE("winejack loaded\n");
        break;
    }
    case DLL_PROCESS_DETACH:
        for (i = 0; i < sThreadHandleCount; ++i) {
            if (sThreadHandles[i]) {
                CloseHandle(sThreadHandles[i]);
                sThreadHandles[i] = 0;
            }
        }
        sThreadHandleCount = 0;
        TRACE("winejack unloaded\n");
        break;
    }
    return TRUE;
}

/***********************************************************************
 *           jack_activate
 */
int32_t WINAPI jack_activate(wine_jack_client_t* client)
{
    struct jack_activate_params params = {
        .client = client,
        .result = -1
    };
    NTSTATUS nts;

    if (!sInitialized || !client) return -1;

    nts = WINE_UNIX_CALL(jack_activate_id, &params);
    
    TRACE("client=%p => result=%d\n", client, params.result);
    if (nts != _STATUS_SUCCESS)
        ERR("unix call failed: 0x%lx\n", nts);

    return params.result;
}

/***********************************************************************
 *           jack_client_close
 */
int32_t WINAPI jack_client_close(wine_jack_client_t* client)
{
    struct jack_client_close_params params = {
        .client = client,
        .result = -1
    };
    NTSTATUS nts;

    if (!sInitialized || !client) return -1;

    nts = WINE_UNIX_CALL(jack_client_close_id, &params);
    
    TRACE("client=%p => result=%d\n", client, params.result);
    if (nts != _STATUS_SUCCESS)
        ERR("unix call failed: 0x%lx\n", nts);

    return params.result;
}

/***********************************************************************
 *           jack_client_open
 */ 
wine_jack_client_t* WINAPI jack_client_open(const char *client_name, wine_jack_options_t options, wine_jack_status_t *status)
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
    
    nts = WINE_UNIX_CALL(jack_client_open_id, &params);
    
    TRACE("name=%s, options=0x%x => client=%p, status=%d\n", client_name, options, params.client, params.status);
    if (nts != _STATUS_SUCCESS)
    ERR("unix call failed: 0x%lx\n", nts);
    
    if (status) *status = params.status;
    
    return params.client;
}    

/***********************************************************************
 *           jack_deactivate
 */
int32_t WINAPI jack_deactivate(wine_jack_client_t* client)
{
    struct jack_deactivate_params params = {
        .client = client,
        .result = -1
    };
    NTSTATUS nts;

    if (!sInitialized || !client) return -1;

    nts = WINE_UNIX_CALL(jack_deactivate_id, &params);
    
    TRACE("client=%p => result=%d\n", client, params.result);
    if (nts != _STATUS_SUCCESS)
        ERR("unix call failed: 0x%lx\n", nts);

    return params.result;
}

/***********************************************************************
 *           jack_get_ports
 */
const char** WINAPI jack_get_ports(wine_jack_client_t* client, const char* port_name_pattern,
                                   const char* type_name_pattern, uint64_t flags)
{
    struct jack_get_ports_params params = {
        .client = client,
        .port_name_pattern = port_name_pattern,
        .type_name_pattern = type_name_pattern,
        .flags = flags,
        .ports_buffer = 0,
    };
    NTSTATUS nts;

    if (!sInitialized || !client) return 0;

    nts = WINE_UNIX_CALL(jack_get_ports_id, &params);
    
    TRACE("client=%p, name_pat=%s, type_pat=%s, flags=0x%llx => ports_buffer=%p\n", client, port_name_pattern, 
          type_name_pattern, flags, params.ports_buffer);
    if (nts != _STATUS_SUCCESS)
        ERR("unix call failed: 0x%lx\n", nts);

    return params.ports_buffer;
}

/***********************************************************************
 *           jack_get_sample_rate
 */
wine_jack_nframes_t WINAPI jack_get_sample_rate(wine_jack_client_t* client)
{
    struct jack_get_sample_rate_params params = {
        .client = client,
        .sample_rate = 0
    };
    NTSTATUS nts;

    if (!sInitialized || !client) return 0;

    nts = WINE_UNIX_CALL(jack_get_sample_rate_id, &params);

    if (nts != _STATUS_SUCCESS)
        ERR("unix call failed: 0x%lx\n", nts);

    return params.sample_rate;
}

/***********************************************************************
 *           jack_on_shutdown
 */
void WINAPI jack_on_shutdown(wine_jack_client_t* client, void* callback, void* arg)
{
    struct jack_on_shutdown_params params = {
        .client = client,
        .pe_callback = callback,
        .arg = arg
    };
    NTSTATUS nts;

    if (!sInitialized || !client) return;

    nts = WINE_UNIX_CALL(jack_on_shutdown_id, &params);
    
    TRACE("client=%p, callback=%p, arg=%p\n", client, callback, arg);
    if (nts != _STATUS_SUCCESS)
        ERR("unix call failed: 0x%lx\n", nts);
}

/***********************************************************************
 *           jack_set_buffer_size_callback
 */
int WINAPI jack_set_buffer_size_callback(wine_jack_client_t* client, void* callback, void* arg)
{
    struct jack_set_buffer_size_callback_params params = {
        .client = client,
        .pe_callback = callback,
        .arg = arg,
        .result = -1
    };
    NTSTATUS nts;
    static wine_jack_client_t* s_client;

    if (!sInitialized || !client) return 0;

    if (s_client && s_client != client) {
        FIXME("TODO: add support for jack_set_buffer_size_callback with multiple clients ");
        return -1;
    }
    s_client = client;

    nts = WINE_UNIX_CALL(jack_set_buffer_size_callback_id, &params);

    TRACE("client=%p, callback=%p, arg=%p => result=%d\n",
          client, callback, arg, params.result);
    if (nts != _STATUS_SUCCESS)
        ERR("unix call failed: 0x%lx\n", nts);

    return params.result;
}

/***********************************************************************
 *           jack_set_process_callback
 */
int WINAPI jack_set_process_callback(wine_jack_client_t* client, void* callback, void* arg)
{
    struct jack_set_process_callback_params params = {
        .client = client,
        .pe_callback = callback,
        .arg = arg,
        .result = -1
    };
    NTSTATUS nts;
    static wine_jack_client_t* s_client;

    if (!sInitialized || !client) return 0;

    if (s_client && s_client != client) {
        FIXME("TODO: add support for jack_set_process_callback with multiple clients ");
        return -1;
    }
    s_client = client;

    nts = WINE_UNIX_CALL(jack_set_process_callback_id, &params);

    TRACE("client=%p, callback=%p, arg=%p => result=%d\n",
          client, callback, arg, params.result);
    if (nts != _STATUS_SUCCESS)
        ERR("unix call failed: 0x%lx\n", nts);

    return params.result;
}

/***********************************************************************
 *           jack_set_sample_rate_callback
 */
int WINAPI jack_set_sample_rate_callback(wine_jack_client_t* client, void* callback, void* arg)
{
    struct jack_set_sample_rate_callback_params params = {
        .client = client,
        .pe_callback = callback,
        .arg = arg,
        .result = -1
    };
    NTSTATUS nts;
    static wine_jack_client_t* s_client;

    if (!sInitialized || !client) return 0;

    if (s_client && s_client != client) {
        FIXME("TODO: add support for jack_set_sample_rate_callback with multiple clients ");
        return -1;
    }
    s_client = client;

    nts = WINE_UNIX_CALL(jack_set_sample_rate_callback_id, &params);

    TRACE("client=%p, callback=%p, arg=%p => result=%d\n",
          client, callback, arg, params.result);
    if (nts != _STATUS_SUCCESS)
        ERR("unix call failed: 0x%lx\n", nts);

    return params.result;
}

/***********************************************************************
 *           jack_last_frame_time
 */
wine_jack_nframes_t WINAPI jack_last_frame_time(wine_jack_client_t* client)
{
    struct jack_last_frame_time_params params = {
        .client = client,
        .nframes = 0
    };
    NTSTATUS nts;

    if (!sInitialized || !client) return 0;

    nts = WINE_UNIX_CALL(jack_last_frame_time_id, &params);

    if (nts != _STATUS_SUCCESS)
        ERR("unix call failed: 0x%lx\n", nts);

    return params.nframes;
}

/***********************************************************************
 *           jack_connect
 */
int WINAPI jack_connect(wine_jack_client_t* client, const char* source_port, const char* destination_port)
{
    struct jack_connect_params params = {
        .client = client,
        .source_port = source_port,
        .destination_port = destination_port,
        .result = 0
    };
    NTSTATUS nts;

    if (!sInitialized || !client) return -1;

    nts = WINE_UNIX_CALL(jack_connect_id, &params);
    
    TRACE("client=%p, src=%s, dst=%s => result=%d\n", client, source_port, destination_port, params.result);
    if (nts != _STATUS_SUCCESS)
        ERR("unix call failed: 0x%lx\n", nts);

    return params.result;
}

/***********************************************************************
 *           jack_disconnect
 */
int WINAPI jack_disconnect(wine_jack_client_t* client, const char *source_port, const char *destination_port)
{
    struct jack_disconnect_params params = {
        .client = client,
        .source_port = source_port,
        .destination_port = destination_port,
        .result = 0
    };
    NTSTATUS nts;

    if (!sInitialized || !client) return -1;

    
    nts = WINE_UNIX_CALL(jack_disconnect_id, &params);
    
    TRACE("client=%p, src=%s, dst=%s => result=%d\n", client, source_port, destination_port, params.result);
    if (nts != _STATUS_SUCCESS)
        ERR("unix call failed: 0x%lx\n", nts);

    return params.result;
}

/***********************************************************************
 *           jack_port_get_buffer
 */
void* WINAPI jack_port_get_buffer(wine_jack_port_t* port, wine_jack_nframes_t nframes)
{
    struct jack_port_get_buffer_params params = {
        .port = port,
        .nframes = nframes,
        .buffer = 0
    };
    NTSTATUS nts;

    if (!sInitialized) return 0;

    nts = WINE_UNIX_CALL(jack_port_get_buffer_id, &params);

    if (nts != _STATUS_SUCCESS)
        TRACE("unix call failed: 0x%lx\n", nts);

    return params.buffer;
}

/***********************************************************************
 *           jack_port_name
 */
const char* WINAPI jack_port_name(wine_jack_port_t* port)
{
    struct jack_port_name_params params = {
        .port = port,
        .name = 0
    };
    NTSTATUS nts;

    if (!sInitialized || !port) return FALSE;

    nts = WINE_UNIX_CALL(jack_port_name_id, &params);
    
    TRACE("port=%p => name=%s\n", port, params.name);
    if (nts != _STATUS_SUCCESS)
        ERR("unix call failed: 0x%lx\n", nts);

    return params.name;
}

/***********************************************************************
 *           jack_port_register
 */
wine_jack_port_t* WINAPI jack_port_register(wine_jack_client_t* client, const char *port_name,
                                            const char *port_type, uint64_t flags,
                                            uint64_t buffer_size)
{
    struct jack_port_register_params params = {
        .client = client,
        .port_name = port_name,
        .port_type = port_type,
        .flags = flags,
        .buffer_size = buffer_size,
        .port = 0
    };
    NTSTATUS nts;

    if (!sInitialized || !client) return 0;

    nts = WINE_UNIX_CALL(jack_port_register_id, &params);
    
    TRACE("client=%p, name=%s, type=%s, flags=0x%llx => port=%p\n", client, port_name, port_type, flags, params.port);
    if (nts != _STATUS_SUCCESS)
        ERR("unix call failed: 0x%lx\n", nts);

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
    NTSTATUS nts;

    if (!sInitialized || !ptr) return;

    nts = WINE_UNIX_CALL(jack_free_id, &params);

    TRACE("ptr=%p\n", ptr);
    if (nts != _STATUS_SUCCESS)
        ERR("unix call failed: 0x%lx\n", nts);
}

/***********************************************************************
 *           pe_create_thread_callback
 * 
 * See Threads in README.md for an explanation.
 */

struct thread_entry_args
{
    uint64_t synchronizer;
    uint64_t function;
    uint64_t arg;
    BOOL realtime;
};    

DWORD WINAPI pe_thread_entry(LPVOID lpParam) 
{
    struct thread_entry_args* args = (struct thread_entry_args*) lpParam;
    struct run_pthread_params params = {
        .synchronizer = args->synchronizer,
        .function = args->function,
        .arg = args->arg
    };

    if (args->realtime) {
        if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS) ||
            !SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL)) {
            WARN("SetPriorityClass/SetThreadPriority failed. Audio may glitch");
        }
    }
   
    // Call back into linux to run the thread function, capture the pthread_t,
    // and release the linux_create_thread_callback
    WINE_UNIX_CALL(run_pthread_id, &params);
    
    TRACE("function=%llx, arg=%llx, realtime=%d => (none)\n", args->function, args->arg, args->realtime);

    return 0;
}

static NTSTATUS WINAPI pe_create_thread_callback(void *args, ULONG len)
 {
    struct pe_create_thread_callback_params *params = args;
    struct thread_entry_args entry_args = {
        .synchronizer = params->synchronizer,
        .function = params->function,
        .arg = params->arg,
        .realtime = params->realtime
    };
    HANDLE hThread;

    if (sThreadHandleCount == MAX_WINE_THREADS)
        return _STATUS_INVALID_PARAMETER;

    hThread = CreateThread(NULL, 0, pe_thread_entry, &entry_args, 0, NULL);
    if (!hThread)
        return _STATUS_INVALID_PARAMETER;

    sThreadHandles[sThreadHandleCount++] = hThread;

    TRACE("function=%llx, arg=%llx, realtime=%d => hThread=%p\n", params->function,
        params->arg, params->realtime, hThread);

    return _STATUS_SUCCESS;
}

/***********************************************************************
 *           pe_callback_nframes_arg
 *
 * Shared function for pe_process_callback, pe_sample_rate_callback, and 
 * pe_buffer_size_callback which all share the same signature.
 */
static NTSTATUS WINAPI pe_callback_nframes_arg(void *args, ULONG len)
{
    typedef int (*callback_nframes_arg_t)(wine_jack_nframes_t nframes, void *arg);

    struct pe_callback_nframes_arg_params *params = args;
    callback_nframes_arg_t pe_callback;
    int32_t result;

    if (len < sizeof(*params) || !params->pe_callback)
        return _STATUS_INVALID_PARAMETER;

    // Respond to call from KeUserDispatchCallback on the linux side
    pe_callback = (callback_nframes_arg_t) params->pe_callback;
    result = pe_callback(params->nframes, params->arg);

    // Return result to linux side
    return NtCallbackReturn(&result, sizeof(result), _STATUS_SUCCESS);
}

/***********************************************************************
 *           pe_buffer_size_callback
 *
 */
static NTSTATUS WINAPI pe_buffer_size_callback(void *args, ULONG len)
{
    return pe_callback_nframes_arg(args, len);
}

/***********************************************************************
 *           pe_process_callback
 *
 */
static NTSTATUS WINAPI pe_process_callback(void *args, ULONG len)
{
    return pe_callback_nframes_arg(args, len);
}

/***********************************************************************
 *           pe_sample_rate_callback
 *
 */
static NTSTATUS WINAPI pe_sample_rate_callback(void *args, ULONG len)
{
    return pe_callback_nframes_arg(args, len);
}

/***********************************************************************
 *           pe_shutdown_callback
 *
 */
static NTSTATUS WINAPI pe_shutdown_callback(void *args, ULONG len)
{
    typedef void (*wine_jack_shutdown_callback_t)(void *arg);

    struct pe_shutdown_callback_params *params = args;
    wine_jack_shutdown_callback_t pe_callback;

    if (len < sizeof(*params) || !params->pe_callback)
        return _STATUS_INVALID_PARAMETER;

    pe_callback = (wine_jack_shutdown_callback_t) params->pe_callback;
    pe_callback(params->arg);

    return _STATUS_SUCCESS;
}
