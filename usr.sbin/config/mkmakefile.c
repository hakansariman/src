/*	$OpenBSD: mkmakefile.c,v 1.25 2007/11/26 17:25:59 deraadt Exp $	*/
/*	$NetBSD: mkmakefile.c,v 1.34 1997/02/02 21:12:36 thorpej Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratories.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)mkmakefile.c	8.1 (Berkeley) 6/6/93
 */

#include <sys/param.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "sem.h"

/*
 * Make the Makefile.
 */

static const char *srcpath(struct files *);

static int emitdefs(FILE *);
static int emitfiles(FILE *, int);

static int emitobjs(FILE *);
static int emitcfiles(FILE *);
static int emitsfiles(FILE *);
static int emitrules(FILE *);
static int emitload(FILE *);

int
mkmakefile(void)
{
	FILE *ifp, *ofp;
	int lineno;
	int (*fn)(FILE *);
	char *ifname;
	char line[BUFSIZ], buf[200];

	(void)snprintf(buf, sizeof buf, "arch/%s/conf/Makefile.%s",
	    machine, machine);
	ifname = sourcepath(buf);
	if ((ifp = fopen(ifname, "r")) == NULL) {
		(void)fprintf(stderr, "config: cannot read %s: %s\n",
		    ifname, strerror(errno));
		free(ifname);
		return (1);
	}
	if ((ofp = fopen("Makefile", "w")) == NULL) {
		(void)fprintf(stderr, "config: cannot write Makefile: %s\n",
		    strerror(errno));
		free(ifname);
		return (1);
	}
	if (emitdefs(ofp) != 0)
		goto wrerror;
	lineno = 0;
	while (fgets(line, sizeof(line), ifp) != NULL) {
		lineno++;
		if (line[0] != '%') {
			if (fputs(line, ofp) < 0)
				goto wrerror;
			continue;
		}
		if (strcmp(line, "%OBJS\n") == 0)
			fn = emitobjs;
		else if (strcmp(line, "%CFILES\n") == 0)
			fn = emitcfiles;
		else if (strcmp(line, "%SFILES\n") == 0)
			fn = emitsfiles;
		else if (strcmp(line, "%RULES\n") == 0)
			fn = emitrules;
		else if (strcmp(line, "%LOAD\n") == 0)
			fn = emitload;
		else {
			xerror(ifname, lineno,
			    "unknown %% construct ignored: %s", line);
			continue;
		}
		if ((*fn)(ofp))
			goto wrerror;
	}
	if (ferror(ifp)) {
		(void)fprintf(stderr,
		    "config: error reading %s (at line %d): %s\n",
		    ifname, lineno, strerror(errno));
		goto bad;
	}
	if (fclose(ofp)) {
		ofp = NULL;
		goto wrerror;
	}
	(void)fclose(ifp);
	free(ifname);
	return (0);
wrerror:
	(void)fprintf(stderr, "config: error writing Makefile: %s\n",
	    strerror(errno));
bad:
	if (ofp != NULL)
		(void)fclose(ofp);
	/* (void)unlink("Makefile"); */
	free(ifname);
	return (1);
}

/*
 * Return (possibly in a static buffer) the name of the `source' for a
 * file.  If we have `options source', or if the file is marked `always
 * source', this is always the path from the `file' line; otherwise we
 * get the .o from the obj-directory.
 */
static const char *
srcpath(struct files *fi)
{
	/* Always have source, don't support object dirs for kernel builds. */
	struct nvlist *nv, *nv1;
	char *expand, *source;
	const char *var;

	/* Search path list for files we will want to use */
	if (fi->fi_nvpath->nv_next == NULL) {
		nv = fi->fi_nvpath;
		goto onlyone;
	}

	for (nv = fi->fi_nvpath; nv; nv = nv->nv_next) {
		char *s, *e;

		expand = NULL;
		s = strchr(nv->nv_name, '$');
		if (s) {
			size_t len;

			/* Search for a ${name} to expand */
			if (s[1] != '{')
				error("{");
			e = strchr(s + 2, '}');
			if (!e)
				error("}");

			len = e - s - 2;
			if (strlen("MACHINE_ARCH") == len &&
			    bcmp(s + 2, "MACHINE_ARCH", len) == 0)
				var = machinearch ? machinearch : machine;
			else if (strlen("MACHINE") == len &&
			    bcmp(s + 2, "MACHINE", len) == 0)
				var = machine;
			else
				error("variable %*.*s not supported",
				    e - s - 2, e - s - 2, s + 2);

			asprintf(&expand, "%*.*s%s%s",
			    s - nv->nv_name, s - nv->nv_name, nv->nv_name,
			    var, e + 1);
			source = sourcepath(expand);
		} else
			source = sourcepath(nv->nv_name);
		if (access(source, R_OK) == 0) {
			/* poolalloc() prevents freeing old nv_name */
			if (expand)
				nv->nv_name = intern(expand);
			break;
		}
		if (expand)
			free(expand);
		free(source);
	}
	if (nv == NULL)
		nv = fi->fi_nvpath;

	/*
	 * Now that we know which path is right, delete all the
	 * other filenames to skip the access() checks next time
	 */
	while ((nv1 = fi->fi_nvpath)) {
		nv1 = nv1->nv_next;
		if (fi->fi_nvpath != nv)
			nvfree(fi->fi_nvpath);
		fi->fi_nvpath = nv1;
	}
	fi->fi_nvpath = nv;
	nv->nv_next = NULL;
onlyone:
	return (nv->nv_name);
}

static int
emitdefs(FILE *fp)
{
	struct nvlist *nv;
	char *sp;

	if (fputs("IDENT=", fp) < 0)
		return (1);
	sp = "";
	for (nv = options; nv != NULL; nv = nv->nv_next) {
		if (ht_lookup(defopttab, nv->nv_name) != NULL)
			continue;
		if (fprintf(fp, "%s-D%s", sp, nv->nv_name) < 0)
		    return 1;
		if (nv->nv_str)
		    if (fprintf(fp, "=\"%s\"", nv->nv_str) < 0)
			return 1;
		sp = " ";
	}
	if (putc('\n', fp) < 0)
		return (1);
	if (fprintf(fp, "PARAM=-DMAXUSERS=%d\n", maxusers) < 0)
		return (1);
	switch (*srcdir) {
	case '/':
		if (fprintf(fp, "S=\t%s\n", srcdir) < 0)
			return (1);
		break;
	case '.':
		if (fprintf(fp, "S!=\techo `/bin/pwd`/%s\n", srcdir) < 0)
			return (1);
		break;
	default:
		/*
		 * libkern and libcompat "Makefile.inc"s want relative S
		 * specification to begin with '.'.
		 */
		if (fprintf(fp, "S!=\techo .`/bin/pwd`/%s\n", srcdir) < 0)
			return (1);
		break;
	}
	for (nv = mkoptions; nv != NULL; nv = nv->nv_next)
		if (fprintf(fp, "%s=%s\n", nv->nv_name, nv->nv_str) < 0)
			return (1);
	return (0);
}

static int
emitobjs(FILE *fp)
{
	struct files *fi;
	struct objects *oi;
	int lpos, len, sp;
	const char *fpath;

	if (fputs("LINTS=", fp) < 0)
		return (1);
	sp = '\t';
	lpos = 7;
	for (fi = allfiles; fi != NULL; fi = fi->fi_next) {
		if ((fi->fi_flags & FI_SEL) == 0)
			continue;
		if ((fpath = srcpath(fi)) == NULL)
			return (1);
		len = strlen(fpath);
		if (fpath[len - 1] == 's' || fpath[len - 1] == 'S')
			continue;
		len = strlen(fi->fi_base) + 3;
		if (lpos + len > 72) {
			if (fputs(" \\\n", fp) < 0)
				return (1);
			sp = '\t';
			lpos = 7;
		}
		if (fprintf(fp, "%c%s.ln", sp, fi->fi_base) < 0)
			return (1);
		lpos += len + 1;
		sp = ' ';
	}
	if (fputs("\n\nOBJS=\t${LINTS:.ln=.o}", fp) < 0)
		return (1);
	lpos = 7 + strlen("${LINTS:.ln=.o}");
	for (fi = allfiles; fi != NULL; fi = fi->fi_next) {
		if ((fi->fi_flags & FI_SEL) == 0)
			continue;
		if ((fpath = srcpath(fi)) == NULL)
			return (1);
		len = strlen(fpath);
		if (fpath[len - 1] != 's' && fpath[len - 1] != 'S')
			continue;
		len = strlen(fi->fi_base) + 3;
		if (lpos + len > 72) {
			if (fputs(" \\\n", fp) < 0)
				return (1);
			sp = '\t';
			lpos = 7;
		}
		if (fprintf(fp, "%c%s.o", sp, fi->fi_base) < 0)
			return (1);
		lpos += len + 1;
		sp = ' ';
	}
	for (oi = allobjects; oi != NULL; oi = oi->oi_next) {
		if ((oi->oi_flags & OI_SEL) == 0)
			continue;
		len = strlen(oi->oi_path) + 3;
		if (lpos + len > 72) {
			if (fputs(" \\\n", fp) < 0)
				return (1);
			sp = '\t';
			lpos = 7;
		}
		if (fprintf(fp, "%c$S/%s", sp, oi->oi_path) < 0)
			return (1);
		lpos += len + 1;
		sp = ' ';
	}
	if (putc('\n', fp) < 0)
		return (1);
	return (0);
}

static int
emitcfiles(FILE *fp)
{

	return (emitfiles(fp, 'c'));
}

static int
emitsfiles(FILE *fp)
{

	return (emitfiles(fp, 's'));
}

static int
emitfiles(FILE *fp, int suffix)
{
	struct files *fi;
	struct config *cf;
	int lpos, len, sp;
	const char *fpath;
	char swapname[100];
	int uppersuffix = toupper(suffix);

	if (fprintf(fp, "%cFILES=", uppersuffix) < 0)
		return (1);
	sp = '\t';
	lpos = 7;
	for (fi = allfiles; fi != NULL; fi = fi->fi_next) {
		if ((fi->fi_flags & FI_SEL) == 0)
			continue;
		if ((fpath = srcpath(fi)) == NULL)
			return (1);
		len = strlen(fpath);
		if (fpath[len - 1] != suffix && fpath[len - 1] != uppersuffix)
			continue;
		if (*fpath != '/')
			len += 3;	/* "$S/" */
		if (lpos + len > 72) {
			if (fputs(" \\\n", fp) < 0)
				return (1);
			sp = '\t';
			lpos = 7;
		}
		if (fprintf(fp, "%c%s%s", sp, *fpath != '/' ? "$S/" : "",
		    fpath) < 0)
			return (1);
		lpos += len + 1;
		sp = ' ';
	}
	/*
	 * The allfiles list does not include the configuration-specific
	 * C source files.  These files should be eliminated someday, but
	 * for now, we have to add them to ${CFILES} (and only ${CFILES}).
	 */
	if (suffix == 'c') {
		for (cf = allcf; cf != NULL; cf = cf->cf_next) {
			if (cf->cf_root == NULL)
				(void)snprintf(swapname, sizeof swapname,
				    "$S/conf/swapgeneric.c");
			else
				(void)snprintf(swapname, sizeof swapname,
				    "./swap%s.c", cf->cf_name);
			len = strlen(swapname);
			if (lpos + len > 72) {
				if (fputs(" \\\n", fp) < 0)
					return (1);
				sp = '\t';
				lpos = 7;
			}
			if (fprintf(fp, "%c%s", sp, swapname) < 0)
				return (1);
			lpos += len + 1;
			sp = ' ';
		}
	}
	if (putc('\n', fp) < 0)
		return (1);
	return (0);
}

/*
 * Emit the make-rules.
 */
static int
emitrules1(FILE *fp, const char *suffix, const char *rule_prefix, int ruleindex)
{
	struct files *fi;
	const char *cp, *fpath;
	int ch;
	char buf[200];

	for (fi = allfiles; fi != NULL; fi = fi->fi_next) {
		if ((fi->fi_flags & FI_SEL) == 0)
			continue;
		if ((fpath = srcpath(fi)) == NULL)
			return (1);
		if (fprintf(fp, "%s%s: %s%s\n", fi->fi_base, suffix,
		    *fpath != '/' ? "$S/" : "", fpath) < 0)
			return (1);
		if ((cp = fi->fi_mkrule[ruleindex]) == NULL) {
			cp = rule_prefix;
			if (fpath[0] == '\0') {
				errno = ENOENT;
				return (1);
			}
			ch = fpath[strlen(fpath) - 1];
			if (islower(ch))
				ch = toupper(ch);
			(void)snprintf(buf, sizeof buf, "${%s_%c}",
			    cp, ch);
			cp = buf;
		}
		if (fprintf(fp, "\t%s\n\n", cp) < 0)
			return (1);
	}
	return (0);
}

static int
emitrules(FILE *fp)
{
	return emitrules1(fp, ".o", "NORMAL", 0) || emitrules1(fp, ".ln", "LINT", 1);
}

/*
 * Emit the load commands.
 *
 * This function is not to be called `spurt'.
 */
static int
emitload(FILE *fp)
{
	struct config *cf;
	const char *nm, *swname;
	int first;

	if (fputs("all:", fp) < 0)
		return (1);
	for (cf = allcf; cf != NULL; cf = cf->cf_next) {
		if (fprintf(fp, " %s", cf->cf_name) < 0)
			return (1);
	}
	if (fputs("\n\n", fp) < 0)
		return (1);
	for (first = 1, cf = allcf; cf != NULL; cf = cf->cf_next) {
		nm = cf->cf_name;
		swname =
		    cf->cf_root != NULL ? cf->cf_name : "generic";
		if (fprintf(fp, "%s: ${SYSTEM_DEP} swap%s.o", nm, swname) < 0)
			return (1);
		if (first) {
			if (fputs(" newvers", fp) < 0)
				return (1);
			first = 0;
		}
		if (fprintf(fp, "\n"
		    "\t${SYSTEM_LD_HEAD}\n"
		    "\t${SYSTEM_LD} swap%s.o\n"
		    "\t${SYSTEM_LD_TAIL}\n"
		    "\n"
		    "swap%s.o: ", swname, swname) < 0)
			return (1);
		if (cf->cf_root != NULL) {
			if (fprintf(fp, "swap%s.c\n", nm) < 0)
				return (1);
		} else {
			if (fprintf(fp, "$S/conf/swapgeneric.c\n") < 0)
				return (1);
		}
		if (fputs("\t${NORMAL_C}\n\n", fp) < 0)
			return (1);
	}
	return (0);
}
