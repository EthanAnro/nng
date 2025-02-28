//
// Copyright 2024 Staysail Systems, Inc. <info@staysail.tech>
// Copyright 2018 Capitar IT Group BV <info@capitar.com>
// Copyright 2019 Devolutions <info@devolutions.net>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#ifndef PLATFORM_POSIX_IPC_H
#define PLATFORM_POSIX_IPC_H

#include "core/nng_impl.h"
#include "core/stream.h"

#ifdef NNG_PLATFORM_POSIX
#include "posix_aio.h"

#include <sys/types.h> // For mode_t

struct nni_ipc_conn {
	nng_stream      stream;
	nni_posix_pfd   pfd;
	nni_list        readq;
	nni_list        writeq;
	bool            closed;
	nni_mtx         mtx;
	nni_aio        *dial_aio;
	nni_ipc_dialer *dialer;
	nng_sockaddr    sa;
	nni_reap_node   reap;
};

struct nni_ipc_dialer {
	nng_stream_dialer sd;
	nni_list          connq; // pending connections
	bool              closed;
	nni_mtx           mtx;
	nng_sockaddr      sa;
	nni_refcnt        ref;
#ifdef NNG_TEST_LIB
	bool no_connect; // don't actually connect, for testing cancellation,
#endif
};

extern int nni_posix_ipc_alloc(
    nni_ipc_conn **, nni_sockaddr *, nni_ipc_dialer *, int);
extern void nni_posix_ipc_dialer_rele(nni_ipc_dialer *);
extern void nni_posix_ipc_dialer_cb(void *arg, unsigned events);

#endif // NNG_PLATFORM_POSIX

#endif // PLATFORM_POSIX_IPC_H
