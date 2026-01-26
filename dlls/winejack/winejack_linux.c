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
#include "wine/unixlib.h"

#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"
#include "wine/unixlib.h"
// TODO: fix these includes
#define _STATUS_SUCCESS             ((NTSTATUS) 0x00000000)
#define _STATUS_INVALID_PARAMETER   ((NTSTATUS) 0xC000000D)
#define _STATUS_ALREADY_INITIALIZED ((NTSTATUS) 0xC0000228)

#include <errno.h>
#include <string.h>
#include <jack/jack.h>

// verify the size of same-named winejack types
C_ASSERT(sizeof(jack_client_t*) == sizeof(uint64_t));
C_ASSERT(sizeof(jack_options_t) == sizeof(uint32_t));
C_ASSERT(sizeof(jack_status_t) == sizeof(uint32_t));
C_ASSERT(sizeof(jack_nframes_t) == sizeof(uint32_t));

// This thunking layer passes char* pointers directly between PE and Unix code.
// This only works when both sides have the same pointer size (64-bit).
// If you need 32-bit PE support, all char* params must be changed to uint64_t and 
// strings must be copied between layers.
C_ASSERT(sizeof(char*) == sizeof(uint64_t));

WINE_DEFAULT_DEBUG_CHANNEL(jack);

static jack_client_t *to_jack_client_t(wine_jack_client_t h)
{
    return (jack_client_t *)(UINT_PTR)h;
}

static wine_jack_client_t to_wine_jack_client_t(jack_client_t *client)
{
    return (wine_jack_client_t)(UINT_PTR)client;
}

static jack_port_t *to_jack_port_t(wine_jack_port_t h)
{
    return (jack_port_t *)(UINT_PTR)h;
}

static wine_jack_port_t to_wine_jack_port_t(jack_port_t *port)
{
    return (wine_jack_port_t)(UINT_PTR)port;
}

/*
 * unix_jack_activate
 */
static NTSTATUS unix_jack_activate(void* args)
{
    struct jack_activate_params* params = args;
    jack_client_t* client = to_jack_client_t(params->client);

    TRACE("client=%p\n", client);

    params->result = jack_activate(client);

    return params->result == 0? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

/*
 * unix_jack_client_close
 */ 
static NTSTATUS unix_jack_client_close(void* args)
{
    struct jack_client_close_params* params = args;
    jack_client_t* client = to_jack_client_t(params->client);

    TRACE("client=%p\n", client);

    params->result = jack_client_close(client);

    return params->result == 0? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}    

/*
 * unix_jack_client_open
 */  
static NTSTATUS unix_jack_client_open(void* args)
{
    struct jack_client_open_params* params = args;
    jack_status_t status;
    jack_client_t* client;

    TRACE("name=%s, options=0x%x\n", params->client_name, params->options);

    client = jack_client_open(params->client_name, params->options, &status);

    params->status = status;
    params->client = to_wine_jack_client_t(client);

    return params->client? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}        

/*
 * unix_jack_get_ports
 */
static NTSTATUS unix_jack_get_ports(void *args)
{
    struct jack_get_ports_params *params = args;
    jack_client_t *client = to_jack_client_t(params->client);

    TRACE("client=%p, name_pattern=%s, type_pattern=%s, flags=0x%lx\n",
          client, params->port_name_pattern, params->type_name_pattern,
          params->flags);

    params->ports_buffer = jack_get_ports(client, params->port_name_pattern,
                                          params->type_name_pattern, params->flags);

    return params->ports_buffer? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

/*
 * unix_jack_get_sample_rate
 */
static NTSTATUS unix_jack_get_sample_rate(void *args)
{
    struct jack_get_sample_rate_params *params = args;
    jack_client_t *client = to_jack_client_t(params->client);

    params->sample_rate = jack_get_sample_rate(client);

    return _STATUS_SUCCESS;
}

/*
 * unix_jack_set_process_callback
 */
static NTSTATUS unix_jack_set_process_callback(void *args)
{
    struct jack_set_process_callback_params *params = args;
    jack_client_t *client = to_jack_client_t(params->client);

    params->result = jack_set_process_callback(client, params->process_callback, params->arg);

    return params->result == 0? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

/*
 * unix_jack_on_shutdown
 */
static NTSTATUS unix_jack_on_shutdown(void *args)
{
    struct jack_on_shutdown_params* params = args;
    jack_client_t *client = to_jack_client_t(params->client);

    jack_on_shutdown(client, params->shutdown_callback, params->arg);

    return _STATUS_SUCCESS;
}

/*
 * unix_jack_connect
 */
static NTSTATUS unix_jack_connect(void *args)
{
    struct jack_connect_params *params = args;
    jack_client_t *client = to_jack_client_t(params->client);

    TRACE("client=%p, src=%s, dst=%s\n", client,
          params->source_port, params->destination_port);

    params->result = jack_connect(client, params->source_port,
                                   params->destination_port);

    return params->result == 0? _STATUS_SUCCESS : 
           params->result == EEXIST? _STATUS_ALREADY_INITIALIZED : 
           _STATUS_INVALID_PARAMETER;
}

/*
 * unix_jack_disconnect
 */
static NTSTATUS unix_jack_disconnect(void *args)
{
    struct jack_disconnect_params *params = args;
    jack_client_t *client = to_jack_client_t(params->client);

    TRACE("client=%p, src=%s, dst=%s\n", client,
          params->source_port, params->destination_port);

    params->result = jack_disconnect(client, params->source_port,
                                      params->destination_port);

    return params->result == 0? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

/*
 * unix_jack_port_get_buffer
 */
static NTSTATUS unix_jack_port_get_buffer(void *args)
{
    struct jack_port_get_buffer_params* params = args;
    jack_port_t *port = to_jack_port_t(params->port);

    params->buffer = jack_port_get_buffer(port, params->nframes);

    return params->buffer == 0? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

/*
 * unix_jack_port_name
 */
static NTSTATUS unix_jack_port_name(void *args)
{
    struct jack_port_name_params *params = args;
    jack_port_t *port = to_jack_port_t(params->port);

    params->name = jack_port_name(port);

    return params->name == 0? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

/*
 * unix_jack_port_register
 */
static NTSTATUS unix_jack_port_register(void *args)
{
   struct jack_port_register_params *params = args;
   jack_client_t *client = to_jack_client_t(params->client);
   jack_port_t *port;

   TRACE("client=%p, name=%s, type=%s, flags=0x%lx, buffer_size=%lu\n",
         client, params->port_name, params->port_type,
         params->flags, params->buffer_size);

   port = jack_port_register(client, params->port_name, params->port_type,
                              params->flags, params->buffer_size);

   params->port = to_wine_jack_port_t(port);

   return params->port? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

/*
 * unix_jack_free
 */
static NTSTATUS unix_jack_free(void *args)
{
    struct jack_free_params *params = args;
    jack_free(params->ptr);

    return _STATUS_SUCCESS;
}

/*
 * Unixlib function table
 */
const unixlib_entry_t __wine_unix_call_funcs[] =
{
    unix_jack_activate,
    unix_jack_client_close,
    unix_jack_client_open,
    unix_jack_get_ports,
    unix_jack_get_sample_rate,
    unix_jack_set_process_callback,
    unix_jack_on_shutdown,

    unix_jack_connect,
    unix_jack_disconnect,
    unix_jack_port_get_buffer,
    unix_jack_port_name,
    unix_jack_port_register,

    unix_jack_free
};

C_ASSERT(ARRAYSIZE(__wine_unix_call_funcs) == wine_jack_funcs_count);
