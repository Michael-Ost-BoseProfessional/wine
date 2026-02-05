/*
 * Wine JACK unixlib - Define unixlib thunking for both PE- and Linux-side implementations 
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

/* Opaque handles for JACK objects */
typedef void wine_jack_client_t;
typedef void wine_jack_port_t;
/* Int types */
typedef uint32_t wine_jack_options_t;
typedef uint32_t wine_jack_status_t;
typedef uint32_t wine_jack_nframes_t;

/* Parameter structures for each function */

struct process_attach_params
{
    uint64_t pe_buffer_size_callback;
    uint64_t pe_create_thread_callback;
    uint64_t pe_process_callback;
    uint64_t pe_sample_rate_callback;
    uint64_t pe_shutdown_callback;
};

struct jack_activate_params
{
    wine_jack_client_t* client;

    /* outputs */
    int32_t result;
};

struct jack_client_close_params
{
    wine_jack_client_t* client;

    /* outputs */
    int32_t result;
};    

struct jack_deactivate_params
{
    wine_jack_client_t* client;

    /* outputs */
    int32_t result;
};

struct jack_client_open_params
{
    const char* client_name;
    wine_jack_options_t options;

    /* outputs */
    wine_jack_status_t status;
    wine_jack_client_t* client;
};        

struct jack_get_ports_params
{
    wine_jack_client_t* client;
    const char* port_name_pattern;
    const char* type_name_pattern;
    uint64_t flags;
    
    /* outputs */
    const char** ports_buffer;
};

struct jack_get_sample_rate_params
{
    wine_jack_client_t* client;

    /* outputs */
    wine_jack_nframes_t sample_rate;
};

struct jack_on_shutdown_params
{
    wine_jack_client_t* client;
    void* pe_callback;
    void* arg;

    /* outputs */
};

struct jack_set_buffer_size_callback_params
{
    wine_jack_client_t* client;
    void* pe_callback;
    void* arg;

    /* outputs */
    int result;
};

struct jack_set_process_callback_params
{
    wine_jack_client_t* client;
    void* pe_callback;
    void* arg;

    /* outputs */
    int result;
};

struct jack_set_sample_rate_callback_params
{
    wine_jack_client_t* client;
    void* pe_callback;
    void* arg;

    /* outputs */
    int result;
};

struct jack_connect_params
{
    wine_jack_client_t* client;
    const char* source_port;
    const char* destination_port;

    /* outputs */
    int32_t result;
};

struct jack_disconnect_params
{
    wine_jack_client_t* client;
    const char* source_port;
    const char* destination_port;

    /* outputs */
    int32_t result;
};

struct jack_port_get_buffer_params
{
    wine_jack_port_t* port;
    wine_jack_nframes_t nframes;

    /* outputs */
    void* buffer;
};

struct jack_port_name_params
{
    wine_jack_port_t* port;
 
    /* outputs */
    const char* name;
};

struct jack_port_register_params
{
    wine_jack_client_t* client;
    const char* port_name;
    const char* port_type;
    uint64_t flags;
    uint64_t buffer_size;

    /* outputs */
    wine_jack_port_t* port;
};

struct jack_free_params
{
    void* ptr;
};

struct run_pthread_params
{
    /* see pe_create_thread_callback_params */
    uint64_t synchronizer;
    uint64_t function;
    uint64_t arg;
};

// See ntuser.h (which is difficult to include here)
struct _dispatch_callback_params {
    uint64_t callback;
};

struct pe_create_thread_callback_params
{
    struct _dispatch_callback_params dispatch;

    uint64_t synchronizer;  /* communicate between callbacks */
    uint64_t function;      /* pthread style thread function */
    uint64_t arg;           /* opaque argument to 'function' */

    int realtime;
};

/* params for process, sample_rate, and buffer_size callbacks */
struct pe_callback_nframes_arg_params
{
    struct _dispatch_callback_params dispatch;

    uint64_t pe_callback;
    wine_jack_nframes_t nframes;
    void* arg;

    /* output */
    int32_t result;
};

struct pe_shutdown_callback_params
{
    struct _dispatch_callback_params dispatch;

    uint64_t pe_callback;
    void* arg;
};

/* Function enumeration - must match __wine_unix_call_funcs order */
enum wine_jack_func_ids
{
    process_attach_id,

    jack_activate_id,
    jack_client_close_id,
    jack_client_open_id,
    jack_deactivate_id,
    jack_get_ports_id,
    jack_get_sample_rate_id,
    jack_on_shutdown_id,
    jack_set_buffer_size_callback_id,
    jack_set_process_callback_id,
    jack_set_sample_rate_callback_id,

    jack_connect_id,
    jack_disconnect_id,
    jack_port_get_buffer_id,
    jack_port_name_id,
    jack_port_register_id,

    jack_free_id,
    run_pthread_id,

    wine_jack_funcs_count
};

#endif /* __WINEJACK_UNIXLIB_H */
