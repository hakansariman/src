/*-
 * Copyright (c) 1997 Brian Somers <brian@Awfulhak.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$Id: throughput.h,v 1.5 1998/06/12 20:12:26 brian Exp $
 */

#define SAMPLE_PERIOD 5

#define THROUGHPUT_OVERALL 0x0001
#define THROUGHPUT_CURRENT 0x0002
#define THROUGHPUT_PEAK    0x0004
#define THROUGHPUT_ALL     THROUGHPUT_OVERALL | THROUGHPUT_CURRENT \
                           | THROUGHPUT_PEAK	

struct pppThroughput {
  time_t uptime;
  u_long OctetsIn;
  u_long OctetsOut;
  u_long SampleOctets[SAMPLE_PERIOD];
  int OctetsPerSecond;
  int BestOctetsPerSecond;
  time_t BestOctetsPerSecondTime;
  int nSample;
  unsigned rolling : 1;
  struct pppTimer Timer;
};

extern void throughput_init(struct pppThroughput *);
extern void throughput_disp(struct pppThroughput *, struct prompt *);
extern void throughput_log(struct pppThroughput *, int, const char *);
extern void throughput_start(struct pppThroughput *, const char *, int);
extern void throughput_stop(struct pppThroughput *);
extern void throughput_addin(struct pppThroughput *, int);
extern void throughput_addout(struct pppThroughput *, int);
extern void throughput_clear(struct pppThroughput *, int, struct prompt *);
