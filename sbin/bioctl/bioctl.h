/* $OpenBSD: bioctl.h,v 1.1 2005/04/04 17:36:46 marco Exp $       */
/*
 * Copyright (c) 2004, 2005 Marco Peereboom
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#ifndef _BIOCTL_H_
#define _BIOCTL_H_

#define READCAP 0x01
#define ENUM    0x02
#define TUR     0x04
#define INQUIRY 0x08

#define PARSELIST (0x8000000000000000llu)

struct read_cap {
	u_int32_t		maxlba;
	u_int32_t		bsize;
};

struct dev {
	SLIST_ENTRY(dev)	next;
	u_int16_t		id;
	u_int8_t		channel;
	u_int8_t		target;
	u_int64_t		capacity;
};

void		usage(void);
void		cleanup(void);
u_int64_t	parse_passthru(char *);
void		parse_devlist(char *);
void		print_sense(u_int8_t *, u_int8_t);

int		bio_get_capabilities(bioc_capabilities *);
void		bio_alarm(char *);
void		bio_ping(void);
void		bio_startstop(char *, u_int8_t, u_int8_t);
void		bio_status(void);
u_int64_t	bio_pt_readcap(u_int8_t, u_int8_t);
u_int32_t	bio_pt_inquire(u_int8_t, u_int8_t, u_int8_t *);
u_int32_t	bio_pt_tur(u_int8_t, u_int8_t);
void		bio_pt_enum(void);

#endif /* _BIOCTL_H_ */
