//
// Copyright 2025 Staysail Systems, Inc. <info@staysail.tech>
// Copyright 2018 Capitar IT Group BV <info@capitar.com>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#ifndef NNG_SUPPLEMENTAL_HTTP_HTTP_MSG
#define NNG_SUPPLEMENTAL_HTTP_HTTP_MSG

#include "core/defs.h"
#include "core/list.h"

// Note that as we parse headers, the rule is that if a header is already
// present, then we can append it to the existing header, separated by
// a comma.  From experience, for example, Firefox uses a Connection:
// header with two values, "keepalive", and "upgrade".
typedef struct http_header {
	char         *name;
	char         *value;
	nni_list_node node;
	bool          static_name : 1;  // name is static, do not free it
	bool          static_value : 1; // value is static, do not free it
	bool          alloc_header : 1; // header is heap allocated
} http_header;
typedef struct http_header nni_http_header;

typedef struct nni_http_entity {
	char       *data;
	size_t      size;
	bool        own;        // if true, data is "ours", and should be freed
	char        clen[24];   // 64-bit lengths, in decimal
	char        ctype[128]; // 63+63+; per RFC 6838
	http_header content_type;
	http_header content_length;
	nni_list    hdrs;
	char       *buf;
	size_t      bufsz;
	bool        parsed;
} nni_http_entity;

struct nng_http_req {
	nni_http_entity data;
	char            meth[32];
	char            host[260]; // 253 per IETF, plus 6 for :port plus null
	char            ubuf[200]; // Most URIs are smaller than this
	char           *uri;
	const char     *vers;
	http_header     host_header;
};

struct nng_http_res {
	nni_http_entity data;
	uint16_t        code;
	char           *rsn;
	const char     *vers;
	bool            iserr;
	http_header     location;
};

extern void nni_http_free_header(http_header *);

#endif
