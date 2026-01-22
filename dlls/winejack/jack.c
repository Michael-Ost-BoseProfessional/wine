/*
 * Wine JACK unixlib - Exposes JACK API to Windows/PE side
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

#include <errno.h>
#include <string.h>

#include <jack/jack.h>

WINE_DEFAULT_DEBUG_CHANNEL(jack);

static jack_client_t *to_jack_client_t(wine_jack_client_t h)
{
    return (jack_client_t *)(UINT_PTR)h;
}

static wine_jack_client_t to_wine_jack_client_t(jack_client_t *client)
{
    return (wine_jack_client_t)(UINT_PTR)client;
}

#if LATER
static jack_port_t *to_jack_port_t(wine_jack_port_t h)
{
    return (jack_port_t *)(UINT_PTR)h;
}

static wine_jack_port_t to_wine_jack_port_t(jack_port_t *port)
{
    return (wine_jack_port_t)(UINT_PTR)port;
}

static void safe_strncpy(char* dest, const char* src, size_t destSize)
{
    int copyCount;
    if (destSize == 0)
        return;

    copyCount = 0;
    while (*src) {
        if (++copyCount == destSize)
            break;
        *dest++ = *src++;
    }
    *dest = '\0';
}
#endif

/*
 * unix_jack_client_open
 */
static NTSTATUS unix_jack_client_open(void *args)
{
    struct jack_client_open_params *params = args;
    jack_status_t status;
    jack_client_t *client;

    TRACE("name=%s, options=0x%x\n", params->client_name, params->options);

    client = jack_client_open(params->client_name, params->options, &status);

    params->status = status;
    params->client = to_wine_jack_client_t(client);

    return params->client? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

/*
 * unix_jack_client_close
 */
static NTSTATUS unix_jack_client_close(void *args)
{
    struct jack_client_close_params *params = args;
    jack_client_t *client = to_jack_client_t(params->client);

    TRACE("client=%p\n", client);

    params->result = jack_client_close(client);

    return params->result == 0? _STATUS_SUCCESS : _STATUS_INVALID_PARAMETER;
}

#if LATER
/*
 * unix_jack_activate
 */
static NTSTATUS unix_jack_activate(void *args)
{
    struct jack_activate_params *params = args;
    jack_client_t *client = to_jack_client_t(params->client);

    TRACE("client=%p\n", client);

    params->result = jack_activate(client);

    return params->result == 0? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
}

/*
 * unix_jack_deactivate
 */
static NTSTATUS unix_jack_deactivate(void *args)
{
    struct jack_deactivate_params *params = args;
    jack_client_t *client = to_jack_client_t(params->client);

    TRACE("client=%p\n", client);

    params->result = jack_deactivate(client);

    return params->result == 0? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
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

    return params->result == 0? STATUS_SUCCESS : 
           params->result == EEXIST? STATUS_ALREADY_INITIALIZED : 
           STATUS_INVALID_PARAMETER;
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

    return params->result == 0? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
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

    return params->port? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
}

/*
 * unix_jack_port_unregister
 */
static NTSTATUS unix_jack_port_unregister(void *args)
{
    struct jack_port_unregister_params *params = args;
    jack_client_t *client = to_jack_client_t(params->client);
    jack_port_t *port = to_jack_port_t(params->port);

    TRACE("client=%p, port=%p\n", client, port);

    params->result = jack_port_unregister(client, port);

    return params->result == 0? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
}

/*
 * unix_jack_port_name
 */
static NTSTATUS unix_jack_port_name(void *args)
{
    struct jack_port_name_params *params = args;
    jack_port_t *port = to_jack_port_t(params->port);
    const char *name;

    name = jack_port_name(port);

    if (name && params->buffer && params->buffer_size > 0) {
        safe_strncpy(params->buffer, name, params->buffer_size);
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

/*
 * unix_jack_get_ports
 */
static NTSTATUS unix_jack_get_ports(void *args)
{
    struct jack_get_ports_params *params = args;
    jack_client_t *client = to_jack_client_t(params->client);
    const char **ports;
    unsigned int i, count;

    TRACE("client=%p, name_pattern=%s, type_pattern=%s, flags=0x%lx\n",
          client, params->port_name_pattern, params->type_name_pattern,
          params->flags);

    ports = jack_get_ports(client, params->port_name_pattern,
                            params->type_name_pattern, params->flags);

    if (!ports) {
        params->count = 0;
        return STATUS_INVALID_PARAMETER;
    }

    /* Count ports */
    for (count = 0; ports[count]; count++)
        ;

    params->count = count;

    /* Copy port names to output buffer if provided */
    if (params->ports_buffer && params->buffer_size > 0) {
        char *ptr = params->ports_buffer;
        size_t remaining = params->buffer_size;

        for (i = 0; i < count && remaining > 1; i++) {
            size_t len = strlen(ports[i]) + 1;
            if (len > remaining)
                break;
            memcpy(ptr, ports[i], len);
            ptr += len;
            remaining -= len;
        }
        /* Null terminator for the list */
        if (remaining > 0)
            *ptr = '\0';
    }

    jack_free(ports);

    return STATUS_SUCCESS;
}

/*
 * unix_jack_port_by_name
 */
static NTSTATUS unix_jack_port_by_name(void *args)
{
    struct jack_port_by_name_params *params = args;
    jack_client_t *client = to_jack_client_t(params->client);
    jack_port_t *port;

    TRACE("client=%p, name=%s\n", client, params->port_name);

    port = jack_port_by_name(client, params->port_name);
    params->port = to_wine_jack_port_t(port);

    return params->port? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
}

/*
 * unix_jack_get_sample_rate
 */
static NTSTATUS unix_jack_get_sample_rate(void *args)
{
    struct jack_get_sample_rate_params *params = args;
    jack_client_t *client = to_jack_client_t(params->client);

    params->sample_rate = jack_get_sample_rate(client);

    return STATUS_SUCCESS;
}

/*
 * unix_jack_get_buffer_size
 */
static NTSTATUS unix_jack_get_buffer_size(void *args)
{
    struct jack_get_buffer_size_params *params = args;
    jack_client_t *client = to_jack_client_t(params->client);

    params->buffer_size = jack_get_buffer_size(client);

    return STATUS_SUCCESS;
}

/*
 * unix_jack_get_client_name
 */
static NTSTATUS unix_jack_get_client_name(void *args)
{
    struct jack_get_client_name_params *params = args;
    jack_client_t *client = to_jack_client_t(params->client);
    const char *name;

    name = jack_get_client_name(client);

    if (name && params->buffer && params->buffer_size > 0) {
        safe_strncpy(params->buffer, name, params->buffer_size);
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}
#endif

/*
 * Unixlib function table
 */
const unixlib_entry_t __wine_unix_call_funcs[] =
{
    unix_jack_client_open,
    unix_jack_client_close,
#if LATER
    unix_jack_activate,
    unix_jack_deactivate,
    unix_jack_connect,
    unix_jack_disconnect,
    unix_jack_port_register,
    unix_jack_port_unregister,
    unix_jack_port_name,
    unix_jack_get_ports,
    unix_jack_port_by_name,
    unix_jack_get_sample_rate,
    unix_jack_get_buffer_size,
    unix_jack_get_client_name,
#endif
};

C_ASSERT(ARRAYSIZE(__wine_unix_call_funcs) == unix_jack_funcs_count);
