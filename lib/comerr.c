/* @(#)comerr.c	1.22 00/05/07 Copyright 1985 J. Schilling */
/*
 *	Routines for printing command errors
 *
 *	Copyright (c) 1985 J. Schilling
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <mconfig.h>
#include <stdio.h>
#include <standard.h>
#include <stdxlib.h>
#include <vadefs.h>
#include <strdefs.h>
#include <schily.h>
#ifndef	HAVE_STRERROR
extern	char	*sys_errlist[];
extern	int	sys_nerr;
#endif

EXPORT	int	on_comerr	__PR((void (*fun)(int, void *), void *arg));
EXPORT	void	comerr		__PR((const char *, ...));
EXPORT	void	comerrno	__PR((int, const char *, ...));
EXPORT	int	errmsg		__PR((const char *, ...));
EXPORT	int	errmsgno	__PR((int, const char *, ...));
LOCAL	int	_comerr		__PR((int, int, const char *, va_list));
EXPORT	void	comexit		__PR((int));
EXPORT	char	*errmsgstr	__PR((int));

typedef	struct ex {
	struct ex *next;
	void	(*func) __PR((int, void *));
	void	*arg;
} ex_t;

LOCAL	ex_t	*exfuncs;

EXPORT	int
on_comerr(func, arg)
	void	(*func) __PR((int, void *));
	void	*arg;
{
	ex_t	*fp;

	fp = malloc(sizeof(*fp));
	if (fp == NULL)
		return (-1);

	fp->func = func;
	fp->arg  = arg;
	fp->next = exfuncs;
	exfuncs = fp;
	return (0);
}

/* VARARGS1 */
#ifdef	PROTOTYPES
void comerr(const char *msg, ...)
#else
void comerr(msg, va_alist)
	char	*msg;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	(void)_comerr(TRUE, geterrno(), msg, args);
	/* NOTREACHED */
	va_end(args);
}

/* VARARGS2 */
#ifdef	PROTOTYPES
void comerrno(int err, const char *msg, ...)
#else
void comerrno(err, msg, va_alist)
	int	err;
	char	*msg;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	(void)_comerr(TRUE, err, msg, args);
	/* NOTREACHED */
	va_end(args);
}

/* VARARGS1 */
#ifdef	PROTOTYPES
int errmsg(const char *msg, ...)
#else
int errmsg(msg, va_alist)
	char	*msg;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	ret = _comerr(FALSE, geterrno(), msg, args);
	va_end(args);
	return (ret);
}

/* VARARGS2 */
#ifdef	PROTOTYPES
int errmsgno(int err, const char *msg, ...)
#else
int errmsgno(err, msg, va_alist)
	int	err;
	char	*msg;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	ret = _comerr(FALSE, err, msg, args);
	va_end(args);
	return (ret);
}

LOCAL int _comerr(exflg, err, msg, args)
	int		exflg;
	int		err;
	const char	*msg;
	va_list		args;
{
	char	errbuf[20];
	char	*errnam;
	char	*prognam = get_progname();
	
	if (err < 0) {
		error("%s: %r", prognam, msg, args);
	} else {
		errnam = errmsgstr(err);
		if (errnam == NULL) {
			(void)sprintf(errbuf, "Error %d", err);
			errnam = errbuf;
		}
		error("%s: %s. %r", prognam, errnam, msg, args);
	}
	if (exflg) {
		comexit(err);
		/* NOTREACHED */
	}
	return(err);
}

EXPORT void
comexit(err)
	int	err;
{
	while (exfuncs) {
		(*exfuncs->func)(err, exfuncs->arg);
		exfuncs = exfuncs->next;
	}
	exit(err);
	/* NOTREACHED */
}

EXPORT char *
errmsgstr(err)
	int	err;
{
#ifdef	HAVE_STRERROR
	return (strerror(err));
#else
	if (err < 0 || err >= sys_nerr) {
		return (NULL);
	} else {
		return (sys_errlist[err]);
	}
#endif
}
