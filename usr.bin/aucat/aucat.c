/*	$OpenBSD: aucat.c,v 1.87 2010/04/24 06:18:23 ratchov Exp $	*/
/*
 * Copyright (c) 2008 Alexandre Ratchov <alex@caoua.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <varargs.h>

#include "abuf.h"
#include "aparams.h"
#include "aproc.h"
#include "conf.h"
#include "dev.h"
#include "listen.h"
#include "midi.h"
#include "opt.h"
#include "wav.h"
#ifdef DEBUG
#include "dbg.h"
#endif

/*
 * unprivileged user name
 */
#define SNDIO_USER	"_sndio"

/*
 * priority when run as root
 */
#define SNDIO_PRIO	(-20)

#define PROG_AUCAT	"aucat"
#define PROG_MIDICAT	"midicat"

#ifdef DEBUG
int debug_level = 0;
#endif
volatile int quit_flag = 0;

/*
 * SIGINT handler, it raises the quit flag. If the flag is already set,
 * that means that the last SIGINT was not handled, because the process
 * is blocked somewhere, so exit.
 */
void
sigint(int s)
{
	if (quit_flag)
		_exit(1);
	quit_flag = 1;
}

#ifdef DEBUG
/*
 * Increase debug level on SIGUSR1.
 */
void
sigusr1(int s)
{
	if (debug_level < 4)
		debug_level++;
}

/*
 * Decrease debug level on SIGUSR2.
 */
void
sigusr2(int s)
{
	if (debug_level > 0)
		debug_level--;
}
#endif

void
opt_ch(struct aparams *par)
{
	char *next, *end;
	long cmin, cmax;

	errno = 0;
	cmin = strtol(optarg, &next, 10);
	if (next == optarg || *next != ':')
		goto failed;
	if (errno == ERANGE && (cmin == LONG_MIN || cmin == LONG_MAX))
		goto failed;
	cmax = strtol(++next, &end, 10);
	if (end == next || *end != '\0')
		goto failed;
	if (errno == ERANGE && (cmax == LONG_MIN || cmax == LONG_MAX))
		goto failed;
	if (cmin < 0 || cmax < cmin || cmax > NCHAN_MAX)
		goto failed;
	par->cmin = cmin;
	par->cmax = cmax;
	return;
failed:
	errx(1, "%s: bad channel range", optarg);
}

void
opt_enc(struct aparams *par)
{
	int len;

	len = aparams_strtoenc(par, optarg);
	if (len == 0 || optarg[len] != '\0')
		errx(1, "%s: bad encoding", optarg);
}

int
opt_hdr(void)
{
	if (strcmp("auto", optarg) == 0)
		return HDR_AUTO;
	if (strcmp("raw", optarg) == 0)
		return HDR_RAW;
	if (strcmp("wav", optarg) == 0)
		return HDR_WAV;
	errx(1, "%s: bad header specification", optarg);
}

int
opt_mmc(void)
{
	if (strcmp("off", optarg) == 0)
		return 0;
	if (strcmp("slave", optarg) == 0)
		return 1;
	errx(1, "%s: bad MMC mode", optarg);
}

int
opt_join(void)
{
	if (strcmp("off", optarg) == 0)
		return 0;
	if (strcmp("on", optarg) == 0)
		return 1;
	errx(1, "%s: bad join/expand setting", optarg);
}

int
opt_xrun(void)
{
	if (strcmp("ignore", optarg) == 0)
		return XRUN_IGNORE;
	if (strcmp("sync", optarg) == 0)
		return XRUN_SYNC;
	if (strcmp("error", optarg) == 0)
		return XRUN_ERROR;
	errx(1, "%s: bad underrun/overrun policy", optarg);
}

unsigned
opt_mode(void)
{
	unsigned mode = 0;
	char *p = optarg;
	size_t len;

	for (p = optarg; *p != NULL; p++) {
		len = strcspn(p, ",");
		if (strncmp("play", p, len) == 0) {
			mode |= MODE_PLAY;
		} else if (strncmp("rec", p, len) == 0) {
			mode |= MODE_REC;
		} else if (strncmp("mon", p, len) == 0) {
			mode |= MODE_MON;
		} else if (strncmp("duplex", p, len) == 0) {
			/* XXX: backward compat, remove this */
			mode |= MODE_REC | MODE_PLAY;
		} else 
			errx(1, "%s: bad mode", optarg);
		p += len;
		if (*p == '\0')
			break;
	}
	if (mode == 0)
		errx(1, "empty mode");
	return mode;
}

/*
 * Arguments of -i, -o and -s options are stored in a list.
 */
struct farg {
	SLIST_ENTRY(farg) entry;
	struct aparams ipar;	/* input (read) parameters */
	struct aparams opar;	/* output (write) parameters */
	unsigned vol;		/* last requested volume */
	char *name;		/* optarg pointer (no need to copy it */
	int hdr;		/* header format */
	int xrun;		/* overrun/underrun policy */
	int mmc;		/* MMC mode */
	int join;		/* join/expand enabled */
	unsigned mode;
};

SLIST_HEAD(farglist, farg);

/*
 * Add a farg entry to the given list, corresponding
 * to the given file name.
 */
void
farg_add(struct farglist *list,
    struct aparams *ipar, struct aparams *opar, unsigned vol,
    int hdr, int xrun, int mmc, int join, unsigned mode, char *name)
{
	struct farg *fa;
	size_t namelen;

	fa = malloc(sizeof(struct farg));
	if (fa == NULL)
		err(1, "%s", name);

	if (hdr == HDR_AUTO) {
		if (name != NULL && (namelen = strlen(name)) >= 4 &&
		    strcasecmp(name + namelen - 4, ".wav") == 0) {
			fa->hdr = HDR_WAV;
		} else {
			fa->hdr = HDR_RAW;
		}
	} else
		fa->hdr = hdr;
	if (mmc && xrun == XRUN_IGNORE)
		xrun = XRUN_SYNC;
	fa->xrun = xrun;
	fa->ipar = *ipar;
	fa->opar = *opar;
	fa->vol = vol;
	fa->name = name; 
	fa->mmc = mmc;
	fa->join = join;
	fa->mode = mode;
	SLIST_INSERT_HEAD(list, fa, entry);
}

void
setsig(void)
{
	struct sigaction sa;

	quit_flag = 0;
	sigfillset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = sigint;
	if (sigaction(SIGINT, &sa, NULL) < 0)
		err(1, "sigaction(int) failed");
	if (sigaction(SIGTERM, &sa, NULL) < 0)
		err(1, "sigaction(term) failed");
	if (sigaction(SIGHUP, &sa, NULL) < 0)
		err(1, "sigaction(hup) failed");
#ifdef DEBUG
	sa.sa_handler = sigusr1;
	if (sigaction(SIGUSR1, &sa, NULL) < 0)
		err(1, "sigaction(usr1) failed");
	sa.sa_handler = sigusr2;
	if (sigaction(SIGUSR2, &sa, NULL) < 0)
		err(1, "sigaction(usr2) failed1n");
#endif
}

void
unsetsig(void)
{
	struct sigaction sa;

	sigfillset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = SIG_DFL;
#ifdef DEBUG
	if (sigaction(SIGUSR2, &sa, NULL) < 0)
		err(1, "unsetsig(usr2): sigaction failed");
	if (sigaction(SIGUSR1, &sa, NULL) < 0)
		err(1, "unsetsig(usr1): sigaction failed");
#endif
	if (sigaction(SIGHUP, &sa, NULL) < 0)
		err(1, "unsetsig(hup): sigaction failed\n");
	if (sigaction(SIGTERM, &sa, NULL) < 0)
		err(1, "unsetsig(term): sigaction failed\n");
	if (sigaction(SIGINT, &sa, NULL) < 0)
		err(1, "unsetsig(int): sigaction failed\n");
}

void
getbasepath(char *base, size_t size)
{
	uid_t uid;
	struct stat sb;
	mode_t mask;

	uid = geteuid();
	if (uid == 0) {
		mask = 022;
		snprintf(base, PATH_MAX, "/tmp/aucat");
	} else {
		mask = 077;
		snprintf(base, PATH_MAX, "/tmp/aucat-%u", uid);
	}
	if (mkdir(base, 0777 & ~mask) < 0) {
		if (errno != EEXIST)
			err(1, "mkdir(\"%s\")", base);
	}
	if (stat(base, &sb) < 0)
		err(1, "stat(\"%s\")", base);
	if (sb.st_uid != uid || (sb.st_mode & mask) != 0)
		errx(1, "%s has wrong permissions", base);
}

void
privdrop(void)
{
	struct passwd *pw;
	struct stat sb;

	if ((pw = getpwnam(SNDIO_USER)) == NULL)
		err(1, "getpwnam");
	if (stat(pw->pw_dir, &sb) < 0)
		err(1, "stat(\"%s\")", pw->pw_dir);
	if (sb.st_uid != 0 || (sb.st_mode & 022) != 0)
		errx(1, "%s has wrong permissions", pw->pw_dir);
	if (setpriority(PRIO_PROCESS, 0, SNDIO_PRIO) < 0)
		err(1, "setpriority");
	if (setgroups(1, &pw->pw_gid) ||
	    setresgid(pw->pw_gid, pw->pw_gid, pw->pw_gid) ||
	    setresuid(pw->pw_uid, pw->pw_uid, pw->pw_uid))
		err(1, "cannot drop privileges");
}

void
stopall(char *base)
{
	struct file *f;

  restart:
	LIST_FOREACH(f, &file_list, entry) {
		/*
		 * skip connected streams (handled by dev_done())
		 */
		if (APROC_OK(dev_mix) && f->rproc &&
		    aproc_depend(dev_mix, f->rproc))
			continue;
		if (APROC_OK(dev_sub) && f->wproc &&
		    aproc_depend(f->wproc, dev_sub))
			continue;
		if (APROC_OK(dev_midi)) {
			if (f->rproc && aproc_depend(dev_midi, f->rproc))
				continue;
			if (f->wproc && aproc_depend(f->wproc, dev_midi))
				continue;
		}
		/*
		 * kill anything else
		 */
		file_close(f);
		goto restart;
	}
}

void
aucat_usage(void)
{
	(void)fputs("usage: " PROG_AUCAT " [-dlnu] [-b nframes] "
	    "[-C min:max] [-c min:max] [-e enc]\n\t"
	    "[-f device] [-h fmt] [-i file] [-j flag] [-m mode]"
	    "[-o file] [-q device]\n\t"
	    "[-r rate] [-s name] [-t mode] [-U unit] "
	    "[-v volume] [-x policy]\n\t"
	    "[-z nframes]\n",
	    stderr);
}

int
aucat_main(int argc, char **argv)
{
	int c, u_flag, d_flag, l_flag, n_flag, hdr, xrun, unit;
	struct farg *fa;
	struct farglist ifiles, ofiles, sfiles, qfiles;
	struct aparams ipar, opar, dipar, dopar;
	char base[PATH_MAX], path[PATH_MAX], *file;
	unsigned bufsz, round, mode;
	char *devpath;
	const char *str;
	unsigned volctl;
	int mmc, autostart, join;

	aparams_init(&ipar, 0, 1, 44100);
	aparams_init(&opar, 0, 1, 44100);
	u_flag = 0;
	d_flag = 0;
	l_flag = 0;
	n_flag = 0;
	unit = -1;
	mmc = 0;
	devpath = NULL;
	SLIST_INIT(&ifiles);
	SLIST_INIT(&ofiles);
	SLIST_INIT(&sfiles);
	SLIST_INIT(&qfiles);
	hdr = HDR_AUTO;
	xrun = XRUN_IGNORE;
	volctl = MIDI_MAXCTL;
	mode = MODE_PLAY | MODE_REC;
	bufsz = 0;
	round = 0;
	autostart = 1;
	join = 1;

	while ((c = getopt(argc, argv, "dnb:c:C:e:r:h:x:v:i:o:f:m:luq:s:U:t:j:z:")) != -1) {
		switch (c) {
		case 'd':
#ifdef DEBUG
			if (d_flag)
				debug_level++;
#endif
			d_flag = 1;
			break;
		case 'n':
			n_flag = 1;
			break;
		case 'm':
			mode = opt_mode();
			break;
		case 'h':
			hdr = opt_hdr();
			break;
		case 'x':
			xrun = opt_xrun();
			break;
		case 'j':
			join = opt_join();
			break;
		case 't':
			mmc = opt_mmc();
			if (mmc)
				autostart = 0;
			break;
		case 'c':
			opt_ch(&ipar);
			break;
		case 'C':
			opt_ch(&opar);
			break;
		case 'e':
			opt_enc(&ipar);
			aparams_copyenc(&opar, &ipar);
			break;
		case 'r':
			ipar.rate = strtonum(optarg, RATE_MIN, RATE_MAX, &str);
			if (str)
				errx(1, "%s: rate is %s", optarg, str);
			opar.rate = ipar.rate;
			break;
		case 'v':
			volctl = strtonum(optarg, 0, MIDI_MAXCTL, &str);
			if (str)
				errx(1, "%s: volume is %s", optarg, str);
			break;
		case 'i':
			file = optarg;
			if (strcmp(file, "-") == 0)
				file = NULL;
			farg_add(&ifiles, &ipar, &opar, volctl,
			    hdr, xrun, mmc, join, mode & MODE_PLAY, file);
			break;
		case 'o':
			file = optarg;
			if (strcmp(file, "-") == 0)
				file = NULL;
			farg_add(&ofiles, &ipar, &opar, volctl,
			    hdr, xrun, mmc, join, mode & MODE_RECMASK, file);
			break;
		case 's':
			farg_add(&sfiles, &ipar, &opar, volctl,
			    hdr, xrun, mmc, join, mode, optarg);
			break;
		case 'q':
			farg_add(&qfiles, &aparams_none, &aparams_none,
			    0, HDR_RAW, 0, 0, 0, 0, optarg);
			break;
		case 'f':
			if (devpath)
				err(1, "only one -f allowed");
			devpath = optarg;
			dipar = opar;
			dopar = ipar;
			break;
		case 'l':
			l_flag = 1;
			autostart = 0;
			break;
		case 'u':
			u_flag = 1;
			break;
		case 'b':
			bufsz = strtonum(optarg, 1, RATE_MAX * 5, &str);
			if (str)
				errx(1, "%s: buffer size is %s", optarg, str);
			break;
		case 'U':
			unit = strtonum(optarg, 0, MIDI_MAXCTL, &str);
			if (str)
				errx(1, "%s: device number is %s", optarg, str);
			break;
		case 'z':
			round = strtonum(optarg, 1, SHRT_MAX, &str);
			if (str)
				errx(1, "%s: block size is %s", optarg, str);
			break;
		default:
			aucat_usage();
			exit(1);
		}
	}
	argc -= optind;
	argv += optind;

	if (!devpath) {
		dopar = ipar;
		dipar = opar;
	}
	if (!l_flag && SLIST_EMPTY(&ifiles) &&
	    SLIST_EMPTY(&ofiles) && argc > 0) {
		/*
		 * Legacy mode: if no -i or -o options are provided, and
		 * there are arguments then assume the arguments are files
		 * to play.
		 */
		for (c = 0; c < argc; c++)
			if (legacy_play(devpath, argv[c]) != 0) {
				errx(1, "%s: could not play\n", argv[c]);
			}
		exit(0);
	} else if (argc > 0) {
		aucat_usage();
		exit(1);
	}

	if (!l_flag && (!SLIST_EMPTY(&sfiles) || unit >= 0))
		errx(1, "can't use -s or -U without -l");
	if (l_flag && (!SLIST_EMPTY(&ofiles) || !SLIST_EMPTY(&ifiles)))
		errx(1, "can't use -l, and -s with -o or -i");
	if (n_flag) {
		if (devpath != NULL || !SLIST_EMPTY(&qfiles) ||
		    l_flag || !autostart)
			errx(1, "can't use -n with -f, -q, -t or -l");
		if (SLIST_EMPTY(&ifiles) || SLIST_EMPTY(&ofiles))
			errx(1, "both -i and -o are required with -n");
	}

	/*
	 * If there are no sockets paths provided use the default.
	 */
	if (l_flag && SLIST_EMPTY(&sfiles)) {
		farg_add(&sfiles, &ipar, &opar,
		    volctl, HDR_RAW, XRUN_IGNORE, mmc, join, mode, DEFAULT_OPT);
	}

	/*
	 * Check modes and calculate "best" device parameters. Iterate over all
	 * inputs and outputs and find the maximum sample rate and channel
	 * number.
	 */
	mode = 0;
	aparams_init(&dipar, dipar.cmin, dipar.cmax, dipar.rate);
	aparams_init(&dopar, dopar.cmin, dopar.cmax, dopar.rate);
	SLIST_FOREACH(fa, &ifiles, entry) {
		if (fa->mode == 0)
			errx(1, "%s: not in play mode", fa->name);
		mode |= fa->mode;
		if (!u_flag)
			aparams_grow(&dopar, &fa->ipar);
	}
	SLIST_FOREACH(fa, &ofiles, entry) {
		if (fa->mode == 0)
			errx(1, "%s: not in rec/mon mode", fa->name);
		if ((fa->mode & MODE_REC) && (fa->mode & MODE_MON))
			errx(1, "%s: can't record and monitor", fa->name);
		mode |= fa->mode;
		if (!u_flag)
			aparams_grow(&dipar, &fa->opar);
	}
	SLIST_FOREACH(fa, &sfiles, entry) {
		if ((fa->mode & MODE_REC) && (fa->mode & MODE_MON))
			errx(1, "%s: can't record and monitor", fa->name);
		mode |= fa->mode;
		if (!u_flag) {
			aparams_grow(&dopar, &fa->ipar);
			aparams_grow(&dipar, &fa->opar);
		}
	}

	if (!round)
		round = ((mode & MODE_REC) ? dipar.rate : dopar.rate) / 15;
	if (!bufsz)
		bufsz = ((mode & MODE_REC) ? dipar.rate : dopar.rate) * 4 / 15;

	if (l_flag) {
		getbasepath(base, sizeof(base));
		if (unit < 0)
			unit = 0;
	}
	setsig();
	filelist_init();

	/*
	 * Open the device. Give half of the buffer to the device,
	 * the other half is for the socket/files.
	 */
	if (n_flag) {
		if (mode & MODE_MON)
			errx(1, "monitoring not allowed in loopback mode");
		dev_loopinit(&dipar, &dopar, bufsz);
	} else {
		if ((mode & MODE_MON) && !(mode & MODE_PLAY))
			errx(1, "no playback stream to monitor");
		if (!dev_init(devpath, mode, &dipar, &dopar, bufsz, round)) {
			errx(1, "%s: can't open device", 
			    devpath ? devpath : "<default>");
		}
	}

	/*
	 * Create buffers for all input and output pipes.
	 */
	while (!SLIST_EMPTY(&qfiles)) {
		fa = SLIST_FIRST(&qfiles);
		SLIST_REMOVE_HEAD(&qfiles, entry);
		if (!dev_thruadd(fa->name, 1, 1))
			errx(1, "%s: can't open device", fa->name);
		free(fa);
	}
	while (!SLIST_EMPTY(&ifiles)) {
		fa = SLIST_FIRST(&ifiles);
		SLIST_REMOVE_HEAD(&ifiles, entry);
		if (!wav_new_in(&wav_ops, fa->mode, fa->name,
			fa->hdr, &fa->ipar, fa->xrun, fa->vol, fa->mmc,
			fa->join))
			exit(1);
		free(fa);
	}
	while (!SLIST_EMPTY(&ofiles)) {
		fa = SLIST_FIRST(&ofiles);
		SLIST_REMOVE_HEAD(&ofiles, entry);
		if (!wav_new_out(&wav_ops, fa->mode, fa->name,
			fa->hdr, &fa->opar, fa->xrun, fa->mmc,
			fa->join))
		free(fa);
	}
	while (!SLIST_EMPTY(&sfiles)) {
		fa = SLIST_FIRST(&sfiles);
		SLIST_REMOVE_HEAD(&sfiles, entry);
		opt_new(fa->name, &fa->opar, &fa->ipar,
		    MIDI_TO_ADATA(fa->vol), fa->mmc, fa->join, fa->mode);
		free(fa);
	}
	if (l_flag) {
		snprintf(path, sizeof(path), "%s/%s%u", base,
		    DEFAULT_SOFTAUDIO, unit);
		listen_new(&listen_ops, path);
		if (geteuid() == 0)
			privdrop();
		if (!d_flag && daemon(0, 0) < 0)
			err(1, "daemon");
	}
	if (autostart) {
		/*
		 * inject artificial mmc start
		 */
		ctl_start(dev_midi);
	}
	if (l_flag)
		dev_prime();

	/*
	 * Loop, start audio.
	 */
	for (;;) {
		if (quit_flag) {
			break;
		}
		if ((APROC_OK(dev_mix) && LIST_EMPTY(&dev_mix->outs)) ||
		    (APROC_OK(dev_sub) && LIST_EMPTY(&dev_sub->ins))) {
			fprintf(stderr, "device disappeared, terminating\n");
			break;
		}
		if (!l_flag && ctl_idle(dev_midi))
			break;
		if (!file_poll())
			break;
		if ((!APROC_OK(dev_mix)    || dev_mix->u.mix.idle > 2 * dev_bufsz) &&
		    (!APROC_OK(dev_sub)    || dev_sub->u.sub.idle > 2 * dev_bufsz) &&
		    (!APROC_OK(dev_submon) || dev_submon->u.sub.idle > 2 * dev_bufsz) &&
		    (!APROC_OK(dev_midi)   || dev_midi->u.ctl.tstate != CTL_RUN)) {
		    	if (dev_pstate == DEV_RUN) {
				dev_pstate = DEV_INIT;
				dev_stop();
				dev_clear();
				/*
				 * priming buffer in non-server mode is not
				 * ok, because it will insert silence and
				 * break synchronization
				 */
				if (l_flag)
					dev_prime();
			}
		}
		/*
		 * move device state machine
		 * XXX: move this to dev.c
		 */
		if (dev_pstate == DEV_START) {
			dev_pstate = DEV_RUN;
			dev_start();
		}
	}
	stopall(base);
	dev_done();
	filelist_done();
	if (l_flag) {
		if (rmdir(base) < 0 && errno != ENOTEMPTY && errno != EPERM)
			warn("rmdir(\"%s\")", base);
	}
	unsetsig();
	return 0;
}

void
midicat_usage(void)
{
	(void)fputs("usage: " PROG_MIDICAT " [-dl] "
	    "[-i file] [-o file] [-q device] [-U unit]\n",
	    stderr);
}
int
midicat_main(int argc, char **argv)
{
	int c, d_flag, l_flag, unit, fd;
	struct farglist dfiles, ifiles, ofiles;
	char base[PATH_MAX], path[PATH_MAX];
	struct farg *fa;
	struct file *stdx;
	struct aproc *p;
	struct abuf *buf;
	const char *str;

	d_flag = 0;
	l_flag = 0;
	unit = -1;
	SLIST_INIT(&dfiles);
	SLIST_INIT(&ifiles);
	SLIST_INIT(&ofiles);

	while ((c = getopt(argc, argv, "di:o:lf:q:U:")) != -1) {
		switch (c) {
		case 'd':
#ifdef DEBUG
			if (d_flag)
				debug_level++;
#endif
			d_flag = 1;
			break;
		case 'i':
			farg_add(&ifiles, &aparams_none, &aparams_none,
			    0, HDR_RAW, 0, 0, 0, 0, optarg);
			break;
		case 'o':
			farg_add(&ofiles, &aparams_none, &aparams_none,
			    0, HDR_RAW, 0, 0, 0, 0, optarg);
			break;
			/* XXX: backward compat, remove this */
		case 'f':	
		case 'q':
			farg_add(&dfiles, &aparams_none, &aparams_none,
			    0, HDR_RAW, 0, 0, 0, 0, optarg);
			break;
		case 'l':
			l_flag = 1;
			break;
		case 'U':
			unit = strtonum(optarg, 0, MIDI_MAXCTL, &str);
			if (str)
				errx(1, "%s: device number is %s", optarg, str);
			break;
		default:
			midicat_usage();
			exit(1);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 0 || (SLIST_EMPTY(&ifiles) && SLIST_EMPTY(&ofiles) &&
	    !l_flag)) {
		midicat_usage();
		exit(1);
	}
	if (!l_flag && unit >= 0)
		errx(1, "can't use -U without -l");
	if (l_flag) {
		if (!SLIST_EMPTY(&ifiles) || !SLIST_EMPTY(&ofiles))
			errx(1, "can't use -i or -o with -l");
		getbasepath(base, sizeof(path));
		if (unit < 0)
			unit = 0;
	}
	setsig();
	filelist_init();

	dev_thruinit();
	if (!l_flag && APROC_OK(dev_midi))
		dev_midi->flags |= APROC_QUIT;
	if ((!SLIST_EMPTY(&ifiles) || !SLIST_EMPTY(&ofiles)) && 
	    SLIST_EMPTY(&dfiles)) {
		farg_add(&dfiles, &aparams_none, &aparams_none,
		    0, HDR_RAW, 0, 0, 0, 0, NULL);
	}
	while (!SLIST_EMPTY(&dfiles)) {
		fa = SLIST_FIRST(&dfiles);
		SLIST_REMOVE_HEAD(&dfiles, entry);
		if (!dev_thruadd(fa->name, 
			!SLIST_EMPTY(&ofiles) || l_flag,
			!SLIST_EMPTY(&ifiles) || l_flag)) {
			errx(1, "%s: can't open device", 
			    fa->name ? fa->name : "<default>");
		}
		free(fa);
	}
	if (l_flag) {
		snprintf(path, sizeof(path), "%s/%s%u", base,
		    DEFAULT_MIDITHRU, unit);
		listen_new(&listen_ops, path);
		if (geteuid() == 0)
			privdrop();
		if (!d_flag && daemon(0, 0) < 0)
			err(1, "daemon");
	}
	while (!SLIST_EMPTY(&ifiles)) {
		fa = SLIST_FIRST(&ifiles);
		SLIST_REMOVE_HEAD(&ifiles, entry);
		if (strcmp(fa->name, "-") == 0) {
			fd = STDIN_FILENO;
			if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
				warn("stdin");
		} else {
			fd = open(fa->name, O_RDONLY | O_NONBLOCK, 0666);
			if (fd < 0)
				err(1, "%s", fa->name);
		}
		stdx = (struct file *)pipe_new(&pipe_ops, fd, fa->name);
		p = rfile_new(stdx);
		buf = abuf_new(MIDI_BUFSZ, &aparams_none);
		aproc_setout(p, buf);
		dev_midiattach(buf, NULL);
		free(fa);
	}
	while (!SLIST_EMPTY(&ofiles)) {
		fa = SLIST_FIRST(&ofiles);
		SLIST_REMOVE_HEAD(&ofiles, entry);
		if (strcmp(fa->name, "-") == 0) {
			fd = STDOUT_FILENO;
			if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
				warn("stdout");
		} else {
			fd = open(fa->name,
			    O_WRONLY | O_TRUNC | O_CREAT | O_NONBLOCK, 0666);
			if (fd < 0)
				err(1, "%s", fa->name);
		}
		stdx = (struct file *)pipe_new(&pipe_ops, fd, fa->name);
		p = wfile_new(stdx);
		buf = abuf_new(MIDI_BUFSZ, &aparams_none);
		aproc_setin(p, buf);
		dev_midiattach(NULL, buf);
		free(fa);
	}
	/*
	 * loop, start processing
	 */
	for (;;) {
		if (quit_flag) {
			break;
		}
		if (!file_poll())
			break;
	}
	stopall(base);
	dev_done();
	filelist_done();
	if (l_flag) {
		if (rmdir(base) < 0 && errno != ENOTEMPTY && errno != EPERM)
			warn("rmdir(\"%s\")", base);
	}
	unsetsig();
	return 0;
}


int
main(int argc, char **argv)
{
	char *prog;

	prog = strrchr(argv[0], '/');
	if (prog == NULL)
		prog = argv[0];
	else
		prog++;
	if (strcmp(prog, PROG_AUCAT) == 0) {
		return aucat_main(argc, argv);
	} else if (strcmp(prog, PROG_MIDICAT) == 0) {
		return midicat_main(argc, argv);
	} else {
		fprintf(stderr, "%s: can't determine program to run\n", prog);
	}
	return 1;
}
