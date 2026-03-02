/*
 * Wine JACK driver - Public API header
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

#ifndef __WINEJACK_H
#define __WINEJACK_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handles */
typedef void jack_client_t;
typedef void jack_port_t;

/* JACK options from jack/types.h */
enum JackOptions {
    JackNullOption      = 0x00,
    JackNoStartServer   = 0x01,
    JackUseExactName    = 0x02,
    // TODO: optional jack_client_open args are NYI
    JackServerName      = 0x04,
    JackLoadName        = 0x08,
    JackLoadInit        = 0x10,
    JackSessionID       = 0x20
};
typedef enum JackOptions jack_options_t;

/* JACK status bits from jack/types.h */
enum JackStatus {
    JackFailure         = 0x01,
    JackInvalidOption   = 0x02,
    JackNameNotUnique   = 0x04,
    JackServerStarted   = 0x08,
    JackServerFailed    = 0x10,
    JackServerError     = 0x20,
    JackNoSuchClient    = 0x40,
    JackLoadFailure     = 0x80,
    JackInitFailure     = 0x100,
    JackShmFailure      = 0x200,
    JackVersionError    = 0x400,
    JackBackendError    = 0x800,
    JackClientZombie    = 0x1000
};
typedef enum JackStatus jack_status_t;

/* JACK port flags from jack/types.h */
enum JackPortFlags {
    JackPortIsInput = 0x1,
    JackPortIsOutput = 0x2,
    JackPortIsPhysical = 0x4,
    JackPortCanMonitor = 0x8,
    JackPortIsTerminal = 0x10,
};

typedef uint32_t jack_nframes_t;
typedef float jack_default_audio_sample_t;
typedef int (*JackProcessCallback)(jack_nframes_t nframes, void *arg);
typedef int (*JackSampleRateCallback)(jack_nframes_t nframes, void *arg);
typedef int (*JackBufferSizeCallback)(jack_nframes_t nframes, void *arg);
typedef void (*JackShutdownCallback)(void *arg);

#define JACK_DEFAULT_AUDIO_TYPE "32 bit float mono audio"

/*
 * JACK client interface - see jack.h
 */
int             jack_activate (jack_client_t* client);
int             jack_client_close(jack_client_t* client);
int             jack_deactivate (jack_client_t* client);
jack_client_t*  jack_client_open(const char* client_name, jack_options_t options, jack_status_t* status);
char*           jack_get_client_name(jack_client_t* client);
const char**    jack_get_ports(jack_client_t* client, const char *port_name_pattern,
                            const char *type_name_pattern, uint64_t flags);
jack_nframes_t  jack_get_sample_rate (jack_client_t* client);
void            jack_on_shutdown(jack_client_t* client, JackShutdownCallback callback, void* arg);
int             jack_set_buffer_size_callback(jack_client_t* client, JackBufferSizeCallback callback, void* arg);
int             jack_set_process_callback(jack_client_t* client, JackProcessCallback callback, void* arg);
int             jack_set_sample_rate_callback(jack_client_t* client, JackSampleRateCallback callback, void* arg);
jack_nframes_t  jack_last_frame_time(const jack_client_t* client);

/*
 * JACK port interface - see jack.h
 */
int             jack_connect(jack_client_t* client, const char *source_port, const char *destination_port);
int             jack_disconnect(jack_client_t* client, const char *source_port, const char *destination_port);
void*           jack_port_get_buffer (jack_port_t *port, jack_nframes_t);
const char*     jack_port_name(const jack_port_t* port);
jack_port_t*    jack_port_register(jack_client_t* client, const char* port_name, const char* port_type,
                                   uint64_t flags, uint64_t buffer_size);

void            jack_free(void* ptr);

#ifdef __cplusplus
}
#endif

#endif /* __WINEJACK_H */
