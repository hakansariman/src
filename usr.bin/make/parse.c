/*	$OpenPackages$ */
/*	$OpenBSD: parse.c,v 1.60 2001/05/15 12:52:15 espie Exp $	*/
/*	$NetBSD: parse.c,v 1.29 1997/03/10 21:20:04 christos Exp $	*/

/*
 * Copyright (c) 1999 Marc Espie.
 *
 * Extensive code changes for the OpenBSD project.
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
 * THIS SOFTWARE IS PROVIDED BY THE OPENBSD PROJECT AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OPENBSD
 * PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1988, 1989, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1989 by Berkeley Softworks
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam de Boor.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 */

/*-
 * parse.c --
 *	Functions to parse a makefile.
 *
 *	One function, Parse_Init, must be called before any functions
 *	in this module are used. After that, the function Parse_File is the
 *	main entry point and controls most of the other functions in this
 *	module.
 *
 *	Most important structures are kept in Lsts. Directories for
 *	the #include "..." function are kept in the 'parseIncPath' Lst, while
 *	those for the #include <...> are kept in the 'sysIncPath' Lst. The
 *	targets currently being defined are kept in the 'targets' Lst.
 *
 *	The variables 'fname' and 'lineno' are used to track the name
 *	of the current file and the line number in that file so that error
 *	messages can be more meaningful.
 *
 * Interface:
 *	Parse_Init		    Initialization function which must be
 *				    called before anything else in this module
 *				    is used.
 *
 *	Parse_End		    Cleanup the module
 *
 *	Parse_File		    Function used to parse a makefile. It must
 *				    be given the name of the file, which should
 *				    already have been opened, and a function
 *				    to call to read a character from the file.
 *
 *	Parse_IsVar		    Returns TRUE if the given line is a
 *				    variable assignment. Used by MainParseArgs
 *				    to determine if an argument is a target
 *				    or a variable assignment. Used internally
 *				    for pretty much the same thing...
 *
 *	Parse_Error		    Function called when an error occurs in
 *				    parsing. Used by the variable and
 *				    conditional modules.
 *	Parse_MainName		    Returns a Lst of the main target to create.
 */

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include "make.h"
#include "ohash.h"
#include "dir.h"
#include "job.h"
#include "buf.h"
#include "pathnames.h"
#include "lowparse.h"

#ifndef lint
#if 0
static char sccsid[] = "@(#)parse.c	8.3 (Berkeley) 3/19/94";
#else
UNUSED
static char rcsid[] = "$OpenBSD: parse.c,v 1.60 2001/05/15 12:52:15 espie Exp $";
#endif
#endif /* not lint */

LIST		parseIncPath;	/* list of directories for "..." includes */
LIST		sysIncPath;	/* list of directories for <...> includes */

static LIST	    targets;	/* targets we're working on */
#ifdef CLEANUP
static LIST	    targCmds;	/* command lines for targets */
static LIST	    fileNames;
#endif

static GNode	    *mainNode;	/* The main target to create. This is the
				 * first target on the first dependency
				 * line in the first makefile */
/*-
 * specType contains the SPECial TYPE of the current target. It is
 * Not if the target is unspecial. If it *is* special, however, the children
 * are linked as children of the parent but not vice versa. This variable is
 * set in ParseDoDependency
 */
typedef enum {
    Begin,	    /* .BEGIN */
    Default,	    /* .DEFAULT */
    End,	    /* .END */
    Ignore,	    /* .IGNORE */
    Includes,	    /* .INCLUDES */
    Interrupt,	    /* .INTERRUPT */
    Libs,	    /* .LIBS */
    MFlags,	    /* .MFLAGS or .MAKEFLAGS */
    Main,	    /* .MAIN and we don't have anything user-specified to
		     * make */
    NoExport,	    /* .NOEXPORT */
    NoPath,	    /* .NOPATH */
    Not,	    /* Not special */
    NotParallel,    /* .NOTPARALELL */
    Null,	    /* .NULL */
    Order,	    /* .ORDER */
    Parallel,	    /* .PARALLEL */
    ExPath,	    /* .PATH */
    Phony,	    /* .PHONY */
    Precious,	    /* .PRECIOUS */
    ExShell,	    /* .SHELL */
    Silent,	    /* .SILENT */
    SingleShell,    /* .SINGLESHELL */
    Suffixes,	    /* .SUFFIXES */
    Wait,	    /* .WAIT */
    Attribute	    /* Generic attribute */
} ParseSpecial;

static ParseSpecial specType;
static int waiting;

/*
 * Predecessor node for handling .ORDER. Initialized to NULL when .ORDER
 * seen, then set to each successive source on the line.
 */
static GNode	*predecessor;

/*
 * The parseKeywords table is searched using binary search when deciding
 * if a target or source is special. The 'spec' field is the ParseSpecial
 * type of the keyword ("Not" if the keyword isn't special as a target) while
 * the 'op' field is the operator to apply to the list of targets if the
 * keyword is used as a source ("0" if the keyword isn't special as a source)
 */
static struct {
    char	  *name;	/* Name of keyword */
    ParseSpecial  spec; 	/* Type when used as a target */
    int 	  op;		/* Operator when used as a source */
} parseKeywords[] = {
{ ".BEGIN",	  Begin,	0 },
{ ".DEFAULT",	  Default,	0 },
{ ".END",	  End,		0 },
{ ".EXEC",	  Attribute,	OP_EXEC },
{ ".IGNORE",	  Ignore,	OP_IGNORE },
{ ".INCLUDES",	  Includes,	0 },
{ ".INTERRUPT",   Interrupt,	0 },
{ ".INVISIBLE",   Attribute,	OP_INVISIBLE },
{ ".JOIN",	  Attribute,	OP_JOIN },
{ ".LIBS",	  Libs, 	0 },
{ ".MADE",	  Attribute,	OP_MADE },
{ ".MAIN",	  Main, 	0 },
{ ".MAKE",	  Attribute,	OP_MAKE },
{ ".MAKEFLAGS",   MFlags,	0 },
{ ".MFLAGS",	  MFlags,	0 },
#if 0	/* basic scaffolding for NOPATH, not working yet */
{ ".NOPATH",	  NoPath,	OP_NOPATH },
#endif
{ ".NOTMAIN",	  Attribute,	OP_NOTMAIN },
{ ".NOTPARALLEL", NotParallel,	0 },
{ ".NO_PARALLEL", NotParallel,	0 },
{ ".NULL",	  Null, 	0 },
{ ".OPTIONAL",	  Attribute,	OP_OPTIONAL },
{ ".ORDER",	  Order,	0 },
{ ".PARALLEL",	  Parallel,	0 },
{ ".PATH",	  ExPath,	0 },
{ ".PHONY",	  Phony,	OP_PHONY },
{ ".PRECIOUS",	  Precious,	OP_PRECIOUS },
{ ".RECURSIVE",   Attribute,	OP_MAKE },
{ ".SHELL",	  ExShell,	0 },
{ ".SILENT",	  Silent,	OP_SILENT },
{ ".SINGLESHELL", SingleShell,	0 },
{ ".SUFFIXES",	  Suffixes,	0 },
{ ".USE",	  Attribute,	OP_USE },
{ ".WAIT",	  Wait, 	0 },
};

static int ParseFindKeyword(const char *);
static void ParseLinkSrc(GNode *, GNode *);
static int ParseDoOp(void *, void *);
static int ParseAddDep(void *, void *);
static void ParseDoSrc(int, const char *, Lst);
static int ParseFindMain(void *, void *);
static void ParseAddDir(void *, void *);
static void ParseClearPath(void *);
static void ParseDoDependency(char *);
static void ParseAddCmd(void *, void *);
static void ParseHasCommands(void *);
static void ParseDoInclude(char *);
static void ParseTraditionalInclude(char *);
static void ParseConditionalInclude(char *);
static void ParseLookupIncludeFile(char *, char *, Boolean, Boolean);
#define ParseGetLoopLine(linebuf)	ParseGetLine(linebuf, "for loop")
static void ParseFinishDependency(void);
static Boolean ParseIsCond(Buffer, Buffer, char *);
static char *strip_comments(Buffer, const char *);

static void ParseDoCommands(const char *);
static const char *find_op1(const char *);
static const char *find_op2(const char *);

/*-
 *----------------------------------------------------------------------
 * ParseFindKeyword --
 *	Look in the table of keywords for one matching the given string.
 *
 * Results:
 *	The index of the keyword, or -1 if it isn't there.
 *----------------------------------------------------------------------
 */
static int
ParseFindKeyword(str)
    const char	    *str;		/* String to find */
{
    int 	    start,
		    end,
		    cur;
    int 	    diff;

    start = 0;
    end = (sizeof(parseKeywords)/sizeof(parseKeywords[0])) - 1;

    do {
	cur = start + (end - start) / 2;
	diff = strcmp(str, parseKeywords[cur].name);

	if (diff == 0) {
	    return cur;
	} else if (diff < 0) {
	    end = cur - 1;
	} else {
	    start = cur + 1;
	}
    } while (start <= end);
    return -1;
}
/*-
 *---------------------------------------------------------------------
 * ParseLinkSrc  --
 *	Link the parent node to its new child. Used by
 *	ParseDoDependency. If the specType isn't 'Not', the parent
 *	isn't linked as a parent of the child.
 *
 * Side Effects:
 *	New elements are added to the parents list of cgn and the
 *	children list of cgn. the unmade field of pgn is updated
 *	to reflect the additional child.
 *---------------------------------------------------------------------
 */
static void
ParseLinkSrc(pgn, cgn)
    GNode		*pgn;	/* The parent node */
    GNode		*cgn;	/* The child node */
{
    if (Lst_AddNew(&pgn->children, cgn) == SUCCESS) {
	if (specType == Not)
	    Lst_AtEnd(&cgn->parents, pgn);
	pgn->unmade++;
    }
}

/*-
 *---------------------------------------------------------------------
 * ParseDoOp  --
 *	Apply the parsed operator to the given target node. Used in a
 *	Lst_Find call by ParseDoDependency once all targets have
 *	been found and their operator parsed. If the previous and new
 *	operators are incompatible, a major error is taken.
 *
 * Side Effects:
 *	The type field of the node is altered to reflect any new bits in
 *	the op.
 *---------------------------------------------------------------------
 */
static int
ParseDoOp(gnp, opp)
    void	   *gnp;	/* The node to which the operator is to be
				 * applied */
    void	   *opp;	/* The operator to apply */
{
    GNode	   *gn = (GNode *)gnp;
    int 	    op = *(int *)opp;
    /*
     * If the dependency mask of the operator and the node don't match and
     * the node has actually had an operator applied to it before, and
     * the operator actually has some dependency information in it, complain.
     */
    if (((op & OP_OPMASK) != (gn->type & OP_OPMASK)) &&
	!OP_NOP(gn->type) && !OP_NOP(op)) {
	Parse_Error(PARSE_FATAL, "Inconsistent operator for %s", gn->name);
	return 0;
    }

    if (op == OP_DOUBLEDEP && ((gn->type & OP_OPMASK) == OP_DOUBLEDEP)) {
	/* If the node was the object of a :: operator, we need to create a
	 * new instance of it for the children and commands on this dependency
	 * line. The new instance is placed on the 'cohorts' list of the
	 * initial one (note the initial one is not on its own cohorts list)
	 * and the new instance is linked to all parents of the initial
	 * instance.  */
	GNode		*cohort;
	LstNode 	ln;

	cohort = Targ_NewGN(gn->name, NULL);
	/* Duplicate links to parents so graph traversal is simple. Perhaps
	 * some type bits should be duplicated?
	 *
	 * Make the cohort invisible as well to avoid duplicating it into
	 * other variables. True, parents of this target won't tend to do
	 * anything with their local variables, but better safe than
	 * sorry.  */
	for (ln = Lst_First(&gn->parents); ln != NULL; ln = Lst_Adv(ln))
	    ParseLinkSrc((GNode *)Lst_Datum(ln), cohort);
	cohort->type = OP_DOUBLEDEP|OP_INVISIBLE;
	Lst_AtEnd(&gn->cohorts, cohort);

	/* Replace the node in the targets list with the new copy */
	ln = Lst_Member(&targets, gn);
	Lst_Replace(ln, cohort);
	gn = cohort;
    }
    /* We don't want to nuke any previous flags (whatever they were) so we
     * just OR the new operator into the old.  */
    gn->type |= op;
    return 1;
}

/*-
 *---------------------------------------------------------------------
 * ParseAddDep	--
 *	Check if the pair of GNodes given needs to be synchronized.
 *	This has to be when two nodes are on different sides of a
 *	.WAIT directive.
 *
 * Results:
 *	Returns 0 if the two targets need to be ordered, 1 otherwise.
 *	If it returns 0, the search can stop.
 *
 * Side Effects:
 *	A dependency can be added between the two nodes.
 *
 *---------------------------------------------------------------------
 */
static int
ParseAddDep(pp, sp)
    void *pp;
    void *sp;
{
    GNode *p = (GNode *)pp;
    GNode *s = (GNode *)sp;

    if (p->order < s->order) {
	/* XXX: This can cause loops, and loops can cause unmade targets,
	 * but checking is tedious, and the debugging output can show the
	 * problem.  */
	Lst_AtEnd(&p->successors, s);
	Lst_AtEnd(&s->preds, p);
	return 1;
    }
    else
	return 0;
}


/*-
 *---------------------------------------------------------------------
 * ParseDoSrc  --
 *	Given the name of a source, figure out if it is an attribute
 *	and apply it to the targets if it is. Else decide if there is
 *	some attribute which should be applied *to* the source because
 *	of some special target and apply it if so. Otherwise, make the
 *	source be a child of the targets in the list 'targets'
 *
 * Side Effects:
 *	Operator bits may be added to the list of targets or to the source.
 *	The targets may have a new source added to their lists of children.
 *---------------------------------------------------------------------
 */
static void
ParseDoSrc(tOp, src, allsrc)
    int 	tOp;	/* operator (if any) from special targets */
    const char	*src;	/* name of the source to handle */
    Lst 	allsrc; /* List of all sources to wait for */

{
    GNode	*gn = NULL;

    if (*src == '.' && isupper(src[1])) {
	int keywd = ParseFindKeyword(src);
	if (keywd != -1) {
	    int op = parseKeywords[keywd].op;
	    if (op != 0) {
		Lst_Find(&targets, ParseDoOp, &op);
		return;
	    }
	    if (parseKeywords[keywd].spec == Wait) {
		waiting++;
		return;
	    }
	}
    }

    switch (specType) {
    case Main:
	/*
	 * If we have noted the existence of a .MAIN, it means we need
	 * to add the sources of said target to the list of things
	 * to create. The string 'src' is likely to be free, so we
	 * must make a new copy of it. Note that this will only be
	 * invoked if the user didn't specify a target on the command
	 * line. This is to allow #ifmake's to succeed, or something...
	 */
	Lst_AtEnd(&create, estrdup(src));
	/*
	 * Add the name to the .TARGETS variable as well, so the user cna
	 * employ that, if desired.
	 */
	Var_Append(".TARGETS", src, VAR_GLOBAL);
	return;

    case Order:
	/*
	 * Create proper predecessor/successor links between the previous
	 * source and the current one.
	 */
	gn = Targ_FindNode(src, NULL, TARG_CREATE);
	if (predecessor != NULL) {
	    Lst_AtEnd(&predecessor->successors, gn);
	    Lst_AtEnd(&gn->preds, predecessor);
	}
	/*
	 * The current source now becomes the predecessor for the next one.
	 */
	predecessor = gn;
	break;

    default:
	/*
	 * If the source is not an attribute, we need to find/create
	 * a node for it. After that we can apply any operator to it
	 * from a special target or link it to its parents, as
	 * appropriate.
	 *
	 * In the case of a source that was the object of a :: operator,
	 * the attribute is applied to all of its instances (as kept in
	 * the 'cohorts' list of the node) or all the cohorts are linked
	 * to all the targets.
	 */
	gn = Targ_FindNode(src, NULL, TARG_CREATE);
	if (tOp) {
	    gn->type |= tOp;
	} else {
	    LstNode	ln;

	    for (ln = Lst_First(&targets); ln != NULL; ln = Lst_Adv(ln))
		ParseLinkSrc((GNode *)Lst_Datum(ln), gn);
	}
	if ((gn->type & OP_OPMASK) == OP_DOUBLEDEP) {
	    GNode	*cohort;
	    LstNode	ln;

	    for (ln=Lst_First(&gn->cohorts); ln != NULL; ln = Lst_Adv(ln)){
		cohort = (GNode *)Lst_Datum(ln);
		if (tOp) {
		    cohort->type |= tOp;
		} else {
		    LstNode	ln;

		    for (ln = Lst_First(&targets); ln != NULL; ln = Lst_Adv(ln))
			ParseLinkSrc((GNode *)Lst_Datum(ln), cohort);
		}
	    }
	}
	break;
    }

    gn->order = waiting;
    Lst_AtEnd(allsrc, gn);
    if (waiting) {
	Lst_Find(allsrc, ParseAddDep, gn);
    }
}

/*-
 *-----------------------------------------------------------------------
 * ParseFindMain --
 *	Find a real target in the list and set it to be the main one.
 *	Called by ParseDoDependency when a main target hasn't been found
 *	yet.
 *
 * Results:
 *	1 if main not found yet, 0 if it is.
 *
 * Side Effects:
 *	mainNode is changed and Targ_SetMain is called.
 *-----------------------------------------------------------------------
 */
static int
ParseFindMain(gnp, dummy)
    void *gnp;	    /* Node to examine */
    void *dummy 	UNUSED;
{
    GNode	  *gn = (GNode *)gnp;
    if ((gn->type & OP_NOTARGET) == 0) {
	mainNode = gn;
	Targ_SetMain(gn);
	return 0;
    } else {
	return 1;
    }
}

/*-
 *-----------------------------------------------------------------------
 * ParseAddDir --
 *	Front-end for Dir_AddDir to make sure Lst_ForEach keeps going
 *
 * Side Effects:
 *	See Dir_AddDir.
 *-----------------------------------------------------------------------
 */
static void
ParseAddDir(path, name)
    void *path;
    void *name;
{
    Dir_AddDir((Lst)path, (char *)name, NULL);
}

/*-
 *-----------------------------------------------------------------------
 * ParseClearPath --
 *	Reinit path to an empty path
 *-----------------------------------------------------------------------
 */
static void
ParseClearPath(p)
    void	*p;
{
    Lst 	path = (Lst)p;

    Lst_Destroy(path, Dir_Destroy);
    Lst_Init(path);
}

/*-
 *---------------------------------------------------------------------
 * ParseDoDependency  --
 *	Parse the dependency line in line.
 *
 * Side Effects:
 *	The nodes of the sources are linked as children to the nodes of the
 *	targets. Some nodes may be created.
 *
 *	We parse a dependency line by first extracting words from the line and
 * finding nodes in the list of all targets with that name. This is done
 * until a character is encountered which is an operator character. Currently
 * these are only ! and :. At this point the operator is parsed and the
 * pointer into the line advanced until the first source is encountered.
 *	The parsed operator is applied to each node in the 'targets' list,
 * which is where the nodes found for the targets are kept, by means of
 * the ParseDoOp function.
 *	The sources are read in much the same way as the targets were except
 * that now they are expanded using the wildcarding scheme of the C-Shell
 * and all instances of the resulting words in the list of all targets
 * are found. Each of the resulting nodes is then linked to each of the
 * targets as one of its children.
 *	Certain targets are handled specially. These are the ones detailed
 * by the specType variable.
 *	The storing of transformation rules is also taken care of here.
 * A target is recognized as a transformation rule by calling
 * Suff_IsTransform. If it is a transformation rule, its node is gotten
 * from the suffix module via Suff_AddTransform rather than the standard
 * Targ_FindNode in the target module.
 *---------------------------------------------------------------------
 */
static void
ParseDoDependency(line)
    char	   *line;	/* the line to parse */
{
    char	   *cp; 	/* our current position */
    GNode	   *gn; 	/* a general purpose temporary node */
    int 	    op; 	/* the operator on the line */
    char	    savec;	/* a place to save a character */
    LIST	    paths;	/* List of search paths to alter when parsing
				 * a list of .PATH targets */
    int 	    tOp;	/* operator from special target */
    LIST	    curTargs;	/* list of target names to be found and added
				 * to the targets list */
    LIST	    curSrcs;	/* list of sources in order */

    tOp = 0;

    specType = Not;
    waiting = 0;
    Lst_Init(&paths);

    Lst_Init(&curTargs);
    Lst_Init(&curSrcs);

    do {
	for (cp = line; *cp && !isspace(*cp) && *cp != '(';)
	    if (*cp == '$')
		/* Must be a dynamic source (would have been expanded
		 * otherwise), so call the Var module to parse the puppy
		 * so we can safely advance beyond it...There should be
		 * no errors in this, as they would have been discovered
		 * in the initial Var_Subst and we wouldn't be here.  */
		cp += Var_ParseSkip(cp, NULL, NULL);
	    else {
		/* We don't want to end a word on ':' or '!' if there is a
		 * better match later on in the string.  By "better" I mean
		 * one that is followed by whitespace.	This allows the user
		 * to have targets like:
		 *    fie::fi:fo: fum
		 * where "fie::fi:fo" is the target.  In real life this is used
		 * for perl5 library man pages where "::" separates an object
		 * from its class.  Ie: "File::Spec::Unix".  This behaviour
		 * is also consistent with other versions of make.  */
		if (*cp == '!' || *cp == ':') {
		    char *p = cp + 1;

		    if (*cp == ':' && *p == ':')
			p++;

		    /* Found the best match already. */
		    if (isspace(*p) || *p == '\0')
			break;

		    do {
			p += strcspn(p, "!:");
			if (*p == '\0')
			    break;
			p++;
		    } while (!isspace(*p));

		    /* No better match later on... */
		    if (*p == '\0')
			break;

		}
		cp++;
	    }
	if (*cp == '(') {
	    /* Archives must be handled specially to make sure the OP_ARCHV
	     * flag is set in their 'type' field, for one thing, and because
	     * things like "archive(file1.o file2.o file3.o)" are permissible.
	     * Arch_ParseArchive will set 'line' to be the first non-blank
	     * after the archive-spec. It creates/finds nodes for the members
	     * and places them on the given list, returning SUCCESS if all
	     * went well and FAILURE if there was an error in the
	     * specification. On error, line should remain untouched.  */
	    if (Arch_ParseArchive(&line, &targets, NULL) != SUCCESS) {
		Parse_Error(PARSE_FATAL,
			     "Error in archive specification: \"%s\"", line);
		return;
	    } else {
		continue;
	    }
	}
	savec = *cp;

	if (*cp == '\0') {
	    /* Ending a dependency line without an operator is a Bozo no-no */
	    Parse_Error(PARSE_FATAL, "Need an operator");
	    return;
	}
	*cp = '\0';
	/* Have a word in line. See if it's a special target and set
	 * specType to match it.  */
	if (*line == '.' && isupper(line[1])) {
	    /* See if the target is a special target that must have it
	     * or its sources handled specially.  */
	    int keywd = ParseFindKeyword(line);
	    if (keywd != -1) {
		if (specType == ExPath && parseKeywords[keywd].spec != ExPath) {
		    Parse_Error(PARSE_FATAL, "Mismatched special targets");
		    return;
		}

		specType = parseKeywords[keywd].spec;
		tOp = parseKeywords[keywd].op;

		/*
		 * Certain special targets have special semantics:
		 *	.PATH		Have to set the dirSearchPath
		 *			variable too
		 *	.MAIN		Its sources are only used if
		 *			nothing has been specified to
		 *			create.
		 *	.DEFAULT	Need to create a node to hang
		 *			commands on, but we don't want
		 *			it in the graph, nor do we want
		 *			it to be the Main Target, so we
		 *			create it, set OP_NOTMAIN and
		 *			add it to the list, setting
		 *			DEFAULT to the new node for
		 *			later use. We claim the node is
		 *			A transformation rule to make
		 *			life easier later, when we'll
		 *			use Make_HandleUse to actually
		 *			apply the .DEFAULT commands.
		 *	.PHONY		The list of targets
		 *	.NOPATH 	Don't search for file in the path
		 *	.BEGIN
		 *	.END
		 *	.INTERRUPT	Are not to be considered the
		 *			main target.
		 *	.NOTPARALLEL	Make only one target at a time.
		 *	.SINGLESHELL	Create a shell for each command.
		 *	.ORDER		Must set initial predecessor to NULL
		 */
		switch (specType) {
		    case ExPath:
			Lst_AtEnd(&paths, &dirSearchPath);
			break;
		    case Main:
			if (!Lst_IsEmpty(&create)) {
			    specType = Not;
			}
			break;
		    case Begin:
		    case End:
		    case Interrupt:
			gn = Targ_FindNode(line, NULL, TARG_CREATE);
			gn->type |= OP_NOTMAIN;
			Lst_AtEnd(&targets, gn);
			break;
		    case Default:
			gn = Targ_NewGN(".DEFAULT", NULL);
			gn->type |= OP_NOTMAIN|OP_TRANSFORM;
			Lst_AtEnd(&targets, gn);
			DEFAULT = gn;
			break;
		    case NotParallel:
		    {
			extern int  maxJobs;

			maxJobs = 1;
			break;
		    }
		    case SingleShell:
			compatMake = 1;
			break;
		    case Order:
			predecessor = NULL;
			break;
		    default:
			break;
		}
	    } else if (strncmp(line, ".PATH", 5) == 0) {
		/*
		 * .PATH<suffix> has to be handled specially.
		 * Call on the suffix module to give us a path to
		 * modify.
		 */
		Lst	path;

		specType = ExPath;
		path = Suff_GetPath(&line[5]);
		if (path == NULL) {
		    Parse_Error(PARSE_FATAL,
				 "Suffix '%s' not defined (yet)",
				 &line[5]);
		    return;
		} else {
		    Lst_AtEnd(&paths, path);
		}
	    }
	}

	/*
	 * Have word in line. Get or create its node and stick it at
	 * the end of the targets list
	 */
	if (specType == Not && *line != '\0') {
	    char *targName;

	    if (Dir_HasWildcards(line)) {
		/*
		 * Targets are to be sought only in the current directory,
		 * so create an empty path for the thing. Note we need to
		 * use Dir_Destroy in the destruction of the path as the
		 * Dir module could have added a directory to the path...
		 */
		LIST	    emptyPath;

		Lst_Init(&emptyPath);
		Dir_Expand(line, &emptyPath, &curTargs);
		Lst_Destroy(&emptyPath, Dir_Destroy);
	    } else {
		/*
		 * No wildcards, but we want to avoid code duplication,
		 * so create a list with the word on it.
		 */
		Lst_AtEnd(&curTargs, line);
	    }

	    while ((targName = (char *)Lst_DeQueue(&curTargs)) != NULL) {
		if (!Suff_IsTransform(targName)) {
		    gn = Targ_FindNode(targName, NULL, TARG_CREATE);
		} else {
		    gn = Suff_AddTransform(targName);
		}

		if (gn != NULL)
		    Lst_AtEnd(&targets, gn);
	    }
	} else if (specType == ExPath && *line != '.' && *line != '\0') {
	    Parse_Error(PARSE_WARNING, "Extra target (%s) ignored", line);
	}

	*cp = savec;
	/*
	 * If it is a special type and not .PATH, it's the only target we
	 * allow on this line...
	 */
	if (specType != Not && specType != ExPath) {
	    Boolean warn = FALSE;

	    while (*cp != '!' && *cp != ':' && *cp) {
		if (*cp != ' ' && *cp != '\t') {
		    warn = TRUE;
		}
		cp++;
	    }
	    if (warn) {
		Parse_Error(PARSE_WARNING, "Extra target ignored");
	    }
	} else {
	    while (*cp && isspace(*cp)) {
		cp++;
	    }
	}
	line = cp;
    } while (*line != '!' && *line != ':' && *line);

    /*
     * Don't need the list of target names anymore...
     */
    Lst_Destroy(&curTargs, NOFREE);

    if (!Lst_IsEmpty(&targets)) {
	switch (specType) {
	    default:
		Parse_Error(PARSE_WARNING, "Special and mundane targets don't mix. Mundane ones ignored");
		break;
	    case Default:
	    case Begin:
	    case End:
	    case Interrupt:
		/* These four create nodes on which to hang commands, so
		 * targets shouldn't be empty...  */
	    case Not:
		/* Nothing special here -- targets can be empty if it wants.  */
		break;
	}
    }

    /* Have now parsed all the target names. Must parse the operator next. The
     * result is left in op .  */
    if (*cp == '!') {
	op = OP_FORCE;
    } else if (*cp == ':') {
	if (cp[1] == ':') {
	    op = OP_DOUBLEDEP;
	    cp++;
	} else {
	    op = OP_DEPENDS;
	}
    } else {
	Parse_Error(PARSE_FATAL, "Missing dependency operator");
	return;
    }

    cp++;			/* Advance beyond operator */

    Lst_Find(&targets, ParseDoOp, &op);

    /*
     * Get to the first source
     */
    while (*cp && isspace(*cp)) {
	cp++;
    }
    line = cp;

    /*
     * Several special targets take different actions if present with no
     * sources:
     *	a .SUFFIXES line with no sources clears out all old suffixes
     *	a .PRECIOUS line makes all targets precious
     *	a .IGNORE line ignores errors for all targets
     *	a .SILENT line creates silence when making all targets
     *	a .PATH removes all directories from the search path(s).
     */
    if (!*line) {
	switch (specType) {
	    case Suffixes:
		Suff_ClearSuffixes();
		break;
	    case Precious:
		allPrecious = TRUE;
		break;
	    case Ignore:
		ignoreErrors = TRUE;
		break;
	    case Silent:
		beSilent = TRUE;
		break;
	    case ExPath:
		Lst_Every(&paths, ParseClearPath);
		break;
	    default:
		break;
	}
    } else if (specType == MFlags) {
	/*
	 * Call on functions in main.c to deal with these arguments and
	 * set the initial character to a null-character so the loop to
	 * get sources won't get anything
	 */
	Main_ParseArgLine(line);
	*line = '\0';
    } else if (specType == ExShell) {
	if (Job_ParseShell(line) != SUCCESS) {
	    Parse_Error(PARSE_FATAL, "improper shell specification");
	    return;
	}
	*line = '\0';
    } else if (specType == NotParallel || specType == SingleShell) {
	*line = '\0';
    }

    /*
     * NOW GO FOR THE SOURCES
     */
    if (specType == Suffixes || specType == ExPath ||
	specType == Includes || specType == Libs ||
	specType == Null) {
	while (*line) {
	    /*
	     * If the target was one that doesn't take files as its sources
	     * but takes something like suffixes, we take each
	     * space-separated word on the line as a something and deal
	     * with it accordingly.
	     *
	     * If the target was .SUFFIXES, we take each source as a
	     * suffix and add it to the list of suffixes maintained by the
	     * Suff module.
	     *
	     * If the target was a .PATH, we add the source as a directory
	     * to search on the search path.
	     *
	     * If it was .INCLUDES, the source is taken to be the suffix of
	     * files which will be #included and whose search path should
	     * be present in the .INCLUDES variable.
	     *
	     * If it was .LIBS, the source is taken to be the suffix of
	     * files which are considered libraries and whose search path
	     * should be present in the .LIBS variable.
	     *
	     * If it was .NULL, the source is the suffix to use when a file
	     * has no valid suffix.
	     */
	    char  savec;
	    while (*cp && !isspace(*cp)) {
		cp++;
	    }
	    savec = *cp;
	    *cp = '\0';
	    switch (specType) {
		case Suffixes:
		    Suff_AddSuffix(line);
		    break;
		case ExPath:
		    Lst_ForEach(&paths, ParseAddDir, line);
		    break;
		case Includes:
		    Suff_AddInclude(line);
		    break;
		case Libs:
		    Suff_AddLib(line);
		    break;
		case Null:
		    Suff_SetNull(line);
		    break;
		default:
		    break;
	    }
	    *cp = savec;
	    if (savec != '\0') {
		cp++;
	    }
	    while (*cp && isspace(*cp)) {
		cp++;
	    }
	    line = cp;
	}
	Lst_Destroy(&paths, NOFREE);
    } else {
	while (*line) {
	    /*
	     * The targets take real sources, so we must beware of archive
	     * specifications (i.e. things with left parentheses in them)
	     * and handle them accordingly.
	     */
	    while (*cp && !isspace(*cp)) {
		if (*cp == '(' && cp > line && cp[-1] != '$') {
		    /*
		     * Only stop for a left parenthesis if it isn't at the
		     * start of a word (that'll be for variable changes
		     * later) and isn't preceded by a dollar sign (a dynamic
		     * source).
		     */
		    break;
		} else {
		    cp++;
		}
	    }

	    if (*cp == '(') {
		GNode	  *gn;
		LIST	  sources; /* list of archive source names after
				    * expansion */

		Lst_Init(&sources);
		if (Arch_ParseArchive(&line, &sources, NULL) != SUCCESS) {
		    Parse_Error(PARSE_FATAL,
				 "Error in source archive spec \"%s\"", line);
		    return;
		}

		while ((gn = (GNode *)Lst_DeQueue(&sources)) != NULL)
		    ParseDoSrc(tOp, gn->name, &curSrcs);
		cp = line;
	    } else {
		if (*cp) {
		    *cp = '\0';
		    cp += 1;
		}

		ParseDoSrc(tOp, line, &curSrcs);
	    }
	    while (*cp && isspace(*cp)) {
		cp++;
	    }
	    line = cp;
	}
    }

    if (mainNode == NULL) {
	/* If we have yet to decide on a main target to make, in the
	 * absence of any user input, we want the first target on
	 * the first dependency line that is actually a real target
	 * (i.e. isn't a .USE or .EXEC rule) to be made.  */
	Lst_Find(&targets, ParseFindMain, NULL);
    }

    /* Finally, destroy the list of sources.  */
    Lst_Destroy(&curSrcs, NOFREE);
}

/*-
 *---------------------------------------------------------------------
 * Parse_IsVar	--
 *	Return TRUE if the passed line is a variable assignment. A variable
 *	assignment consists of a single word followed by optional whitespace
 *	followed by either a += or an = operator.
 *	This function is used both by the Parse_File function and main when
 *	parsing the command-line arguments.
 *
 * Results:
 *	TRUE if it is. FALSE if it ain't
 *---------------------------------------------------------------------
 */
Boolean
Parse_IsVar(line)
    char  *line;	/* the line to check */
{
    Boolean wasSpace = FALSE;	/* set TRUE if found a space */
    Boolean haveName = FALSE;	/* Set TRUE if have a variable name */
    int level = 0;
#define ISEQOPERATOR(c) \
	((c) == '+' || (c) == ':' || (c) == '?' || (c) == '!')

    for (; *line != '=' || level != 0; line++)
	switch (*line) {
	case '\0':
	    /* end-of-line -- can't be a variable assignment.  */
	    return FALSE;

	case ' ':
	case '\t':
	    /*
	     * there can be as much white space as desired so long as there is
	     * only one word before the operator
	     */
	    wasSpace = TRUE;
	    break;

	case '(':
	case '{':
	    level++;
	    break;

	case '}':
	case ')':
	    level--;
	    break;

	default:
	    if (wasSpace && haveName) {
		    if (ISEQOPERATOR(*line)) {
			/* We must have a finished word.  */
			if (level != 0)
			    return FALSE;

			/* When an = operator [+?!:] is found, the next
			 * character must be an = or it ain't a valid
			 * assignment.	*/
			if (line[1] == '=')
			    return haveName;
			/* This is a shell command.  */
			if (FEATURES(FEATURE_SUNSHCMD) && 
			    strncmp(line, ":sh", 3) == 0)
			    return haveName;
		    }
		    /* This is the start of another word, so not assignment.  */
		    return FALSE;
	    }
	    else {
		haveName = TRUE;
		wasSpace = FALSE;
	    }
	    break;
	}

    return haveName;
}

static const char *
find_op1(p)
    const char *p;
{
    for(;; p++) {
    	if (*p == '=' || isspace(*p) || *p == '$')
	    break;
	if (p[1] == '=' && (*p == '?' || *p == ':' || *p == '!' || *p == '+'))
	    break;
	if (p[0] == ':' && p[1] == 's' && p[2] == 'h')
	    break;
    }
    return p;
}

static const char *
find_op2(p)
    const char *p;
{
    for(;; p++) {
    	if (*p == '=' || isspace(*p) || *p == '$')
	    break;
	if (p[1] == '=' && (*p == '?' || *p == ':' || *p == '!' || *p == '+'))
	    break;
    }
    return p;
}

/*-
 *---------------------------------------------------------------------
 * Parse_DoVar	--
 *	Take the variable assignment in the passed line and do it in the
 *	global context.
 *
 *	Note: There is a lexical ambiguity with assignment modifier characters
 *	in variable names. This routine interprets the character before the =
 *	as a modifier. Therefore, an assignment like
 *	    C++=/usr/bin/CC
 *	is interpreted as "C+ +=" instead of "C++ =".
 *
 * Side Effects:
 *	the variable structure of the given variable name is altered in the
 *	global context.
 *---------------------------------------------------------------------
 */
void
Parse_DoVar(line, ctxt)
    const char	    *line;	/* a line guaranteed to be a variable
				 * assignment. This reduces error checks */
    GSymT	    *ctxt;    /* Context in which to do the assignment */
{
    const char	    *end;
    const char	    *arg;
    enum {
	VAR_SUBST, VAR_APPEND, VAR_SHELL, VAR_NORMAL
    }		    type;	/* Type of assignment */
    struct Name	    name;

    end = Var_Name_Get(line, &name, (SymTable *)ctxt, TRUE,
	FEATURES(FEATURE_SUNSHCMD) ? find_op1 : find_op2);

    while (isspace(*end))
    	end++;

    /* Check operator type.  */
    switch (*end) {
	case '+':
	    type = VAR_APPEND;
	    break;

	case '?':
	    /* If the variable already has a value, we don't do anything.  */
	    if (Var_Value_interval(name.s, name.e) != NULL) {
	    	Var_Name_Free(&name);
		return;
	    }
	    type = VAR_NORMAL;
	    break;

	case ':':
	    if (FEATURES(FEATURE_SUNSHCMD) && strncmp(end, ":sh", 3) == 0)
		type = VAR_SHELL;
	    else 
		type = VAR_SUBST;
	    break;

	case '!':
	    type = VAR_SHELL;
	    break;

	default:
	    type = VAR_NORMAL;
	    break;
    }

    /* Find operator itself and go over it.  */
    arg = end;
    while (*arg != '=')
    	arg++;
    arg++;
    while (isspace(*arg))
	arg++;

    if (type == VAR_APPEND)
	Var_Append_interval(name.s, name.e, arg, ctxt);
    else if (type == VAR_SUBST) {
	char *sub;
	/*
	 * Allow variables in the old value to be undefined, but leave their
	 * invocation alone -- this is done by forcing oldVars to be false.
	 * XXX: This can cause recursive variables, but that's not hard to do,
	 * and this allows someone to do something like
	 *
	 *  CFLAGS = $(.INCLUDES)
	 *  CFLAGS := -I.. $(CFLAGS)
	 *
	 * And not get an error.
	 */
	Boolean   oldOldVars = oldVars;

	oldVars = FALSE;
	/* ensure the variable is set to something to avoid `variable
	 * is recursive' errors.  */
	if (Var_Value_interval(name.s, name.e) == NULL)
	    Var_Set_interval(name.s, name.e, "", ctxt);

	sub = Var_Subst(arg, (SymTable *)ctxt, FALSE);
	oldVars = oldOldVars;

	Var_Set_interval(name.s, name.e, sub, ctxt);
	free(sub);
    } else if (type == VAR_SHELL) {
	char *res, *err;

	if (strchr(arg, '$') != NULL) {
	    char *sub;
	    /* There's a dollar sign in the command, so perform variable
	     * expansion on the whole thing. */
	    sub = Var_Subst(arg, NULL, TRUE);
	    res = Cmd_Exec(sub, &err);
	    free(sub);
	} else
	    res = Cmd_Exec(arg, &err);

	Var_Set_interval(name.s, name.e, res, ctxt);
	free(res);

	if (err)
	    Parse_Error(PARSE_WARNING, err, arg);

    } else
	/* Normal assignment -- just do it.  */
	Var_Set_interval(name.s, name.e, arg, ctxt);
    Var_Name_Free(&name);
}


/*-
 * ParseAddCmd	--
 *	Lst_ForEach function to add a command line to all targets
 *
 * Side Effects:
 *	A new element is added to the commands list of the node.
 */
static void
ParseAddCmd(gnp, cmd)
    void *gnp;	/* the node to which the command is to be added */
    void *cmd;	/* the command to add */
{
    GNode *gn = (GNode *)gnp;
    /* if target already supplied, ignore commands */
    if (!(gn->type & OP_HAS_COMMANDS)) {
	Lst_AtEnd(&gn->commands, cmd);
	if (!gn->lineno) {
	    gn->lineno = Parse_Getlineno();
	    gn->fname = Parse_Getfilename();
	}
    }
}

/*-
 *-----------------------------------------------------------------------
 * ParseHasCommands --
 *	Callback procedure for Parse_File when destroying the list of
 *	targets on the last dependency line. Marks a target as already
 *	having commands if it does, to keep from having shell commands
 *	on multiple dependency lines.
 *
 * Side Effects:
 *	OP_HAS_COMMANDS may be set for the target.
 *-----------------------------------------------------------------------
 */
static void
ParseHasCommands(gnp)
    void *gnp;	    /* Node to examine */
{
    GNode *gn = (GNode *)gnp;
    if (!Lst_IsEmpty(&gn->commands)) {
	gn->type |= OP_HAS_COMMANDS;
    }
}

/*-
 *-----------------------------------------------------------------------
 * Parse_AddIncludeDir --
 *	Add a directory to the path searched for included makefiles
 *	bracketed by double-quotes. Used by functions in main.c
 *-----------------------------------------------------------------------
 */
void
Parse_AddIncludeDir(dir)
    const char	*dir;	/* The name of the directory to add */
{
    Dir_AddDir(&parseIncPath, dir, NULL);
}

/*-
 *---------------------------------------------------------------------
 * ParseDoInclude  --
 *	Push to another file.
 *
 *	The input is the line minus the #include. A file spec is a string
 *	enclosed in <> or "". The former is looked for only in sysIncPath.
 *	The latter in . and the directories specified by -I command line
 *	options
 *
 * Side Effects:
 *	old parse context is pushed on the stack, new file becomes
 *	current context.
 *---------------------------------------------------------------------
 */
static void
ParseDoInclude(file)
    char	  *file;	/* file specification */
{
    char	  endc; 	/* the character which ends the file spec */
    char	  *cp;		/* current position in file spec */
    Boolean	  isSystem;	/* TRUE if makefile is a system makefile */

    /* Skip to delimiter character so we know where to look.  */
    while (*file == ' ' || *file == '\t')
	file++;

    if (*file != '"' && *file != '<') {
	Parse_Error(PARSE_FATAL,
	    ".include filename must be delimited by '\"' or '<'");
	return;
    }

    /* Set the search path on which to find the include file based on the
     * characters which bracket its name. Angle-brackets imply it's
     * a system Makefile while double-quotes imply it's a user makefile */
    if (*file == '<') {
	isSystem = TRUE;
	endc = '>';
    } else {
	isSystem = FALSE;
	endc = '"';
    }

    /* Skip to matching delimiter.  */
    for (cp = ++file; *cp != endc; cp++) {
	if (*cp == '\0') {
	    Parse_Error(PARSE_FATAL,
		     "Unclosed %cinclude filename. '%c' expected",
		     '.', endc);
	    return;
	}
    }
    ParseLookupIncludeFile(file, cp, isSystem, TRUE);
}

/*-
 *---------------------------------------------------------------------
 * ParseTraditionalInclude  --
 *	Push to another file.
 *
 *	The input is the line minus the "include".  The file name is
 *	the string following the "include".
 *
 * Side Effects:
 *	old parse context is pushed on the stack, new file becomes
 *	current context.
 *
 *	XXX May wish to support multiple files and wildcards ?
 *---------------------------------------------------------------------
 */
static void
ParseTraditionalInclude(file)
    char	  *file;	/* file specification */
{
    char	  *cp;		/* current position in file spec */

    /* Skip over whitespace.  */
    while (isspace(*file))
	file++;
    if (*file == '\0') {
	Parse_Error(PARSE_FATAL,
		     "Filename missing from \"include\"");
	return;
    }
    /* Skip to end of line or next whitespace.	*/
    for (cp = file; *cp != '\0' && !isspace(*cp);)
	cp++;

    ParseLookupIncludeFile(file, cp, TRUE, TRUE);
}

/*-
 *---------------------------------------------------------------------
 * ParseConditionalInclude  --
 *	May push to another file.
 *
 *	No error if the file does not exist.
 *	See ParseTraditionalInclude otherwise.
 *---------------------------------------------------------------------
 */
static void
ParseConditionalInclude(file)
    char	  *file;	/* file specification */
{
    char	  *cp;		/* current position in file spec */

    /* Skip over whitespace.  */
    while (isspace(*file))
	file++;
    if (*file == '\0') {
	Parse_Error(PARSE_FATAL,
		     "Filename missing from \"include\"");
	return;
    }
    /* Skip to end of line or next whitespace.	*/
    for (cp = file; *cp != '\0' && !isspace(*cp);)
	cp++;

    ParseLookupIncludeFile(file, cp, TRUE, FALSE);
}

/* Common part to lookup and read an include file.  */
static void
ParseLookupIncludeFile(spec, endSpec, isSystem, errIfNotFound)
    char *spec;
    char *endSpec;
    Boolean isSystem;
    Boolean errIfNotFound;
{
    char *file;
    char *fullname;
    char endc;

    /* Substitute for any variables in the file name before trying to
     * find the thing.	*/
    endc = *endSpec;
    *endSpec = '\0';
    file = Var_Subst(spec, NULL, FALSE);
    *endSpec = endc;

    /* Now that we know the file name and its search path, we attempt to
     * find the durn thing. NULL indicates the file still hasn't been
     * found.  */
    fullname = NULL;

    /* Handle non-system non-absolute files... */
    if (!isSystem && file[0] != '/') {
	/* ... by first searching relative to the including file's
	 * location. We don't want to cd there, of course, so we
	 * just tack on the old file's leading path components
	 * and call Dir_FindFile to see if we can locate the beast.  */
	char	  *slash;

	slash = strrchr(Parse_Getfilename(), '/');
	if (slash != NULL) {
	    char *base, *newName;

	    base = interval_dup(Parse_Getfilename(), slash);
	    newName = str_concat(base, file, '/');
	    free(base);
	    fullname = Dir_FindFile(newName, &parseIncPath);
	    if (fullname == NULL)
		fullname = Dir_FindFile(newName, &dirSearchPath);
	    free(newName);
	}
    }

    /* Now look first on the -I search path, then on the .PATH
     * search path, if not found in a -I directory.
     * XXX: Suffix specific?  */
    if (fullname == NULL)
	fullname = Dir_FindFile(file, &parseIncPath);
    if (fullname == NULL)
	fullname = Dir_FindFile(file, &dirSearchPath);

    /* Still haven't found the makefile. Look for it on the system
     * path as a last resort.  */
    if (fullname == NULL)
	fullname = Dir_FindFile(file, &sysIncPath);

    if (fullname == NULL && errIfNotFound)
	    Parse_Error(PARSE_FATAL, "Could not find %s", file);
	

    free(file);

    if (fullname != NULL) {
	FILE *f;

	f = fopen(fullname, "r");
	if (f == NULL && errIfNotFound) {
	    Parse_Error(PARSE_FATAL, "Cannot open %s", fullname);
	} else {
	    /* Once we find the absolute path to the file, we push the current
	     * stream to the includes stack, and start reading from the new
	     * file.  We set up the file name to be its absolute name so that
	     * error messages are informative.	*/
	    Parse_FromFile(fullname, f);
	}
    }
}




/* Strip comments from the line. May return either a copy of the line, or
 * the line itself.  */
static char *
strip_comments(copy, line)
    Buffer copy;
    const char *line;
{
    const char *comment;
    const char *p;

    comment = strchr(line, '#');
    assert(comment != line);
    if (comment == NULL)
	return (char *)line;
    else {
	Buf_Reset(copy);

	for (p = line; *p != '\0'; p++) {
	    if (*p == '\\') {
		if (p[1] == '#') {
		    Buf_AddInterval(copy, line, p);
		    Buf_AddChar(copy, '#');
		    line = p+2;
		}
		if (p[1] != '\0')
		    p++;
	    } else if (*p == '#')
		break;
	}
	Buf_AddInterval(copy, line, p);
	Buf_KillTrailingSpaces(copy);
	return Buf_Retrieve(copy);
    }
}

static Boolean
ParseIsCond(linebuf, copy, line)
    Buffer	linebuf;
    Buffer	copy;
    char	*line;
{

    char	*stripped;

    while (*line != '\0' && isspace(*line))
	line++;

    /* The line might be a conditional. Ask the conditional module
     * about it and act accordingly.  */
    switch (Cond_Eval(line)) {
    case COND_SKIP:
	/* Skip to next conditional that evaluates to COND_PARSE.  */
	do {
	    line = ParseSkipGetLine(linebuf);
	    if (line != NULL) {
		while (*line != '\0' && isspace(*line))
		    line++;
		    stripped = strip_comments(copy, line);
	    }
	} while (line != NULL && Cond_Eval(stripped) != COND_PARSE);
	/* FALLTHROUGH */
    case COND_PARSE:
	return TRUE;
    default:
	break;
    }

    {
    For *loop;

    loop = For_Eval(line);
    if (loop != NULL) {
	Boolean ok;
	do {
	    /* Find the matching endfor.  */
	    line = ParseGetLoopLine(linebuf);
	    if (line == NULL) {
		Parse_Error(PARSE_FATAL,
			 "Unexpected end of file in for loop.\n");
		return FALSE;
	    }
	    ok = For_Accumulate(loop, line);
	} while (ok);
	For_Run(loop);
	return TRUE;
    }
    }

    if (strncmp(line, "include", 7) == 0) {
	ParseDoInclude(line + 7);
	return TRUE;
    } else if (strncmp(line, "undef", 5) == 0) {
	char *cp;

	line+=5;
	while (*line != '\0' && isspace(*line))
	    line++;
	for (cp = line; !isspace(*cp) && *cp != '\0';)
	    cp++;
	*cp = '\0';
	Var_Delete(line);
	return TRUE;
    }
    return FALSE;
}

/*-
 *-----------------------------------------------------------------------
 * ParseFinishDependency --
 *	Handle the end of a dependency group.
 *
 * Side Effects:
 *	'targets' list destroyed.
 *
 *-----------------------------------------------------------------------
 */
static void
ParseFinishDependency()
{
    Lst_Every(&targets, Suff_EndTransform);
    Lst_Destroy(&targets, ParseHasCommands);
}

static void
ParseDoCommands(line)
    const char *line;
{
    /* add the command to the list of
     * commands of all targets in the dependency spec */
    char *cmd = estrdup(line);

    Lst_ForEach(&targets, ParseAddCmd, cmd);
#ifdef CLEANUP
    Lst_AtEnd(&targCmds, cmd);
#endif
}

/*-
 *---------------------------------------------------------------------
 * Parse_File --
 *	Parse a file into its component parts, incorporating it into the
 *	current dependency graph. This is the main function and controls
 *	almost every other function in this module
 *
 * Side Effects:
 *	Loads. Nodes are added to the list of all targets, nodes and links
 *	are added to the dependency graph. etc. etc. etc.
 *---------------------------------------------------------------------
 */
void
Parse_File(name, stream)
    char	  *name;	/* the name of the file being read */
    FILE	  *stream;	/* Stream open to makefile to parse */
{
    char	  *cp,		/* pointer into the line */
		  *line;	/* the line we're working on */
    Boolean	  inDependency; /* true if currently in a dependency
				 * line or its commands */

    BUFFER	  buf;
    BUFFER	  copy;

    Buf_Init(&buf, MAKE_BSIZE);
    Buf_Init(&copy, MAKE_BSIZE);
    inDependency = FALSE;
    Parse_FromFile(name, stream);

    do {
	while ((line = ParseReadLine(&buf)) != NULL) {
	    if (*line == '\t') {
		if (inDependency)
		    ParseDoCommands(line+1);
		else
		    Parse_Error(PARSE_FATAL,
			"Unassociated shell command \"%s\"",
			 line);
	    } else {
		char *stripped;
		stripped = strip_comments(&copy, line);
		if (*stripped == '.' && ParseIsCond(&buf, &copy, stripped+1))
		    ;
		else if (FEATURES(FEATURE_SYSVINCLUDE) && 
		    strncmp(stripped, "include", 7) == 0 &&
		    isspace(stripped[7]) &&
		    strchr(stripped, ':') == NULL) {
		    /* It's an S3/S5-style "include".  */
			ParseTraditionalInclude(stripped + 7);
		} else if (FEATURES(FEATURE_CONDINCLUDE) &&
		    (*stripped == '-' || *stripped == 's') &&
		    strncmp(stripped+1, "include", 7) == 0 && 
		    isspace(stripped[8]) &&
		    strchr(stripped, ':') == NULL) {
		    	ParseConditionalInclude(stripped+8);
		} else {
		    char *dep;

		    if (inDependency)
			ParseFinishDependency();
		    if (Parse_IsVar(stripped)) {
			inDependency = FALSE;
			Parse_DoVar(stripped, VAR_GLOBAL);
		    } else {
			size_t pos;
			char *end;

			/* Need a new list for the target nodes.  */
			Lst_Init(&targets);
			inDependency = TRUE;

			dep = NULL;
			/* First we need to find eventual dependencies */
			pos = strcspn(stripped, ":!");
			/* go over :!, and find ;  */
			if (stripped[pos] != '\0' &&
			    (end = strchr(stripped+pos+1, ';')) != NULL) {
				if (line != stripped)
				    /* find matching ; in original... The
				     * original might be slightly longer.  */
				    dep = strchr(line+(end-stripped), ';');
				else
				    dep = end;
				/* kill end of line. */
				*end = '\0';
			}
			/* We now know it's a dependency line so it needs to
			 * have all variables expanded before being parsed.
			 * Tell the variable module to complain if some
			 * variable is undefined... */
			cp = Var_Subst(stripped, NULL, TRUE);
			ParseDoDependency(cp);
			free(cp);

			/* Parse dependency if it's not empty. */
			if (dep != NULL) {
			    do {
				dep++;
			    } while (isspace(*dep));
			    if (*dep != '\0')
				ParseDoCommands(dep);
			}
		    }
		}
	    }
	}
    } while (Parse_NextFile());

    if (inDependency)
	ParseFinishDependency();
    /* Make sure conditionals are clean.  */
    Cond_End();

    Finish_Errors();
    Buf_Destroy(&buf);
    Buf_Destroy(&copy);
}

/*-
 *---------------------------------------------------------------------
 * Parse_Init --
 *	initialize the parsing module
 *
 * Side Effects:
 *	the parseIncPath list is initialized...
 *---------------------------------------------------------------------
 */
void
Parse_Init()
{
    mainNode = NULL;
    Lst_Init(&parseIncPath);
    Lst_Init(&sysIncPath);
    Lst_Init(&targets);
#ifdef CLEANUP
    LowParse_Init();
    Lst_Init(&targCmds);
    Lst_Init(&fileNames);
#endif
}

void
Parse_End()
{
#ifdef CLEANUP
    Lst_Destroy(&targCmds, (SimpleProc)free);
    Lst_Destroy(&fileNames, (SimpleProc)free);
    Lst_Destroy(&targets, NOFREE);
    Lst_Destroy(&sysIncPath, Dir_Destroy);
    Lst_Destroy(&parseIncPath, Dir_Destroy);
    LowParse_End();
#endif
}


/*-
 *-----------------------------------------------------------------------
 * Parse_MainName --
 *	Return a Lst of the main target to create for main()'s sake. If
 *	no such target exists, we Punt with an obnoxious error message.
 *
 * Side effect:
 *	Add the node to create to the list.
 *-----------------------------------------------------------------------
 */
void
Parse_MainName(listmain)
    Lst 	  listmain;	/* result list */
{

    if (mainNode == NULL) {
	Punt("no target to make.");
	/*NOTREACHED*/
    } else if (mainNode->type & OP_DOUBLEDEP) {
	Lst_AtEnd(listmain, mainNode);
	Lst_Concat(listmain, &mainNode->cohorts);
    }
    else
	Lst_AtEnd(listmain, mainNode);
}

