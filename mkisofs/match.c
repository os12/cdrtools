/* @(#)match.c	1.11 00/06/27 joerg */
#ifndef lint
static	char sccsid[] =
	"@(#)match.c	1.11 00/06/27 joerg";
#endif
/*
 * 27-Mar-96: Jan-Piet Mens <jpm@mens.de>
 * added 'match' option (-m) to specify regular expressions NOT to be included
 * in the CD image.
 *
 * Re-written 13-Apr-2000 James Pearson
 * now uses a generic set of routines
 */

#include "config.h"
#include <prototyp.h>
#include <stdio.h>
#include <stdxlib.h>
#include <strdefs.h>
#include "match.h"
#ifdef	USE_LIBSCHILY
#include <standard.h>
#include <schily.h>
#endif

static	int	add_sort_match	__PR((char *fn, int val));

struct match {
	struct match *next;
	char	 *name;
};

typedef struct match match;

static match *mats[MAX_MAT];

static char *mesg[MAX_MAT] = {
	"excluded",
	"excluded ISO-9660",
	"excluded Joliet",
	"hidden attribute ISO-9660",
#ifdef APPLE_HYB
	"excluded HFS",
#endif /* APPLE_HYB */
};

#ifdef SORTING
struct sort_match {
	struct sort_match	*next;
	char			*name;
	int			val;
};

typedef struct sort_match sort_match;

static sort_match	*s_mats;

static int
add_sort_match(fn, val)
	char	*fn;
	int	val;
{
	sort_match *s_mat;

	s_mat = (sort_match *)malloc(sizeof(sort_match));
	if (s_mat == NULL) {
#ifdef	USE_LIBSCHILY
		errmsg("Can't allocate memory for sort filename\n");
#else
		fprintf(stderr,"Can't allocate memory for sort filename\n");
#endif
		return (0);
	}

	if ((s_mat->name = strdup(fn)) == NULL) {
#ifdef	USE_LIBSCHILY
		errmsg("Can't allocate memory for sort filename\n");
#else
		fprintf(stderr,"Can't allocate memory for sort filename\n");
#endif
		return (0);
	}

	/* need to reserve the minimum value for other uses */
	if (val == NOT_SORTED)
		val++;

	s_mat->val = val;
	s_mat->next = s_mats;
	s_mats = s_mat;

	return (1);
}

void
add_sort_list(file)
	char	*file;
{
	FILE	*fp;
	char	name[4096];
	char	*p;
	int	val;

	if ((fp = fopen(file, "r")) == NULL) {
#ifdef	USE_LIBSCHILY
		comerr("Can't open sort file list %s\n", file);
#else
		fprintf(stderr,"Can't open hidden/exclude file list %s\n", file);
		exit (1);
#endif
	}

	while (fgets(name, sizeof(name), fp) != NULL) {
		/*
		 * look for the last space or tab character
		 */
		if ((p = strrchr(name, ' ')) == NULL)
			p = strrchr(name, '\t');
		if (p == NULL) {
#ifdef	USE_LIBSCHILY
			comerrno(EX_BAD, "Incorrect sort file format\n\t%s", name);
#else
			fprintf(stderr, "Incorrect sort file format\n\t%s", name);
#endif
			continue;
		} else {
			*p = '\0';
			val = atoi(++p);
		}
		if (!add_sort_match(name, val)) {
			fclose(fp);
			return;
		}
	}

	fclose(fp);
}

int
sort_matches(fn, val)
	char	*fn;
	int	val;
{
	register sort_match	*s_mat;

	for (s_mat=s_mats; s_mat; s_mat=s_mat->next) {
		if (fnmatch(s_mat->name, fn, FNM_FILE_NAME) != FNM_NOMATCH) {
		        return (s_mat->val); /* found sort value */
		}
	}
	return (val); /* not found - default sort value */
}

void
del_sort()
{
	register sort_match * s_mat, *s_mat1;

	s_mat = s_mats;
	while(s_mat) {
		s_mat1 = s_mat->next;

		free(s_mat->name);
		free(s_mat);

		s_mat = s_mat1;
	}

	s_mats = 0;
}

#endif /* SORTING */


int
gen_add_match(fn, n)
	char	*fn;
	int	n;
{
	match	*mat;

	if (n >= MAX_MAT)
		return (0);

	mat = (match *)malloc(sizeof(match));
	if (mat == NULL) {
#ifdef	USE_LIBSCHILY
		errmsg("Can't allocate memory for %s filename\n", mesg[n]);
#else
		fprintf(stderr,"Can't allocate memory for %s filename\n", mesg[n]);
#endif
		return (0);
	}

	if ((mat->name = strdup(fn)) == NULL) {
#ifdef	USE_LIBSCHILY
		errmsg("Can't allocate memory for %s filename\n", mesg[n]);
#else
		fprintf(stderr,"Can't allocate memory for %s filename\n", mesg[n]);
#endif
		return (0);
	}

	mat->next = mats[n];
	mats[n] = mat;

	return (1);
}

void
gen_add_list(file, n)
	char	*file;
	int	n;
{
	FILE	*fp;
	char	name[4096];

	if ((fp = fopen(file, "r")) == NULL) {
#ifdef	USE_LIBSCHILY
		comerr("Can't open %s file list %s\n", mesg[n], file);
#else
		fprintf(stderr,"Can't open %s file list %s\n", mesg[n], file);
		exit (1);
#endif
	}

	while (fgets(name, sizeof(name), fp) != NULL) {
		/*
		 * strip of '\n'
		*/
		name[strlen(name) - 1] = '\0';
		if (!gen_add_match(name, n)) {
			fclose(fp);
			return;
		}
	}

	fclose(fp);
}

int gen_matches(fn, n)
	char	*fn;
	int	n;
{
	register match * mat;

	if (n >= MAX_MAT)
		return (0);

	for (mat=mats[n]; mat; mat=mat->next) {
		if (fnmatch(mat->name, fn, FNM_FILE_NAME) != FNM_NOMATCH) {
			return (1);	/* found -> excluded filename */
		}
	}
	return (0);			/* not found -> not excluded */
}

int gen_ishidden(n)
	int	n;
{
	if (n >= MAX_MAT)
		return (0);

	return ((int)(mats[n] != 0));
}

void
gen_del_match(n)
	int	n;
{
	register match	*mat;
	register match 	*mat1;

	if (n >= MAX_MAT)
		return;

	mat = mats[n];

	while (mat) {
		mat1 = mat->next;

		free(mat->name);
		free(mat);

		mat = mat1;
	}

	mats[n] = 0;
}
