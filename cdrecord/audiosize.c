/* @(#)audiosize.c	1.13 99/10/17 Copyright 1998 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)audiosize.c	1.13 99/10/17 Copyright 1998 J. Schilling";
#endif
/*
 *	Copyright (c) 1998 J. Schilling
 *
 *	First .vaw implementation made by Dave Platt <dplatt@iq.nc.com>
 *	Current .wav implementation with additional help from Heiko Ei�feld.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <statdefs.h>
#include <unixstd.h>
#include <standard.h>
#include <utypes.h>
#include <strdefs.h>
#include <intcvt.h>

#include <scg/scgcmd.h>
#include "auheader.h"

typedef struct {
	Uchar	magic[4];
	Uchar	hdr_size[4];
	Uchar	data_size[4];
	Uchar	encoding[4];
	Uchar	sample_rate[4];
	Uchar	channels[4];
} sun_au_t;

#define	SUN_AU_MAGIC		".snd"
#define	SUN_AU_UNKNOWN_LEN	((Uint)~0)
#define	SUN_AU_ULAW8		1		/* American ISDN Telephonie  */
#define	SUN_AU_LINEAR8		2		/* Linear PCM 8 bit/channel  */
#define	SUN_AU_LINEAR16		3		/* Linear PCM 16 bit/channel */
#define	SUN_AU_LINEAR24		4		/* Linear PCM 24 bit/channel */
#define	SUN_AU_LINEAR32		5		/* Linear PCM 32 bit/channel */
#define	SUN_AU_FLOAT		6		/* 32 bit IEEE floatingpoint */
#define	SUN_AU_DOUBLE		7		/* 64 bit IEEE floatingpoint */
#define	SUN_AU_G721		23		/* 4 bit CCITT G.721 ADPCM   */
#define	SUN_AU_G722		24		/* CCITT G.722 ADPCM	     */
#define	SUN_AU_G723_3		25		/* 3 bit CCITT G.723 ADPCM   */
#define	SUN_AU_G723_5		26		/* 5 bit CCITT G.723 ADPCM   */
#define	SUN_AU_ALAW8		27		/* International ISDN Tel.   */

typedef struct {
	Uchar	ckid[4];
	Uchar	cksize[4];
} chunk_t;

typedef struct {
	Uchar	wave[4];
} riff_chunk;

typedef struct {
	Uchar	fmt_tag[2];
	Uchar	channels[2];
	Uchar	sample_rate[4];
	Uchar	av_byte_rate[4];
	Uchar	block_size[2];
	Uchar	bits_per_sample[2];
} fmt_chunk;

#define	WAV_RIFF_MAGIC		"RIFF"		/* Magic for file format     */
#define	WAV_WAVE_MAGIC		"WAVE"		/* Magic for Waveform Audio  */
#define	WAV_FMT_MAGIC		"fmt "		/* Start of Waveform format  */
#define	WAV_DATA_MAGIC		"data"		/* Start of data chunk	     */
#define	WAV_FORMAT_PCM		0x0001		/* Linear PCM format	     */
#define	WAV_FORMAT_ULAW		0x0101		/* American ISDN Telephonie  */
#define	WAV_FORMAT_ALAW		0x0102		/* International ISDN Tel.   */
#define	WAV_FORMAT_ADPCM	0x0103		/* ADPCM format		     */

#define	le_a_to_u_short(a)	((unsigned short) \
				((((unsigned char*) a)[0]       & 0xFF) | \
				 (((unsigned char*) a)[1] << 8  & 0xFF00)))

#ifdef	__STDC__
#define	le_a_to_u_long(a)	((unsigned long) \
				((((unsigned char*) a)[0]       & 0xFF) | \
				 (((unsigned char*) a)[1] << 8  & 0xFF00) | \
				 (((unsigned char*) a)[2] << 16 & 0xFF0000) | \
				 (((unsigned char*) a)[3] << 24 & 0xFF000000UL)))
#else
#define	le_a_to_u_long(a)	((unsigned long) \
				((((unsigned char*) a)[0]       & 0xFF) | \
				 (((unsigned char*) a)[1] << 8  & 0xFF00) | \
				 (((unsigned char*) a)[2] << 16 & 0xFF0000) | \
				 (((unsigned char*) a)[3] << 24 & 0xFF000000)))
#endif

EXPORT	BOOL	is_auname	__PR((const char *name));
EXPORT	long	ausize		__PR((int f));
EXPORT	BOOL	is_wavname	__PR((const char *name));
EXPORT	long	wavsize		__PR((int f));

EXPORT	BOOL	is_auname(name)
	const	char	*name;
{
	const	char	*p;

	if ((p = strrchr(name, '.')) == NULL)
		return (FALSE);
	return (streql(p, ".au"));
}

EXPORT long
ausize(f)
	int	f;
{
	sun_au_t	hdr;
	struct stat	sb;
	long		mode;
	long		size;
	long		ret = AU_BAD_HEADER;

	/*
	 * First check if a bad guy tries to call ausize()
	 * with an unappropriate file descriptor.
	 * return -1 in this case.
	 */
	if (isatty(f))
		return (-1L);
	if (fstat(f, &sb) < 0)
		return (-1L);
	mode = (long)(sb.st_mode & S_IFMT);
	if (!S_ISREG(mode) && !S_ISBLK(mode) && !S_ISCHR(mode))
		return (-1L);

	if (read(f, &hdr, sizeof(hdr)) != sizeof(hdr))
		goto err;

	if (strncmp((char *)hdr.magic, SUN_AU_MAGIC, 4) != 0)
		goto err;

	ret = AU_BAD_CODING;

	size = a_to_u_4_byte(hdr.encoding);
	if (size != SUN_AU_LINEAR16)
		goto err;

	size = a_to_u_4_byte(hdr.channels);
	if (size != 2)
		goto err;

	size = a_to_u_4_byte(hdr.sample_rate);
	if (size != 44100)
		goto err;

	size = a_to_u_4_byte(hdr.hdr_size);
	if (size < (long)sizeof(hdr) || size > 512)
		goto err;
	lseek(f, (off_t)size, SEEK_SET);

	/*
	 * Most .au files don't seem to honor the data_size field,
	 * so we use the whole file size without the header.
	 */
	size = sb.st_size - size;
	return (size);

err:
	lseek(f, (off_t)0L, SEEK_SET);
	return (ret);
}

EXPORT	BOOL	is_wavname(name)
	const	char	*name;
{
	const	char	*p;

	if ((p = strrchr(name, '.')) == NULL)
		return (FALSE);
	return (streql(p, ".wav") || streql(p, ".WAV"));
}

EXPORT long
wavsize(f)
	int	f;
{
	chunk_t		chunk;
	riff_chunk	riff;
	fmt_chunk	fmt;
	struct stat	sb;
	long		cursor;
	BOOL		gotFormat;
	long		mode;
	long		size;
	long		ret = AU_BAD_HEADER;

	/*
	 * First check if a bad guy tries to call wavsize()
	 * with an unappropriate file descriptor.
	 * return -1 in this case.
	 */

	if (isatty(f))
		return (-1L);
	if (fstat(f, &sb) < 0)
		return (-1L);
	mode = (long)(sb.st_mode & S_IFMT);
	if (!S_ISREG(mode) && !S_ISBLK(mode) && !S_ISCHR(mode))
		return (-1L);

	cursor = 0;
	gotFormat = FALSE;

	for (;;) {
		if (read(f, &chunk, sizeof (chunk)) != sizeof (chunk))
			goto err;
		size = le_a_to_u_long(chunk.cksize);

		if (strncmp((char *)chunk.ckid, WAV_RIFF_MAGIC, 4) == 0) {
			if (read(f, &riff, sizeof (riff)) != sizeof (riff))
				goto err;
			if (strncmp((char *)riff.wave, WAV_WAVE_MAGIC, 4) != 0)
				goto err;
			size = sizeof (riff);

		} else if (strncmp((char *)chunk.ckid, WAV_FMT_MAGIC, 4) == 0) {
			if (size < (long)sizeof (fmt)) goto err;
			if (sizeof (fmt) != read(f, &fmt, sizeof (fmt))) goto err;
			if (le_a_to_u_short(fmt.channels) != 2 ||
			    le_a_to_u_long(fmt.sample_rate) != 44100 ||
			    le_a_to_u_short(fmt.bits_per_sample) != 16) {
				ret = AU_BAD_CODING;
				goto err;
			}
			gotFormat = TRUE;

		} else if (strncmp((char *)chunk.ckid, WAV_DATA_MAGIC, 4) == 0) {
			if (!gotFormat) {
				ret = AU_BAD_CODING;
				goto err;
			}
			if ((long)(cursor + size + sizeof (chunk)) > sb.st_size)
				size = sb.st_size - (cursor  + sizeof (chunk));
			return (size);
		}
		cursor += size + sizeof (chunk);
		lseek(f, cursor, SEEK_SET);	/* Skip over current chunk */
	}
err:
	lseek(f, (off_t)0L, SEEK_SET);
	return (ret);
}
