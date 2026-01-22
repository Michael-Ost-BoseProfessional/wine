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
typedef uint64_t jack_client_t;
#if LATER
typedef uint64_t jack_port_t;

/* JACK port flags from jack/types.h */
#define JackPortIsInput    0x1
#define JackPortIsOutput   0x2
#define JackPortIsPhysical 0x4
#define JackPortCanMonitor 0x8
#define JackPortIsTerminal 0x10

/* JACK options */
#define JackNullOption      0x00
#define JackNoStartServer  0x01
#define JackUseExactName   0x02
#define JackServerName      0x04
#define JackSessionID       0x20

/* JACK status flags */
#define JackFailure         0x01
#define JackInvalidOption   0x02
#define JackNameNotUnique 0x04
#define JackServerStarted 0x08
#define JackServerFailed  0x10
#define JackServerError   0x20
#define JackNoSuchClient 0x40
#define JackLoadFailure   0x80
#define JackInitFailure   0x100
#define JackShmFailure    0x200
#define JackVersionError  0x400

/* Default audio port type */
#define JACK_DEFAULT_AUDIO_TYPE "32 bit float mono audio"
#endif

/*
 * Client lifecycle
 */
jack_client_t jack_client_open(const char *client_name, int options, int *status);
int jack_client_close(jack_client_t client);
#if LATER
int wine_jack_activate(wine_jack_client_t client);
int wine_jack_deactivate(wine_jack_client_t client);

/*
 * Port connections
 */
int wine_jack_Connect(wine_jack_client_t client, const char *source_port, const char *destination_port);
int wine_jack_Disconnect(wine_jack_client_t client, const char *source_port, const char *destination_port);

/*
 * Port management
 */
wine_jack_port_t wine_jack_PortRegister(wine_jack_client_t client, const char *port_name,
                                    const char *port_type, unsigned long flags,
                                    unsigned long buffer_size);
int wine_jack_PortUnregister(wine_jack_client_t client, wine_jack_port_t port);
int wine_jack_PortName(wine_jack_port_t port, char *buffer, size_t buffer_size);
wine_jack_port_t wine_jack_PortByName(wine_jack_client_t client, const char *port_name);

/*
 * Port enumeration
 * Returns count of ports found. Port names are written to ports_buffer as
 * null-terminated strings: "name1\0name2\0name3\0\0"
 */
unsigned int wine_jack_GetPorts(wine_jack_client_t client, const char *port_name_pattern,
                                const char *type_name_pattern, unsigned long flags,
                                char *ports_buffer, size_t buffer_size);

/*
 * Server info
 */
uint32_t wine_jack_GetSampleRate(wine_jack_client_t client);
uint32_t wine_jack_GetBufferSize(wine_jack_client_t client);
int wine_jack_GetClientName(wine_jack_client_t client, char *buffer, size_t buffer_size);

#endif // LATER

#ifdef __cplusplus
}
#endif

#endif /* __WINEJACK_H */
