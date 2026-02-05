/*
 * Wine JACK unixlib - Linux-side calls to JACK from thunked Windows/PE calls
 *
 * Copyright 2026 - Michael Ost / Bose Professional
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
#if 0
#pragma makedep unix
#endif

#include "winejack_unixlib.h"

#include "winternl.h"
#include "wine/debug.h"
#include "wine/unixlib.h"
#include "ntuser.h"
// TODO: fix these includes
#define _STATUS_SUCCESS             ((NTSTATUS) 0x00000000)
#define _STATUS_INVALID_PARAMETER   ((NTSTATUS) 0xC000000D)
#define _STATUS_ALREADY_INITIALIZED ((NTSTATUS) 0xC0000228)

#include <errno.h>
#include <string.h>
#include <jack/jack.h>
#include <jack/thread.h>

// verify the size of same-named winejack_unixlib.h types
C_ASSERT(sizeof(jack_client_t*) == sizeof(wine_jack_client_t*));
C_ASSERT(sizeof(jack_options_t) == sizeof(wine_jack_options_t));
C_ASSERT(sizeof(jack_status_t) == sizeof(wine_jack_status_t));
C_ASSERT(sizeof(jack_nframes_t) == sizeof(wine_jack_nframes_t));

// This thunking layer passes char* pointers directly between PE and Unix code.
// This only works when both sides have the same pointer size (64-bit).
// If you need 32-bit PE support, all char* params must be changed to uint64_t and 
// strings must be copied between layers.
C_ASSERT(sizeof(char*) == sizeof(uint64_t));

WINE_DEFAULT_DEBUG_CHANNEL(winejack);

static uint64_t pe_buffer_size_callback;
static uint64_t pe_create_thread_callback;
static uint64_t pe_process_callback;
static uint64_t pe_sample_rate_callback;
static uint64_t pe_shutdown_callback;

struct callback_holder_t {
    void* pe_callback;
    void* arg;
};

static int linux_create_thread_callback(pthread_t* id, const pthread_attr_t* attr, void* (*function)(void*), void* arg);

/*
 * linux_process_attach
 */
static NTSTATUS linux_process_attach(void *args)
{
    struct process_attach_params *params = args;

    pe_buffer_size_callback = params->pe_buffer_size_callback;
    pe_process_callback = params->pe_process_callback;
    pe_sample_rate_callback = params->pe_sample_rate_callback;
    pe_shutdown_callback = params->pe_shutdown_callback;

    pe_create_thread_callback = params->pe_create_thread_callback;
    jack_set_thread_creator(linux_create_thread_callback);

    TRACE("pe_process_callback=%lx, pe_create_thread_callback=%lx, pe_shutdown_callback=%lx\n", pe_process_callback, pe_create_thread_callback, pe_shutdown_callback);

    return _STATUS_SUCCESS;
}

/*
 * linux_jack_activate
 */
static NTSTATUS linux_jack_activate(void* args)
{
    struct jack_activate_params* params = args;
    jack_client_t* client = (jack_client_t*)params->client;

    params->result = jack_activate(client);

    return params->result == 0? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

/*
 * linux_jack_client_close
 */ 
static NTSTATUS linux_jack_client_close(void* args)
{
    struct jack_client_close_params* params = args;
    jack_client_t* client = (jack_client_t*)params->client;

    params->result = jack_client_close(client);

    return params->result == 0? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}    

/*
 * linux_jack_client_open
 */  
static NTSTATUS linux_jack_client_open(void* args)
{
    struct jack_client_open_params* params = args;
    jack_status_t status;
    jack_client_t* client;
    
    client = jack_client_open(params->client_name, params->options, &status);
    
    params->status = status;
    params->client = (jack_client_t*)client;
    
    return params->client? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}        

/*
 * linux_jack_deactivate
 */
static NTSTATUS linux_jack_deactivate(void* args)
{
    struct jack_deactivate_params* params = args;
    jack_client_t* client = (jack_client_t*)params->client;

    params->result = jack_deactivate(client);

    return params->result == 0? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

/*
 * linux_jack_get_ports
 */
static NTSTATUS linux_jack_get_ports(void *args)
{
    struct jack_get_ports_params *params = args;
    jack_client_t *client = (jack_client_t*)params->client;

    params->ports_buffer = jack_get_ports(client, params->port_name_pattern,
        params->type_name_pattern, params->flags);
        
    return params->ports_buffer? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

/*
 * linux_jack_get_sample_rate
 */
static NTSTATUS linux_jack_get_sample_rate(void *args)
{
    struct jack_get_sample_rate_params *params = args;
    jack_client_t *client = (jack_client_t*)params->client;

    params->sample_rate = jack_get_sample_rate(client);

    return _STATUS_SUCCESS;
}

/*
 * linux_jack_on_shutdown
 */
static void linux_on_shutdown_callback(void *arg)
{
    struct callback_holder_t* holder = (struct callback_holder_t*) arg;
    struct pe_shutdown_callback_params params = {
        .dispatch = {.callback = pe_shutdown_callback},
        .pe_callback = (uint64_t)holder->pe_callback,
        .arg = holder->arg
    };
    struct dispatch_callback_params* dispatch = (struct dispatch_callback_params*) &params.dispatch;
    void *ret_ptr = NULL;
    ULONG ret_len = 0;

    KeUserDispatchCallback(dispatch, sizeof(params), &ret_ptr, &ret_len);
}

static NTSTATUS linux_jack_on_shutdown(void *args)
{
    struct jack_on_shutdown_params* params = args;
    jack_client_t *client = (jack_client_t*)params->client;

    static struct callback_holder_t callback_holder;
    callback_holder.pe_callback = params->pe_callback;
    callback_holder.arg = params->arg;

    jack_on_shutdown(client, linux_on_shutdown_callback, (void*) &callback_holder);

    return _STATUS_SUCCESS;
}

/*
 * linux_jack_set_buffer_size_callback
 */
static int linux_buffer_size_callback(jack_nframes_t nframes, void *arg)
{
    struct callback_holder_t* holder = (struct callback_holder_t*)arg;
    struct pe_process_callback_params params = {
        .dispatch = {.callback = pe_buffer_size_callback},
        .pe_callback = (uint64_t)holder->pe_callback,
        .nframes = nframes,
        .arg = holder->arg,
        .result = 0
    };
    struct dispatch_callback_params* dispatch = (struct dispatch_callback_params*) &params.dispatch;
    void *ret_ptr = NULL;
    ULONG ret_len = 0;
    NTSTATUS status;

    status = KeUserDispatchCallback(dispatch, sizeof(params), &ret_ptr, &ret_len);

    if (status != 0 || ret_len != sizeof(int32_t))
        return -1;
    return *(int32_t *)ret_ptr;
}

static NTSTATUS linux_jack_set_buffer_size_callback(void *args)
{
    struct jack_set_buffer_size_callback_params *params = args;
    jack_client_t *client = (jack_client_t*)params->client;

    static struct callback_holder_t callback_holder;
    callback_holder.pe_callback = params->pe_callback;
    callback_holder.arg = params->arg;
    
    params->result = jack_set_buffer_size_callback(client, linux_buffer_size_callback, &callback_holder);

    return params->result == 0? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

/*
 * linux_jack_set_process_callback
 */
static int linux_process_callback(jack_nframes_t nframes, void *arg)
{
    struct callback_holder_t* holder = (struct callback_holder_t*)arg;
    struct pe_process_callback_params params = {
        .dispatch = {.callback = pe_process_callback},
        .pe_callback = (uint64_t)holder->pe_callback,
        .nframes = nframes,
        .arg = holder->arg,
        .result = 0
    };
    struct dispatch_callback_params* dispatch = (struct dispatch_callback_params*) &params.dispatch;
    void *ret_ptr = NULL;
    ULONG ret_len = 0;
    NTSTATUS status;

    status = KeUserDispatchCallback(dispatch, sizeof(params), &ret_ptr, &ret_len);

    if (status != 0 || ret_len != sizeof(int32_t))
        return -1;
    return *(int32_t *)ret_ptr;
}

static NTSTATUS linux_jack_set_process_callback(void *args)
{
    struct jack_set_process_callback_params *params = args;
    jack_client_t *client = (jack_client_t*)params->client;

    static struct callback_holder_t callback_holder;
    callback_holder.pe_callback = params->pe_callback;
    callback_holder.arg = params->arg;
    
    params->result = jack_set_process_callback(client, linux_process_callback, &callback_holder);

    return params->result == 0? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

/*
 * linux_jack_set_sample_rate_callback
 */
static int linux_sample_rate_callback(jack_nframes_t nframes, void *arg)
{
    struct callback_holder_t* holder = (struct callback_holder_t*)arg;
    struct pe_process_callback_params params = {
        .dispatch = {.callback = pe_sample_rate_callback},
        .pe_callback = (uint64_t)holder->pe_callback,
        .nframes = nframes,
        .arg = holder->arg,
        .result = 0
    };
    struct dispatch_callback_params* dispatch = (struct dispatch_callback_params*) &params.dispatch;
    void *ret_ptr = NULL;
    ULONG ret_len = 0;
    NTSTATUS status;

    status = KeUserDispatchCallback(dispatch, sizeof(params), &ret_ptr, &ret_len);

    if (status != 0 || ret_len != sizeof(int32_t))
        return -1;
    return *(int32_t *)ret_ptr;
}

static NTSTATUS linux_jack_set_sample_rate_callback(void *args)
{
    struct jack_set_sample_rate_callback_params *params = args;
    jack_client_t *client = (jack_client_t*)params->client;

    static struct callback_holder_t callback_holder;
    callback_holder.pe_callback = params->pe_callback;
    callback_holder.arg = params->arg;
    
    params->result = jack_set_sample_rate_callback(client, linux_sample_rate_callback, &callback_holder);

    return params->result == 0? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

/*
 * linux_jack_connect
 */
static NTSTATUS linux_jack_connect(void *args)
{
    struct jack_connect_params *params = args;
    jack_client_t *client = (jack_client_t*)params->client;
 
    params->result = jack_connect(client, params->source_port,
        params->destination_port);
        
    return params->result == 0? _STATUS_SUCCESS : 
           params->result == EEXIST? _STATUS_ALREADY_INITIALIZED : 
           _STATUS_INVALID_PARAMETER;
}

/*
 * linux_jack_disconnect
 */
static NTSTATUS linux_jack_disconnect(void *args)
{
    struct jack_disconnect_params *params = args;
    jack_client_t *client = (jack_client_t*)params->client;
    
    params->result = jack_disconnect(client, params->source_port,
        params->destination_port);
        
    TRACE("client=%p, src=%s, dst=%s => result=%d\n", client,
            params->source_port, params->destination_port, params->result);

    return params->result == 0? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

/*
 * linux_jack_port_get_buffer
 */
static NTSTATUS linux_jack_port_get_buffer(void *args)
{
    struct jack_port_get_buffer_params* params = args;
    jack_port_t *port = (jack_port_t*)params->port;

    params->buffer = jack_port_get_buffer(port, params->nframes);

    return params->buffer? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

/*
 * linux_jack_port_name
 */
static NTSTATUS linux_jack_port_name(void *args)
{
    struct jack_port_name_params *params = args;
    jack_port_t *port = (jack_port_t*)params->port;
 
    params->name = jack_port_name(port);
    
    return params->name? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

/*
 * linux_jack_port_register
 */
static NTSTATUS linux_jack_port_register(void *args)
{
   struct jack_port_register_params *params = args;
   jack_client_t *client = (jack_client_t*)params->client;
   jack_port_t *port;

   port = jack_port_register(client, params->port_name, params->port_type, params->flags, params->buffer_size);
    
   params->port = (wine_jack_port_t*)port;

   return params->port? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

/*
 * linux_jack_free
 */
static NTSTATUS linux_jack_free(void *args)
{
    struct jack_free_params *params = args;
    jack_free(params->ptr);

    return _STATUS_SUCCESS;
}

/*
 * linux_run_pthread
 */
 struct run_thread_synchronizer {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    volatile int ready;
    pthread_t id;
};

static NTSTATUS linux_run_pthread(void *args)
{
    struct run_pthread_params *params = args;
    struct run_thread_synchronizer *sync = (struct run_thread_synchronizer*) params->synchronizer;
    void* (*function)(void*) = (void* (*)(void*)) params->function;

    pthread_mutex_lock(&sync->mutex);
    sync->id = pthread_self();
    sync->ready = 1;
    pthread_cond_signal(&sync->cond);
    pthread_mutex_unlock(&sync->mutex);

    function((void*)params->arg);
    
    return _STATUS_SUCCESS;
}

/*
 * linux_create_thread_callback
 *
 * Called by JACK when it wants a new thread. See Threads in README.md for an explanation.
 */
static int linux_create_thread_callback(pthread_t* id, const pthread_attr_t* attr, void* (*function)(void*), void* arg)
{
    struct run_thread_synchronizer sync;
    struct pe_create_thread_callback_params params = {
        .dispatch = {.callback = pe_create_thread_callback},
        .synchronizer = (uint64_t) &sync,
        .function = (uint64_t) function,
        .arg = (uint64_t) arg,
        .realtime = FALSE,
    };
    struct dispatch_callback_params* dispatch = (struct dispatch_callback_params*) &params.dispatch;
    void *ret_ptr = NULL;
    ULONG ret_len = 0;
    NTSTATUS status;
    int retval;

    if (attr)
    {
        int policy = 0;
        if (pthread_attr_getschedpolicy(attr, &policy) != 0) {
            if (policy == SCHED_FIFO || policy == SCHED_RR)
            params.realtime = TRUE;
        }
    }
    // note: other attributes set in JACK for its new threads are ignored
    //  - pthread_attr_setdetachstate(PTHREAD_CREATE_JOINABLE): windows threads are all joinable
    //  - pthread_attr_setscope(PTHREAD_SCOPE_SYSTEM): true for all windows threads 
    //  - pthread_attr_setinheritsched(PTHREAD_EXPLICIT_SCHED): we are setting priority explicitly
    //  - pthread_attr_setstacksize(THREAD_STACK): can't see why this matters

    pthread_mutex_init(&sync.mutex, NULL);
    pthread_cond_init(&sync.cond, NULL);
    sync.ready = 0;

    TRACE("function=%lx, arg=%lx, realtime=%d - creating\n", params.function, params.arg, params.realtime);

    // Create the thread on the PE side
    status = KeUserDispatchCallback(dispatch, sizeof(params), &ret_ptr, &ret_len);

    if (status == 0) {
        // wait on linux_run_pthread which is called from the PE thread
        pthread_mutex_lock(&sync.mutex);
        while (!sync.ready)
            pthread_cond_wait(&sync.cond, &sync.mutex);
        pthread_mutex_unlock(&sync.mutex);

        *id = sync.id;
        retval = 0;
    }
    else
        retval = -1;

    pthread_mutex_destroy(&sync.mutex);
    pthread_cond_destroy(&sync.cond);

    TRACE("function=%lx, arg=%lx - created\n", params.function, params.arg);

    return retval;
}

/*
 * Unixlib function table - must match enum wine_jack_func_ids order
 */
const unixlib_entry_t __wine_unix_call_funcs[] =
{
    linux_process_attach,

    linux_jack_activate,
    linux_jack_client_close,
    linux_jack_client_open,
    linux_jack_deactivate,
    linux_jack_get_ports,
    linux_jack_get_sample_rate,
    linux_jack_on_shutdown,
    linux_jack_set_buffer_size_callback,
    linux_jack_set_process_callback,
    linux_jack_set_sample_rate_callback,

    linux_jack_connect,
    linux_jack_disconnect,
    linux_jack_port_get_buffer,
    linux_jack_port_name,
    linux_jack_port_register,

    linux_jack_free,
    linux_run_pthread
};

C_ASSERT(ARRAYSIZE(__wine_unix_call_funcs) == wine_jack_funcs_count);
