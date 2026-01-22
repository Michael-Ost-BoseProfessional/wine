/*
 * Wine JACK unixlib - Header for PE/Unix interface
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

#ifndef __WINEJACK_UNIXLIB_H
#define __WINEJACK_UNIXLIB_H

#include <stddef.h>
#include <stdint.h>

/* Opaque handles for JACK objects (pointers stored as uint64_t) */
typedef uint64_t wine_jack_client_t;
typedef uint64_t wine_jack_port_t;

/* Parameter structures for each function */

struct jack_client_open_params
{
    const char *client_name;
    int options;

    /* outputs */
    int status; /* jack_status_t */
    wine_jack_client_t client;
};

struct jack_client_close_params
{
    wine_jack_client_t client;

    /* outputs */
    int result;
};

#if LATER
struct jack_activate_params
{
    wine_jack_client_t client;

    /* outputs */
    int result;
};

struct jack_deactivate_params
{
    wine_jack_client_t client;

    /* outputs */
    int result;
};

struct jack_connect_params
{
    wine_jack_client_t client;
    const char *source_port;
    const char *destination_port;

    /* outputs */
    int result;
};

struct jack_disconnect_params
{
    wine_jack_client_t client;
    const char *source_port;
    const char *destination_port;

    /* outputs */
    int result;
};

struct jack_port_register_params
{
    wine_jack_client_t client;
    const char *port_name;
    const char *port_type;
    unsigned long flags;
    unsigned long buffer_size;

    /* outputs */
    wine_jack_port_t port;
};

struct jack_port_unregister_params
{
    wine_jack_client_t client;
    wine_jack_port_t port;

    /* outputs */
    int result;
};

struct jack_port_name_params
{
    wine_jack_port_t port;
    size_t buffer_size;

    /* outputs */
    char *buffer;
};

struct jack_get_ports_params
{
    wine_jack_client_t client;
    const char *port_name_pattern;
    const char *type_name_pattern;
    unsigned long flags;
    size_t buffer_size;
    
    /* outputs */
    char *ports_buffer; /* e.g. name1\0name2\0name3\0 */
    unsigned int count;
};

struct jack_port_by_name_params
{
    wine_jack_client_t client;
    const char *port_name;

    /* outputs */
    wine_jack_port_t port;
};

struct jack_get_sample_rate_params
{
    wine_jack_client_t client;

    /* outputs */
    UINT32 sample_rate;
};

struct jack_get_buffer_size_params
{
    wine_jack_client_t client;

    /* outputs */
    UINT32 buffer_size;
};

struct jack_get_client_name_params
{
    wine_jack_client_t client;
    size_t buffer_size;
    
    /* outputs */
    char *buffer;
};
#endif

/* Function enumeration - must match __wine_unix_call_funcs order */
enum wine_jack_func_ids
{
    jack_client_open_id,
    jack_client_close_id,
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
    unix_jack_funcs_count
};

#endif /* __WINEJACK_UNIXLIB_H */
