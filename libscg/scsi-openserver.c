/* @(#)scsi-openserver.c	1.16 00/01/26 Copyright 1998 J. Schilling, Santa Cruz Operation */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-openserver.c	1.16 00/01/26 Copyright 1998 J. Schilling, Santa Cruz Operation";
#endif
/*
 *	Interface for the SCO SCSI implementation.
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 *
 *	Copyright (c) 1998 J. Schilling, Santa Cruz Operation
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

#undef	sense

#include <sys/scsicmd.h>

/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
LOCAL	char	_scg_trans_version[] = "scsi-openserver.c-1.16";	/* The version for this transport*/

#define	MAX_SCG		16		/* Max # of cdrom devices */
#define	MAX_TGT		16		/* Not really needed      */
#define	MAX_LUN		8		/* Not really needed      */

#define	MAX_DMA		(32*1024)	/* Maybe even more ???    */
#define	MAXPATH	       256			/* max length of devicepath  */
#define MAXLINE		80			/* max length of input line  */
#define MAXSCSI		99			/* max number of mscsi lines */
#define	MAXDRVN		10			/* max length of drivername  */

#define DEV_DIR		"/tmp"
#define	DEV_NAME	"scg.s%1dt%1dl%1d"

		/* We will only deal with disks, cdroms and tapes !	*/
		/* Sflp devices (floptical) 21Mb scsi floppy devices	*/
		/* will be ignored. Hmm, I can't remember that I ever 	*/
		/* have seen one ;^)					*/

#define	DEV_ROOT	"/dev/rdsk/0s0"
#define DEV_SDSK	"/dev/rdsk/%ds0"
#define DEV_SROM	"/dev/rcd%d"
#define	DEV_STP		"/dev/xStp%d"

#define	SCAN_DEV	"%s%s%d%d%d%d"

#define	SCSI_CFG	"/etc/sconf -r"		/* no. of configured devices  */
#define	SCSI_DEV	"/etc/sconf -g %d"	/* read line 'n' of mscsi tbl */

#define	DRV_ATAPI	"wd"

typedef struct scg2sdi {

	int	valid;
	int	open;
	int	atapi;
	int	fd;
	int	lmscsi;

} scg2sdi_t;

LOCAL	scg2sdi_t	sdidevs [MAX_SCG][MAX_TGT][MAX_LUN];

typedef struct amscsi {
	char	typ[MAXDRVN];
	char	drv[MAXDRVN];
	int	hba;
	int	bus;
	int	scg;
	int	tgt;
	int	lun;
	char	dev[MAXPATH];

} amscsi_t;

struct scg_local {
	short	scgfiles[MAX_SCG][MAX_TGT][MAX_LUN];
};
#define scglocal(p)	((struct scg_local *)((p)->local)) 

LOCAL	int	sort_mscsi	__PR((const void *l1, const void *l2));
LOCAL	int	openserver_init	__PR((SCSI *scgp));
LOCAL	void	cp_scg2sco	__PR((struct scsicmd2 *sco, struct scg_cmd *scg));
LOCAL	void	cp_sco2scg	__PR((struct scsicmd2 *sco, struct scg_cmd *scg));

/* -------------------------------------------------------------------------
** SCO OpenServer does not have a generic scsi device driver, which can
** be used to access any configured scsi device. 
**
** But we can use the "Srom" scsi peripherial driver (for CD-ROM devices) 
** etc. to send SCSIUSERCMD2's to any target device controlled by the 
** corresponding target driver.
**
** This pass-through implementation for libscg currently allows to 
** handle the following devices classes:
**
**	1. DISK		handled by Sdsk
**	2. CD-ROM	handled by Srom
**	3. TAPES	handled by Stp
**
** NOTE: The libscg OpenServer passthrough routines have changed with 
**       cdrecord-1.8 to enable the -scanbus option. Therefore the
**	 addressing scheme is now the same as used on many other platforms
**	 like Solaris, Linux etc.
**
**   ===============================================================
**   RUN 'cdrecord -scanbus' TO SEE THE DEVICE ADDRESSES YOU CAN USE
**   ===============================================================
**
*/

/*
 * Return version information for the low level SCSI transport code.
 * This has been introduced to make it easier to trace down problems
 * in applications.
 */
EXPORT char *
scg__version(scgp, what)
	SCSI	*scgp;
	int	what;
{
	if (scgp != (SCSI *)0) {
		switch (what) {

		case SCG_VERSION:
			return (_scg_trans_version);
		/*
		 * If you changed this source, you are not allowed to
		 * return "schily" for the SCG_AUTHOR request.
		 */
		case SCG_AUTHOR:
			return (_scg_auth_schily);
		case SCG_SCCS_ID:
			return (__sccsid);
		}
	}
	return ((char *)0);
}

/* ---------------------------------------------------------------
** This routine sorts the amscsi_t lines on the following columns
** in ascending order:
**
**	1. drv  - driver name
**	2. bus  - scsibus per controller
**	3. tgt  - target id of device
**	4. lun  - lun of the device
**
*/


LOCAL int
sort_mscsi(l1, l2)
	const void	*l1;
	const void	*l2;
{
	amscsi_t	*t1 = (amscsi_t *) l1;
	amscsi_t	*t2 = (amscsi_t *) l2;

	if ( strcmp(t1->drv, t2->drv) == 0 ) {
		if (t1->bus < t2->bus) {
			return (-1);

		} else if (t1->bus > t2->bus) {
			return (1);

		} else if (t1->tgt < t2->tgt) {
			return (-1);

		} else if (t1->tgt > t2->tgt) {
			return (1);

		} else if (t1->lun < t2->lun) {
			return (-1);

		} else if (t1->lun > t2->lun) {
			return (1);
		} else {
			return (0);
		}
	} else {
		return (strcmp(t1->drv, t2->drv));
	}
}

/* ---------------------------------------------------------------
** This routine is introduced to find all scsi devices which are
** currently configured into the kernel. This is done by reading
** the dynamic kernel mscsi tables and parse the resulting lines.
** As the output of 'sconf' is not directly usable the information
** found is to be sorted and re-arranged to be used with the libscg
** routines.
**
** NOTE: One problem is currently still not solved ! If you don't
**       have a media in your CD-ROM/CD-Writer we are not able to 
**       do an open() and therefore will set the drive to be not 
**       available (valid=0).
**
**       This will for example cause cdrecord to not list the drive 
**	 in the -scanbus output.
**
*/

LOCAL int 
openserver_init(scgp)
	SCSI	*scgp;
{
	FILE		*cmd;
	int		nscg  = -1, lhba  = -1, lbus = -1;
	int		nSrom = -1, nSdsk = -1, nStp = -1;
	int		atapi, fd, nopen = 0;
	int		pos = 0, len = 0, nlm = 0;
	int		s = 0, t = 0, l = 0;
	int		mscsi;
	char		sconf[MAXLINE];
	char		lines[MAXLINE];
	char		drvid[MAXDRVN];
	amscsi_t	cmtbl[MAXSCSI];
	char		dname[MAXPATH];
	char		**evsave;
extern	char		**environ;


	for (s = 0; s < MAX_SCG; s++) {
		for (t = 0; t < MAX_TGT; t++) {
			for (l = 0; l < MAX_LUN ; l++) {
			  sdidevs[s][t][l].valid  =  0;
			  sdidevs[s][t][l].open   = -1;
			  sdidevs[s][t][l].atapi  = -1;
			  sdidevs[s][t][l].fd     = -1;
			  sdidevs[s][t][l].lmscsi = -1;
			}
		}
	}

	/* read sconf -r and get number of kernel mscsi lines ! */

	evsave = environ;
	environ = 0;
	if ((cmd = popen(SCSI_CFG,"r")) == NULL) {
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Error popen() for \"%s\"",
				SCSI_CFG);
		environ = evsave;
		return (-1);
	}
	environ = evsave;

	if (fgets(lines, MAXLINE, cmd) == NULL) {
		errno = EIO;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Error reading popen() for \"%s\"",
				SCSI_CFG);
		return (-1);
	} else
		nlm = atoi(lines);

	if (scgp->debug) {
		printf("-------------------- \n");
		printf("mscsi lines = %d\n", nlm);
		printf("-------------------- \n");
	}

	if (pclose(cmd) == -1) {
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Error pclose() for \"%s\"",
				SCSI_CFG);
		return (-1);
	}


	for (l = 0; l < nlm; l++) {

		/* initialize cmtbl entry */

		cmtbl[l].hba = -1;
		cmtbl[l].bus = -1;
		cmtbl[l].tgt = -1;
		cmtbl[l].lun = -1;
		cmtbl[l].scg = -1;

		memset( cmtbl[l].typ, '\0', MAXDRVN);
		memset( cmtbl[l].drv, '\0', MAXDRVN);
		memset( cmtbl[l].dev, '\0', MAXDRVN);

		/* read sconf -g 'n' and get line of kernel mscsi table! */
		/* the order the lines will be received in will determine */
		/* the device name we can use to open the device         */

		sprintf(sconf, SCSI_DEV, l + 1); /* enumeration starts with 1 */

		evsave = environ;
		environ = 0;
		if ((cmd = popen(sconf,"r")) == NULL) {
			if (scgp->errstr)
				js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
					"Error popen() for \"%s\"",
					sconf);
			environ = evsave;
			return (-1);
		}
		environ = evsave;

		if (fgets(lines, MAXLINE, cmd) == NULL) 
			break;

		if (pclose(cmd) == -1) {
			if (scgp->errstr)
				js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
					"Error pclose() for \"%s\"",
					sconf);
			return (-1);
		}

		sscanf(lines, SCAN_DEV,  cmtbl[l].typ, 
					 cmtbl[l].drv,
					&cmtbl[l].hba, 
					&cmtbl[l].bus, 
					&cmtbl[l].tgt, 
					&cmtbl[l].lun);

		if ( strstr(cmtbl[l].typ, "Sdsk") != NULL ){
			sprintf(cmtbl[l].dev, DEV_SDSK, ++nSdsk);
		}

		if ( strstr(cmtbl[l].typ, "Srom") != NULL ){
			sprintf(cmtbl[l].dev, DEV_SROM, ++nSrom);
		}

		if ( strstr(cmtbl[l].typ, "Stp") != NULL ){
			sprintf(cmtbl[l].dev, DEV_STP, ++nStp);
		}

		if (scgp->debug) {
			printf("%-4s = %5s(%d,%d,%d,%d) -> %s\n", 
				cmtbl[l].typ, 
				cmtbl[l].drv, 
				cmtbl[l].hba, 
				cmtbl[l].bus, 
				cmtbl[l].tgt, 
				cmtbl[l].lun,
				cmtbl[l].dev);
		}

	}

	if (scgp->debug) {
		printf("-------------------- \n");
		printf("%2d DISK  \n", nSdsk + 1);
		printf("%2d CD-ROM\n", nSrom + 1);
		printf("%2d TAPE  \n", nStp  + 1);
		printf("-------------------- \n");
	}

	/* ok, now let's sort this array of scsi devices	*/

	qsort((void *) cmtbl, nlm, sizeof(amscsi_t), sort_mscsi);

	if (scgp->debug) {
		for (l = 0; l < nlm; l++)
		printf("%-4s = %5s(%d,%d,%d,%d) -> %s\n", 
			cmtbl[l].typ, 
			cmtbl[l].drv, 
			cmtbl[l].hba, 
			cmtbl[l].bus, 
			cmtbl[l].tgt, 
			cmtbl[l].lun,
			cmtbl[l].dev);
		printf("-------------------- \n");
	}



	/* find root disk controller to make it scg 0     	*/

	for (l = 0; l < nlm; l++)
		if (strcmp(cmtbl[l].dev, DEV_ROOT) == 0)
			t = l;


	if (scgp->debug) {
		printf("root = %5s(%d,%d,%d,%d) -> %s\n", 
			cmtbl[t].drv, 
			cmtbl[t].hba, 
			cmtbl[t].bus, 
			cmtbl[t].tgt, 
			cmtbl[t].lun,
			cmtbl[t].dev);
		printf("-------------------- \n");
	}

	/* calculate scg from drv, hba and bus 			*/

	strcpy(drvid, "");

	for (l = 0, s = t; l < nlm; l++, s = (t + l) % nlm) {

		if (strcmp(drvid, cmtbl[s].drv) != 0) {
			strcpy(drvid, cmtbl[s].drv);
			lhba = cmtbl[s].hba;
			lbus = cmtbl[s].bus;
			cmtbl[s].scg = ++nscg;

		} else if (cmtbl[s].hba != lhba) {
			lhba = cmtbl[s].hba;
			lbus = cmtbl[s].bus;
			cmtbl[s].scg = ++nscg;

		} else if (cmtbl[s].bus != lbus) {
			lbus = cmtbl[s].bus;
			cmtbl[s].scg = ++nscg;
		} else {
			cmtbl[s].scg = nscg;
		}
		sdidevs[cmtbl[s].scg][cmtbl[s].tgt][cmtbl[s].lun].valid  = 1;
		sdidevs[cmtbl[s].scg][cmtbl[s].tgt][cmtbl[s].lun].open   = 0;

		if (strcmp(cmtbl[s].drv, DRV_ATAPI) == 0) 
			sdidevs[cmtbl[s].scg][cmtbl[s].tgt][cmtbl[s].lun].atapi = 1;

		sdidevs[cmtbl[s].scg][cmtbl[s].tgt][cmtbl[s].lun].lmscsi = s;

	}


	/* open all device nodes */

	for (s = 0; s < MAX_SCG; s++) {
		for (t = 0; t < MAX_TGT; t++) {
			for (l = 0; l < MAX_LUN ; l++) {

			  if ( sdidevs[s][t][l].valid == 0) 
			  	continue;

			  /* Open pass-through device node */

				mscsi = sdidevs[s][t][l].lmscsi;

				strcpy(dname, cmtbl[mscsi].dev);

	/* ------------------------------------------------------------------
	** NOTE: If we can't open the device, we will set the device invalid!
	** ------------------------------------------------------------------
	*/

				if ((fd = open(dname, O_RDONLY | O_NONBLOCK)) < 0) {
					sdidevs[s][t][l].valid = 0;
					continue;
				}

				if (scgp->debug) {
					printf("%d,%d,%d => %5s(%d,%d,%d,%d) -> %d : %s \n", 
						s, t, l,
						cmtbl[mscsi].drv, 
						cmtbl[mscsi].hba, 
						cmtbl[mscsi].bus, 
						cmtbl[mscsi].tgt, 
						cmtbl[mscsi].lun,
						cmtbl[mscsi].scg,
						cmtbl[mscsi].dev);
				}

				sdidevs[s][t][l].fd   = fd;
				sdidevs[s][t][l].open = 1;
				nopen++;
				scglocal(scgp)->scgfiles[s][t][l] = (short) fd;
			}
		}
	}

	if (scgp->debug) {
		printf("-------------------- \n");
		printf("nopen = %d devices   \n", nopen);
		printf("-------------------- \n");
	}

	return (nopen);
}


EXPORT int
scsi_open(scgp, device, busno, tgt, tlun)
	SCSI	*scgp;
	char	*device;
	int	busno;
	int	tgt;
	int	tlun;
{
	int	f, b, t, l;
	int	nopen = 0;
	char	devname[64];

	if (busno >= MAX_SCG || tgt >= MAX_TGT || tlun >= MAX_LUN) {
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Illegal value for busno, target or lun '%d,%d,%d'",
				busno, tgt, tlun);
		return (-1);
	}

	if (scgp->local == NULL) {
		scgp->local = malloc(sizeof(struct scg_local));
		if (scgp->local == NULL)
			return (0);

		for (b = 0; b < MAX_SCG; b++) {
			for (t = 0; t < MAX_TGT; t++) {
				for (l = 0; l < MAX_LUN ; l++)
					scglocal(scgp)->scgfiles[b][t][l] = (short)-1;
			}
		}
	}

	if (*device != '\0') {          /* we don't allow old dev usage */
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			"Open by 'devname' no longer supported on this OS");
		return (-1);
	} 

	return ( openserver_init(scgp) );

}

EXPORT int
scsi_close(scgp)
	SCSI	*scgp;
{
	register int	f;
	register int	b;
	register int	t;
	register int	l;

	if (scgp->local == NULL)
		return (-1);

	for (b=0; b < MAX_SCG; b++) {
		for (t=0; t < MAX_TGT; t++) {
			for (l=0; l < MAX_LUN ; l++)

				f = scglocal(scgp)->scgfiles[b][t][l];
				if (f >= 0)
					close(f);

				sdidevs[b][t][l].fd    = -1;
				sdidevs[b][t][l].open  =  0;
				sdidevs[b][t][l].valid =  0;

				scglocal(scgp)->scgfiles[b][t][l] = (short)-1;
		}
	}
	return (0);
}

LOCAL long
scsi_maxdma(scgp)
	SCSI	*scgp;
{
	return (MAX_DMA);
}


EXPORT void *
scsi_getbuf(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	if (amt <= 0 || amt > scsi_maxdma(scgp))
		return ((void *)0);
	if (scgp->debug)
		printf("scsi_getbuf: %ld bytes\n", amt);

	scgp->bufbase = valloc((size_t)(amt));

	return (scgp->bufbase);
}

EXPORT void
scsi_freebuf(scgp)
	SCSI	*scgp;
{
	if (scgp->bufbase)
		free(scgp->bufbase);
	scgp->bufbase = NULL;
}

EXPORT
BOOL scsi_havebus(scgp, busno)
	SCSI	*scgp;
	int	busno;
{
	register int	t;
	register int	l;

	if (busno < 0 || busno >= MAX_SCG)
		return (FALSE);

	if (scgp->local == NULL)
		return (FALSE);

	for (t=0; t < MAX_TGT; t++) {
		for (l=0; l < MAX_LUN ; l++)
			if (scglocal(scgp)->scgfiles[busno][t][l] >= 0)
				return (TRUE);
	}
	return (FALSE);
}

EXPORT
int scsi_fileno(scgp, busno, tgt, tlun)
	SCSI	*scgp;
	int	busno;
	int	tgt;
	int	tlun;
{
	if (busno < 0 || busno >= MAX_SCG ||
	    tgt   < 0 || tgt   >= MAX_TGT ||
	    tlun  < 0 || tlun  >= MAX_LUN)
		return (-1);

	if (scgp->local == NULL)
		return (-1);

	return ((int)scglocal(scgp)->scgfiles[busno][tgt][tlun]);
}

EXPORT int
scsi_initiator_id(scgp)
	SCSI	*scgp;
{
	return (-1);

	/* We don't know the initiator ID yet, but we can if we parse the 
	** output of the command 'cat /dev/string/cfg | grep "%adapter"'  
	**
	** Sample line:                                                   
	**
 	**   %adapter 0xE800-0xE8FF 11 - type=alad ha=0 bus=0 id=7 fts=sto  
	**
	** This tells us that the alad controller 0 has an id of 7 !
	** The parsing should be done in openserver_init().
	**
	*/
}

EXPORT
int scsi_isatapi(scgp)
	SCSI	*scgp;
{
	return (sdidevs[scgp->scsibus][scgp->target][scgp->lun].atapi);
}

EXPORT
int scsireset(scgp)
	SCSI	*scgp;
{
	return(-1);		/* no scsi reset available */
}

LOCAL void
cp_scg2sco(sco, scg)
	struct scsicmd2	*sco;
	struct scg_cmd	*scg;
{
	sco->cmd.data_ptr = (char *) scg->addr;
	sco->cmd.data_len = scg->size;
	sco->cmd.cdb_len  = scg->cdb_len;

	sco->sense_len    = scg->sense_len;
	sco->sense_ptr    = scg->u_sense.cmd_sense;

	if (!(scg->flags & SCG_RECV_DATA) && (scg->size > 0))
		sco->cmd.is_write = 1;

	if (scg->cdb_len == SC_G0_CDBLEN )
		memcpy(sco->cmd.cdb, &scg->cdb.g0_cdb, scg->cdb_len);

	if (scg->cdb_len == SC_G1_CDBLEN )
		memcpy(sco->cmd.cdb, &scg->cdb.g1_cdb, scg->cdb_len);

	if (scg->cdb_len == SC_G5_CDBLEN )
		memcpy(sco->cmd.cdb, &scg->cdb.g5_cdb, scg->cdb_len);
}


LOCAL void
cp_sco2scg(sco, scg)
	struct scsicmd2	*sco;
	struct scg_cmd	*scg;
{
	scg->size      = sco->cmd.data_len;

	memset(&scg->scb, 0, sizeof(scg->scb));

	if (sco->sense_len > SCG_MAX_SENSE)
		scg->sense_count = SCG_MAX_SENSE;
	else
		scg->sense_count = sco->sense_len;

	scg->resid = 0;
	scg->u_scb.cmd_scb[0] = sco->cmd.target_sts;

}


LOCAL int
scsi_send(scgp, fd, sp)
	SCSI		*scgp;
	int		fd;
	struct scg_cmd	*sp;
{
	struct scsicmd2	scsi_cmd;
	int		i;
	u_char		sense_buf[SCG_MAX_SENSE];

	if ( fd < 0 ) {
		sp->error = SCG_FATAL;
		return (0);
	}

	memset(&scsi_cmd, 0, sizeof(scsi_cmd));
	memset(sense_buf,  0, sizeof(sense_buf));
	scsi_cmd.sense_ptr = sense_buf;
	scsi_cmd.sense_len = sizeof(sense_buf);
	cp_scg2sco(&scsi_cmd, sp);

	errno = 0;
	for (;;) {
		int	ioctlStatus;

		if ((ioctlStatus = ioctl(fd, SCSIUSERCMD2, &scsi_cmd)) < 0) {
			if (errno == EINTR)
				continue;

			cp_sco2scg(&scsi_cmd, sp);
			sp->ux_errno = errno;
			if (errno == 0)
				sp->ux_errno = EIO;
			sp->error    = SCG_RETRYABLE;

			return (0);
		}
		break;
	}

	cp_sco2scg(&scsi_cmd, sp);
	sp->ux_errno = errno;
	if (scsi_cmd.cmd.target_sts & 0x02) {
		if (errno == 0)
			sp->ux_errno = EIO;
		sp->error    = SCG_RETRYABLE;
	} else {
		sp->error    = SCG_NO_ERROR;
	}

	return 0;
}


#define	sense	u_sense.Sense
