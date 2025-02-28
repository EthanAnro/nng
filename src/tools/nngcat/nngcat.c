//
// Copyright 2025 Staysail Systems, Inc. <info@staysail.tech>
// Copyright 2018 Capitar IT Group BV <info@capitar.com>
// Copyright 2020 Lager Data, Inc. <support@lagerdata.com>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nng/args.h>
#include <nng/nng.h>

// Globals.  We need this to avoid passing around everything.
int          format    = 0;
int          proto     = 0;
int          verbose   = 0;
int          delay     = 0;
nng_duration interval  = NNG_DURATION_INFINITE;
nng_duration sendtimeo = NNG_DURATION_INFINITE;
nng_duration recvtimeo = NNG_DURATION_INFINITE;
void        *data      = NULL;
size_t       datalen   = 0;
int          compat    = 0;
int          async     = 0;
int          insecure  = 0;
void        *cacert    = NULL;
size_t       cacertlen = 0;
void        *keyfile   = NULL;
size_t       keylen    = 0;
void        *certfile  = NULL;
size_t       certlen   = 0;
int          count     = 0;
int          recvmaxsz = -1;

// Options, must start at 1 because zero is sentinel.
enum options {
	OPT_HELP = 1,
	OPT_VERBOSE,
	OPT_SILENT,
	OPT_REP0,
	OPT_REQ0,
	OPT_PUSH0,
	OPT_PULL0,
	OPT_PUB0,
	OPT_SUB0,
	OPT_SURVEY0,
	OPT_RESPOND0,
	OPT_PAIR0,
	OPT_PAIR1,
	OPT_PAIR,
	OPT_BUS0,
	OPT_DIAL,
	OPT_LISTEN,
	OPT_DATA,
	OPT_FILE,
	OPT_SUBSCRIBE,
	OPT_INTERVAL,
	OPT_DELAY,
	OPT_COUNT,
	OPT_FORMAT,
	OPT_RAW,
	OPT_ASCII,
	OPT_QUOTED,
	OPT_MSGPACK,
	OPT_HEX,
	OPT_BLANK, // no printing, not an actual option.
	OPT_COMPAT,
	OPT_ASYNC,
	OPT_RCV_TIMEO,
	OPT_SND_TIMEO,
	OPT_SOCK_NAME,
	OPT_LISTEN_IPC,
	OPT_DIAL_IPC,
	OPT_LISTEN_LOCAL,
	OPT_DIAL_LOCAL,
	OPT_INSECURE,
	OPT_CACERT,
	OPT_KEYFILE,
	OPT_CERTFILE,
	OPT_VERSION,
	OPT_RECVMAXSZ,
};

static nng_arg_spec opts[] = {
	{ .a_name = "help", .a_short = 'h', .a_val = OPT_HELP },
	{ .a_name = "verbose", .a_short = 'v', .a_val = OPT_VERBOSE },
	{ .a_name = "silent", .a_short = 'q', .a_val = OPT_SILENT },
	{ .a_name = "req0", .a_val = OPT_REQ0 },
	{ .a_name = "rep0", .a_val = OPT_REP0 },
	{ .a_name = "push0", .a_val = OPT_PUSH0 },
	{ .a_name = "pull0", .a_val = OPT_PULL0 },
	{ .a_name = "pub0", .a_val = OPT_PUB0 },
	{ .a_name = "sub", .a_val = OPT_SUB0 }, // explicit for alias
	{ .a_name = "sub0", .a_val = OPT_SUB0 },
	{ .a_name = "surveyor0", .a_val = OPT_SURVEY0 },
	{ .a_name = "respondent0", .a_val = OPT_RESPOND0 },
	{ .a_name = "pair", .a_val = OPT_PAIR },
	{ .a_name = "pair0", .a_val = OPT_PAIR0 },
	{ .a_name = "pair1", .a_val = OPT_PAIR1 },
	{ .a_name = "bus0", .a_val = OPT_BUS0 },
	{ .a_name = "dial", .a_val = OPT_DIAL, .a_arg = true },
	{ .a_name = "listen", .a_val = OPT_LISTEN, .a_arg = true },
	{ .a_name = "data", .a_short = 'D', .a_val = OPT_DATA, .a_arg = true },
	{ .a_name = "file", .a_short = 'F', .a_val = OPT_FILE, .a_arg = true },
	{ .a_name = "subscribe", .a_val = OPT_SUBSCRIBE, .a_arg = true },
	{ .a_name = "format", .a_val = OPT_FORMAT, .a_arg = true },
	{ .a_name = "ascii", .a_short = 'A', .a_val = OPT_ASCII },
	{ .a_name = "quoted", .a_short = 'Q', .a_val = OPT_QUOTED },
	{ .a_name = "raw", .a_val = OPT_RAW },
	{ .a_name = "hex", .a_val = OPT_HEX },
	{ .a_name = "compat", .a_val = OPT_COMPAT },
	{ .a_name = "async", .a_val = OPT_ASYNC },
	{ .a_name = "msgpack", .a_val = OPT_MSGPACK },

	{
	    .a_name  = "recv-maxsz",
	    .a_short = 'Z',
	    .a_val   = OPT_RECVMAXSZ,
	    .a_arg   = true,
	},
	{
	    .a_name  = "count",
	    .a_short = 'C',
	    .a_val   = OPT_COUNT,
	    .a_arg   = true,
	},
	{
	    .a_name  = "delay",
	    .a_short = 'd',
	    .a_val   = OPT_DELAY,
	    .a_arg   = true,
	},
	{
	    .a_name  = "interval",
	    .a_short = 'i',
	    .a_val   = OPT_INTERVAL,
	    .a_arg   = true,
	},
	{ .a_name = "recv-timeout", .a_val = OPT_RCV_TIMEO, .a_arg = true },
	{ .a_name = "send-timeout", .a_val = OPT_SND_TIMEO, .a_arg = true },
	{ .a_name = "socket-name", .a_val = OPT_SOCK_NAME, .a_arg = true },

	// Legacy compatibility with nanocat.
	{ .a_name = "bind", .a_val = OPT_LISTEN, .a_arg = true },
	{ .a_name = "connect", .a_val = OPT_DIAL, .a_arg = true },
	{
	    .a_name  = "bind-ipc",
	    .a_short = 'X',
	    .a_val   = OPT_LISTEN_IPC,
	    .a_arg   = true,
	},
	{
	    .a_name  = "connect-ipc",
	    .a_short = 'x',
	    .a_val   = OPT_DIAL_IPC,
	    .a_arg   = true,
	},
	{
	    .a_name  = "bind-local",
	    .a_short = 'L',
	    .a_val   = OPT_LISTEN_LOCAL,
	    .a_arg   = true,
	},
	{
	    .a_name  = "connect-local",
	    .a_short = 'l',
	    .a_val   = OPT_DIAL_LOCAL,
	    .a_arg   = true,
	},
	{ .a_name = "insecure", .a_short = 'k', .a_val = OPT_INSECURE },
	{ .a_name = "cacert", .a_val = OPT_CACERT, .a_arg = true },
	{ .a_name = "key", .a_val = OPT_KEYFILE, .a_arg = true },
	{
	    .a_name  = "cert",
	    .a_short = 'E',
	    .a_val   = OPT_CERTFILE,
	    .a_arg   = true,
	},
	{ .a_name = "version", .a_short = 'V', .a_val = OPT_VERSION },

	// Sentinel.
	{ .a_name = NULL, .a_val = 0 },
};

static void
fatal(const char *msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	exit(1);
}

static void
help(void)
{
	printf(
	    "Usage: nngcat <proto> <addr>... [<fmt>] [<opts>...] [<src>]\n\n");
	printf("<proto> must be one of:\n");
	printf("  --req0                 (or alias --req)\n");
	printf("  --rep0                 (or alias --rep)\n");
	printf("  --pub0                 (or alias --pub)\n");
	printf("  --sub0                 (or alias --sub)\n");
	printf("  --push0                (or alias --push)\n");
	printf("  --pull0                (or alias --pull)\n");
	printf("  --surveyor0            (or alias --surveyor)\n");
	printf("  --respondent0          (or alias --respondent)\n");
	printf("  --pair0\n");
	printf("  --pair1\n");
	printf("  --pair                 (alias for either pair0 or pair1)\n");
	printf("\n<addr> must be one or more of:\n");
	printf("  --dial <url>           (or alias --connect <url>)\n");
	printf("  --listen <url>         (or alias --bind <url>)\n");
	printf("  --bind-local <port>    (or alias -L <port>)\n");
	printf("  --connect-local <port> (or alias -l <port>)\n");
	printf("  --bind-ipc <path>      (or alias -X <path>)\n");
	printf("  --connect-ipc <path>   (or alias -x <path>)\n");
	printf("\n<fmt> may be one or none of:\n");
	printf("  --format <no|ascii|hex|msgpack|quoted|raw>\n");
	printf("  --ascii                (or alias -A)\n");
	printf("  --quoted               (or alias -Q)\n");
	printf("  --hex\n");
	printf("  --msgpack\n");
	printf("  --raw\n");
	printf("\n<opts> may be any of:\n");
	printf("  --subscribe <topic>    (only with --sub protocol)\n");
	printf("  --silent               (or alias -q)\n");
	printf("  --verbose              (or alias -v)\n");
	printf("  --count <num>          (or alias -C <num>)\n");
	printf("  --delay <secs>         (or alias -d <secs>)\n");
	printf("  --interval <secs>      (or alias -i <secs>)\n");
	printf("  --recv-timeout <secs>\n");
	printf("  --send-timeout <secs>\n");
	printf("  --recv-maxsz <size>    (or alias -Z <size>)\n");
	printf("  --compat\n");
	printf("  --async\n");
	printf("  --insecure             (or alias -k)\n");
	printf("  --cacert <file>\n");
	printf("  --cert <file>          (or alias -E <file>)\n");
	printf("  --key <file>\n");
	printf("\n<src> may be one of:\n");
	printf("  --file <file>          (or alias -F <file>). "
	       "Use - for standard input.\n");
	printf("  --data <data>          (or alias -D <data>)\n");
	exit(1);
}

static int
intarg(const char *val, int maxv)
{
	int v = 0;

	if (val[0] == '\0') {
		fatal("Empty integer argument.");
	}
	while (*val != '\0') {
		if (!isdigit(*val)) {
			fatal("Integer argument expected.");
		}
		v *= 10;
		v += ((*val) - '0');
		val++;
		if (v > maxv) {
			fatal("Integer argument too large.");
		}
	}
	if (v < 0) {
		fatal("Integer argument overflow.");
	}
	return (v);
}

// This reads a file into memory.  Care is taken to ensure that
// the buffer is one byte larger and contains a terminating
// NUL. (Useful for key files and such.)
static void
loadfile(const char *path, void **datap, size_t *lenp)
{
	FILE  *f;
	size_t total_read      = 0;
	size_t allocation_size = BUFSIZ;
	char  *fdata;
	char  *realloc_result;

	if (strcmp(path, "-") == 0) {
		f = stdin;
	} else {
		if ((f = fopen(path, "rb")) == NULL) {
			fatal(
			    "Cannot open file %s: %s", path, strerror(errno));
		}
	}

	if ((fdata = malloc(allocation_size + 1)) == NULL) {
		fatal("Out of memory.");
	}

	while (1) {
		total_read += fread(
		    fdata + total_read, 1, allocation_size - total_read, f);
		if (ferror(f)) {
			if (errno == EINTR) {
				continue;
			}
			fatal(
			    "Read from %s failed: %s", path, strerror(errno));
		}
		if (feof(f)) {
			break;
		}
		if (total_read == allocation_size) {
			if (allocation_size > SIZE_MAX / 2) {
				fatal("Out of memory.");
			}
			allocation_size *= 2;
			if ((realloc_result = realloc(
			         fdata, allocation_size + 1)) == NULL) {
				free(fdata);
				fatal("Out of memory.");
			}
			fdata = realloc_result;
		}
	}
	if (f != stdin) {
		fclose(f);
	}
	fdata[total_read] = '\0';
	*datap            = fdata;
	*lenp             = total_read;
}

static void
configtls(nng_tls_config *tls)
{
	int rv = 0;
	if (insecure) {
		rv = nng_tls_config_auth_mode(tls, NNG_TLS_AUTH_MODE_NONE);
	}
	if ((rv == 0) && (certfile != NULL)) {
		keyfile = keyfile ? keyfile : certfile;
		rv = nng_tls_config_own_cert(tls, certfile, keyfile, NULL);
	}
	if ((rv == 0) && (cacert != NULL)) {
		rv = nng_tls_config_ca_chain(tls, cacert, NULL);
	}
	if (rv != 0) {
		fatal("Unable to configure TLS: %s", nng_strerror(rv));
	}
}

struct addr {
	struct addr *next;
	int          mode;
	char        *val;
};

struct addr **
addaddr(struct addr **endp, int mode, const char *a)
{
	struct addr *na;

	if (((na = malloc(sizeof(*na))) == NULL) ||
	    ((na->val = malloc(strlen(a) + 1)) == NULL)) {
		fatal("Out of memory.");
	}
	na->mode = mode;
	memcpy(na->val, a, strlen(a) + 1);
	na->next = NULL;
	*endp    = na;
	return (&na->next);
}

struct topic {
	struct topic *next;
	char         *val;
};

struct topic **
addtopic(struct topic **endp, const char *s)
{
	struct topic *t;

	if (((t = malloc(sizeof(*t))) == NULL) ||
	    ((t->val = malloc(strlen(s) + 1)) == NULL)) {
		fatal("Out of memory.");
	}
	memcpy(t->val, s, strlen(s) + 1);
	t->next = NULL;
	*endp   = t;
	return (&t->next);
}

void
printmsg(char *buf, size_t len)
{
	switch (format) {
	case OPT_BLANK: // Suppress contents.
		return;
	case OPT_RAW: // Just emit raw contents.
		fwrite(buf, 1, len, stdout);
		break;
	case OPT_ASCII: // ASCII, but non-ASCII substituted with '.'
		for (size_t i = 0; i < len; i++) {
			if (isprint(buf[i])) {
				putchar(buf[i]);
			} else {
				putchar('.');
			}
		}
		break;
	case OPT_QUOTED: // C style quoted strings.
		putchar('"');
		for (size_t i = 0; i < len; i++) {
			switch (buf[i]) {
			case '\n':
				putchar('\\');
				putchar('n');
				break;
			case '\r':
				putchar('\\');
				putchar('r');
				break;
			case '\t':
				putchar('\\');
				putchar('t');
				break;
			case '"':
			case '\\':
				putchar('\\');
				putchar(buf[i]);
				break;
			default:
				if (isprint(buf[i])) {
					fputc(buf[i], stdout);
				} else {
					printf("\\x%02x", (uint8_t) buf[i]);
				}
			}
		}
		putchar('"');
		putchar('\n');
		break;
	case OPT_MSGPACK: // MsgPack, we just encode prefix + len, then
	                  // raw.
		if (len < 256) {
			putchar('\xc4');
			putchar(len & 0xff);
		} else if (len < 65536) {
			putchar('\xc5');
			putchar((len >> 8) & 0xff);
			putchar(len & 0xff);
		} else {
			putchar('\xc6');
			putchar((len >> 24) & 0xff);
			putchar((len >> 16) & 0xff);
			putchar((len >> 8) & 0xff);
			putchar(len & 0xff);
		}
		fwrite(buf, 1, len, stdout);
		break;
	case OPT_HEX: // Hex, quoted C string encoded with hex
	              // literals.
		putchar('"');
		for (size_t i = 0; i < len; i++) {
			printf("\\x%02x", (uint8_t) buf[i]);
		}
		putchar('"');
		putchar('\n');
		break;
	}
	fflush(stdout);
}

void
recvloop(nng_socket sock)
{
	int iters = 0;
	for (;;) {
		int      rv;
		nng_msg *msg;

		if ((count > 0) && (iters >= count)) {
			break;
		}
		rv = nng_recvmsg(sock, &msg, 0);
		iters++;
		switch (rv) {
		case NNG_ETIMEDOUT:
		case NNG_ESTATE:
			// Either a regular timeout, or we reached the
			// end of an event like a survey completing.
			return;
		case 0:
			printmsg(nng_msg_body(msg), nng_msg_len(msg));
			nng_msg_free(msg);
			continue;
		default:
			fatal("Receive error: %s", nng_strerror(rv));
		}
	}
}

void
resploop(nng_socket sock)
{
	int iters = 0;
	for (;;) {
		int      rv;
		nng_msg *msg;

		rv = nng_recvmsg(sock, &msg, 0);
		if (rv != 0) {
			fatal("Receive error: %s", nng_strerror(rv));
		}
		printmsg(nng_msg_body(msg), nng_msg_len(msg));
		nng_msg_clear(msg);
		if ((rv = nng_msg_append(msg, data, datalen)) != 0) {
			fatal("%s", nng_strerror(rv));
		}
		if ((rv = nng_sendmsg(sock, msg, 0)) != 0) {
			fatal("Send error: %s", nng_strerror(rv));
		}

		iters++;
		if ((count > 0) && (iters >= count)) {
			break;
		}
	}

	nng_msleep(200);
}

void
sendloop(nng_socket sock)
{
	int iters = 0;

	if (data == NULL) {
		fatal("No data to send (specify with --data or --file)");
	}
	if (delay > 0) {
		nng_msleep(delay);
	}

	for (;;) {
		int          rv;
		nng_msg     *msg;
		nng_time     start;
		nng_time     end;
		nng_duration delta;

		start = nng_clock();
		if (((rv = nng_msg_alloc(&msg, 0)) != 0) ||
		    ((rv = nng_msg_append(msg, data, datalen)) != 0)) {
			fatal("%s", nng_strerror(rv));
		}
		if ((rv = nng_sendmsg(sock, msg, 0)) != 0) {
			fatal("Send error: %s", nng_strerror(rv));
		}
		end   = nng_clock();
		delta = (nng_duration) (end - start);

		iters++;
		// By default, we don't loop.
		if (((interval < 0) && (count == 0)) ||
		    ((count > 0) && (iters >= count))) {
			break;
		}

		// We sleep, but we account for time spent, so that our
		// interval appears more or less constant.  Of course
		// if we took more than the interval here, then we skip
		// the sleep altogether.
		if ((delta >= 0) && (delta < interval)) {
			nng_msleep(interval - delta);
		}
	}
	// Wait a bit to give queues a chance to drain.
	nng_msleep(200);
}

void
sendrecv(nng_socket sock)
{
	int iters = 0;

	if (data == NULL) {
		fatal("No data to send (specify with --data or --file)");
	}
	if (delay > 0) {
		nng_msleep(delay);
	}

	// We start by sending a message, then we receive iteratively
	// but we set a concrete timeout if interval is set, to ensure
	// that we exit the receive loop, and can continue.
	for (;;) {
		int          rv;
		nng_msg     *msg;
		nng_time     start;
		nng_time     end;
		nng_duration delta;

		start = nng_clock();
		if (((rv = nng_msg_alloc(&msg, 0)) != 0) ||
		    ((rv = nng_msg_append(msg, data, datalen)) != 0)) {
			fatal("%s", nng_strerror(rv));
		}
		if ((rv = nng_sendmsg(sock, msg, 0)) != 0) {
			fatal("Send error: %s", nng_strerror(rv));
		}
		if ((interval < 0) && (count == 0)) {
			// Only one iteration through.
			recvloop(sock);
			break;
		}

		// We would like to use recvloop, but we need to reset
		// our timeout each time, as the timer counts down
		// towards zero.  Furthermore, with survey, we don't
		// want to increment the iteration count.

		for (;;) {
			delta = (nng_duration) (nng_clock() - start);

			nng_duration expire = interval - delta;
			if ((recvtimeo >= 0) && (expire > recvtimeo)) {
				expire = recvtimeo;
			}
			rv =
			    nng_socket_set_ms(sock, NNG_OPT_RECVTIMEO, expire);
			if (rv != 0) {
				fatal("Cannot set recv timeout: %s",
				    nng_strerror(rv));
			}

			rv = nng_recvmsg(sock, &msg, 0);
			switch (rv) {
			case 0:
				printmsg(nng_msg_body(msg), nng_msg_len(msg));
				nng_msg_free(msg);
				continue;
			case NNG_ETIMEDOUT:
			case NNG_ESTATE:
				// We're done receiving
				break;
			default:
				fatal("Cannot receive: %s", nng_strerror(rv));
				break;
			}

			// We're done receiving, break out.
			break;
		}

		end   = nng_clock();
		delta = (nng_duration) (end - start);

		iters++;
		if ((count > 0) && (iters >= count)) {
			break;
		}

		// We sleep, but we account for time spent, so that our
		// interval appears more or less constant.  Of course
		// if we took more than the interval here, then we skip
		// the sleep altogether.
		if ((delta >= 0) && (delta < interval)) {
			nng_msleep(interval - delta);
		}
	}
}

int
main(int ac, char **av)
{
	int            idx;
	char          *arg;
	int            val;
	int            rv;
	char           scratch[512];
	struct addr   *addrs = NULL;
	struct addr  **addrend;
	struct topic  *topics = NULL;
	struct topic **topicend;
	nng_socket     sock;
	int            port;

	idx      = 1;
	addrend  = &addrs;
	topicend = &topics;

	nng_init(NULL);
	atexit(nng_fini);

	while ((rv = nng_args_parse(ac, av, opts, &val, &arg, &idx)) == 0) {
		switch (val) {
		case OPT_HELP:
			help();
			break;
		case OPT_REQ0:
		case OPT_REP0:
		case OPT_SUB0:
		case OPT_PUB0:
		case OPT_BUS0:
		case OPT_SURVEY0:
		case OPT_RESPOND0:
		case OPT_PAIR0:
		case OPT_PAIR1:
		case OPT_PUSH0:
		case OPT_PULL0:
			if (proto != 0) {
				fatal("Only one protocol may be "
				      "specified.");
			}
			proto = val;
			break;
		case OPT_DIAL:
		case OPT_LISTEN:
			addrend = addaddr(addrend, val, arg);
			break;
		case OPT_DIAL_LOCAL:
		case OPT_LISTEN_LOCAL:
			port = intarg(arg, 65536);
			snprintf(scratch, sizeof(scratch),
			    "tcp://127.0.0.1:%d", port);
			addrend = addaddr(addrend, val, scratch);
			break;
		case OPT_DIAL_IPC:
		case OPT_LISTEN_IPC:
			snprintf(scratch, sizeof(scratch), "ipc:///%s", arg);
			addrend = addaddr(addrend, val, scratch);
			break;
		case OPT_COUNT:
			count = intarg(arg, 0x7fffffff);
			break;
		case OPT_SUBSCRIBE:
			topicend = addtopic(topicend, arg);
			break;
		case OPT_VERBOSE:
		case OPT_SILENT:
			verbose = val;
			break;
		case OPT_DELAY:
			delay = intarg(arg, 86400) * 1000; // max 1 day
			break;
		case OPT_INTERVAL:
			interval = intarg(arg, 86400) * 1000; // max 1 day
			break;
		case OPT_SND_TIMEO:
			sendtimeo = intarg(arg, 86400) * 1000; // max 1 day
			break;
		case OPT_RCV_TIMEO:
			recvtimeo = intarg(arg, 86400) * 1000; // max 1 day
			break;
		case OPT_RECVMAXSZ:
			recvmaxsz = intarg(arg, 0x7fffffff);
			if (recvmaxsz == 0) {
				recvmaxsz = 0x7fffffff;
			}
			break;
		case OPT_COMPAT:
			compat = 1;
			break;
		case OPT_ASYNC:
			async = NNG_FLAG_NONBLOCK;
			break;
		case OPT_ASCII:
		case OPT_RAW:
		case OPT_QUOTED:
		case OPT_MSGPACK:
		case OPT_HEX:
			if (format != 0) {
				fatal("Format may be specified only "
				      "once.");
			}
			format = val;
			break;
		case OPT_FORMAT:
			if (format != 0) {
				fatal("Format may be specified only "
				      "once.");
			}
			if (strcmp(arg, "no") == 0) {
				format = OPT_BLANK;
			} else if (strcmp(arg, "ascii") == 0) {
				format = OPT_ASCII;
			} else if (strcmp(arg, "hex") == 0) {
				format = OPT_HEX;
			} else if (strcmp(arg, "quoted") == 0) {
				format = OPT_QUOTED;
			} else if (strcmp(arg, "raw") == 0) {
				format = OPT_RAW;
			} else if (strcmp(arg, "msgpack") == 0) {
				format = OPT_MSGPACK;
			} else {
				fatal("Invalid format specified.");
			}
			break;
		case OPT_FILE:
			if (data != NULL) {
				fatal("Data (--file, --data) may be "
				      "specified "
				      "only once.");
			}
			loadfile(arg, &data, &datalen);
			break;
		case OPT_DATA:
			if (data != NULL) {
				fatal("Data (--file, --data) may be "
				      "specified "
				      "only once.");
			}
			if ((data = malloc(strlen(arg) + 1)) == NULL) {
				fatal("Out of memory.");
			}
			memcpy(data, arg, strlen(arg) + 1);
			datalen = strlen(arg);
			break;
		case OPT_CACERT:
			if (cacert != NULL) {
				fatal("CA Certificate (--cacert) may be "
				      "specified only once.");
			}
			loadfile(arg, &cacert, &cacertlen);
			break;
		case OPT_KEYFILE:
			if (keyfile != NULL) {
				fatal(
				    "Key (--key) may be specified only once.");
			}
			loadfile(arg, &keyfile, &keylen);
			break;
		case OPT_CERTFILE:
			if (certfile != NULL) {
				fatal("Cert (--cert) may be specified "
				      "only "
				      "once.");
			}
			loadfile(arg, &certfile, &certlen);
			break;
		case OPT_INSECURE:
			insecure = 1;
			break;
		case OPT_VERSION:
			printf("%s\n", nng_version());
			exit(0);
		}
	}
	switch (rv) {
	case NNG_ARG_INVAL:
		fatal("Option %s is invalid.", av[idx]);
		break;
	case NNG_ARG_AMBIG:
		fatal("Option %s is ambiguous (specify in full).", av[idx]);
		break;
	case NNG_ARG_MISSING:
		fatal("Option %s requires argument.", av[idx]);
		break;
	default:
		break;
	}

	if (addrs == NULL) {
		fatal("No address specified.");
	}

	if (compat) {
		if (async != 0) {
			fatal("Options --async and --compat are "
			      "incompatible.");
		}
		if (count != 0) {
			fatal("Options --count and --compat are "
			      "incompatible.");
		}
		if (proto == OPT_PAIR) {
			proto = OPT_PAIR0;
		}
		if (proto == OPT_PAIR1) {
			fatal("Protocol does not support --compat.");
		}
		async = NNG_FLAG_NONBLOCK;
	} else {
		if (proto == OPT_PAIR) {
			proto = OPT_PAIR1;
		}
	}
	if (proto == OPT_SUB0) {
		if (topics == NULL) {
			(void) addtopic(topicend, ""); // subscribe to all
		}
	} else {
		if (topics != NULL) {
			fatal("Protocol does not support --subscribe.");
		}
	}

	switch (proto) {
	case OPT_PULL0:
	case OPT_SUB0:
		if (data != NULL) {
			fatal("Protocol does not support --file or "
			      "--data.");
		}
		if (interval >= 0) {
			fatal("Protocol does not support --interval.");
		}
		break;
	case OPT_PUSH0:
	case OPT_PUB0:
		if (format != 0) {
			fatal("Protocol does not support --format "
			      "options.");
		}
		if (data == NULL) {
			fatal("Protocol requires either --file or "
			      "--data.");
		}
		break;
	case OPT_SURVEY0:
	case OPT_REQ0:
		if (data == NULL) {
			fatal("Protocol requires either --file or "
			      "--data.");
		}
		break;
	case OPT_REP0:
	case OPT_RESPOND0:
		if (interval >= 0) {
			fatal("Protocol does not support --interval.");
		}
		break;
	case OPT_PAIR0:
	case OPT_PAIR1:
	case OPT_BUS0:
		if ((interval >= 0) && (data == NULL)) {
			fatal("Option --interval requires either "
			      "--file or --data.");
		}
		break;
	default:
		// Will be caught in next switch statement.
		break;
	}

	switch (proto) {
	case OPT_REQ0:
#ifdef NNG_HAVE_REQ0
		rv = nng_req0_open(&sock);
#else
		fatal("Protocol not supported.");
#endif
		break;
	case OPT_REP0:
#ifdef NNG_HAVE_REP0
		rv = nng_rep0_open(&sock);
#else
		fatal("Protocol not supported.");
#endif
		break;
	case OPT_SUB0:
#ifdef NNG_HAVE_SUB0
		rv = nng_sub0_open(&sock);
#else
		fatal("Protocol not supported.");
#endif
		break;
	case OPT_PUB0:
#ifdef NNG_HAVE_PUB0
		rv = nng_pub0_open(&sock);
#else
		fatal("Protocol not supported.");
#endif
		break;
	case OPT_PAIR0:
#ifdef NNG_HAVE_PAIR0
		rv = nng_pair0_open(&sock);
#else
		fatal("Protocol not supported.");
#endif
		break;
	case OPT_PAIR1:
#ifdef NNG_HAVE_PAIR1
		rv = nng_pair1_open(&sock);
#else
		fatal("Protocol not supported");
#endif
		break;
	case OPT_BUS0:
#ifdef NNG_HAVE_BUS0
		rv = nng_bus0_open(&sock);
#else
		fatal("Protocol not supported.");
#endif
		break;
	case OPT_PUSH0:
#ifdef NNG_HAVE_PUSH0
		rv = nng_push0_open(&sock);
#else
		fatal("Protocol not supported.");
#endif
		break;
	case OPT_PULL0:
#ifdef NNG_HAVE_PULL0
		rv = nng_pull0_open(&sock);
#else
		fatal("Protocol not supported.");
#endif
		break;
	case OPT_SURVEY0:
#ifdef NNG_HAVE_SURVEYOR0
		rv = nng_surveyor0_open(&sock);
#else
		fatal("Protocol not supported.");
#endif
		break;
	case OPT_RESPOND0:
#ifdef NNG_HAVE_RESPONDENT0
		rv = nng_respondent0_open(&sock);
#else
		fatal("Protocol not supported");
#endif
		break;
	case 0:
	default:
		fatal("No protocol specified.");
		break;
	}
	if (rv != 0) {
		fatal("Unable to open socket: %s", nng_strerror(rv));
	}

	for (struct topic *t = topics; t != NULL; t = t->next) {
		rv = nng_sub0_socket_subscribe(sock, t->val, strlen(t->val));
		if (rv != 0) {
			fatal("Unable to subscribe to topic %s: %s", t->val,
			    nng_strerror(rv));
		}
	}

	if ((sendtimeo > 0) &&
	    ((rv = nng_socket_set_ms(sock, NNG_OPT_SENDTIMEO, sendtimeo)) !=
	        0)) {
		fatal("Unable to set send timeout: %s", nng_strerror(rv));
	}
	if ((recvtimeo > 0) &&
	    ((rv = nng_socket_set_ms(sock, NNG_OPT_RECVTIMEO, recvtimeo)) !=
	        0)) {
		fatal("Unable to set send timeout: %s", nng_strerror(rv));
	}

	if ((recvmaxsz >= 0) &&
	    ((rv = nng_socket_set_size(sock, NNG_OPT_RECVMAXSZ, recvmaxsz)) !=
	        0)) {
		fatal("Unable to set max receive size: %s", nng_strerror(rv));
	}

	for (struct addr *a = addrs; a != NULL; a = a->next) {
		char           *act;
		nng_listener    l;
		nng_dialer      d;
		nng_tls_config *tls;
		switch (a->mode) {
		case OPT_DIAL:
		case OPT_DIAL_IPC:
		case OPT_DIAL_LOCAL:
			rv = nng_dialer_create(&d, sock, a->val);
			if (rv != 0) {
				fatal("Unable to create dialer for %s: %s",
				    a->val, nng_strerror(rv));
			}
			rv = nng_dialer_get_tls(d, &tls);
			if (rv == 0) {
				configtls(tls);
			} else if (rv != NNG_ENOTSUP) {
				fatal("Unable to get TLS config: %s",
				    nng_strerror(rv));
			}
			rv  = nng_dialer_start(d, async);
			act = "dial";
			if ((rv == 0) && (verbose == OPT_VERBOSE)) {
				char           us[NNG_MAXADDRSTRLEN];
				const nng_url *url;
				if (nng_dialer_get_url(d, &url) == 0) {
					nng_url_sprintf(us, sizeof(us), url);
					printf("Connected to: %s\n", us);
				}
			}
			break;
		case OPT_LISTEN:
		case OPT_LISTEN_IPC:
		case OPT_LISTEN_LOCAL:
			rv = nng_listener_create(&l, sock, a->val);
			if (rv != 0) {
				fatal("Unable to create listener for %s: %s",
				    a->val, nng_strerror(rv));
			}
			rv = nng_listener_get_tls(l, &tls);
			if (rv == 0) {
				configtls(tls);
			} else if (rv != NNG_ENOTSUP) {
				fatal("Unable to get TLS config: %s",
				    nng_strerror(rv));
			}
			rv  = nng_listener_start(l, async);
			act = "listen";
			if ((rv == 0) && (verbose == OPT_VERBOSE)) {
				const nng_url *url;
				char           us[NNG_MAXADDRSTRLEN];
				if (nng_listener_get_url(l, &url) == 0) {
					nng_url_sprintf(us, sizeof(us), url);
					printf("Listening at: %s\n", us);
				}
			}
			break;
		default:
			fatal("Invalid address mode! (Bug!)");
		}

		if (rv != 0) {
			fatal("Unable to %s on %s: %s", act, a->val,
			    nng_strerror(rv));
		}
	}

	switch (proto) {
	case OPT_SUB0:
	case OPT_PULL0:
		recvloop(sock);
		break;
	case OPT_REP0:
	case OPT_RESPOND0:
		if (data == NULL) {
			recvloop(sock);
		} else {
			resploop(sock);
		}
		break;
	case OPT_PUSH0:
	case OPT_PUB0:
		sendloop(sock);
		break;
	case OPT_BUS0:
	case OPT_PAIR0:
	case OPT_PAIR1:
		if (data != NULL) {
			sendrecv(sock);
		} else {
			recvloop(sock);
		}
		break;
	case OPT_REQ0:
	case OPT_SURVEY0:
		sendrecv(sock);
		break;
	default:
		fatal("Protocol handling unimplemented.");
	}

	exit(0);
}
