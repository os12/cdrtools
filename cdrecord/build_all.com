$! build_all.com Version 23.5.2000 for cdrecord 1.8.1 final
$ create/dir [.vms]
$ copy *.c [.vms]
$ copy [-.libscg]*.c [.vms]
$ copy [-.libdeflt]*.c [.vms]
$ set default [.vms]
$ define scg [--.libscg.scg]
$ cc/pref=all/obj/incl=([-],[--.include])/def=("VMS") CDRECORD.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") CDR_DRV.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") DRV_JVC.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") DRV_MMC.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") DRV_PHILIPS.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") DRV_SONY.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") FIFO.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") ISOSIZE.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") MODES.C
$! cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") PORT.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") SCSIERRS.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") SCSITRANSP.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") SCSIOPEN.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") SCSI_CDR.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") SCSI_SCAN.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") WM_PACKET.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") WM_SESSION.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") WM_TRACK.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") AUDIOSIZE.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") DRV_SIMUL.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") DISKID.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") CD_MISC.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") MISC.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") DEFAULTS.C
$ cc/pref=all/obj/incl=([-],[--.include])/define=("VMS") DEFAULT.C
$ crea/dir [.inc]
$ copy [--.inc]*.* [.inc]
$ set default [.inc]
$ cc/pref=all/obj/incl=([--],[---.include])/define=("VMS") ALIGN_TEST.C
$ link ALIGN_TEST
$ delete ALIGN_TEST.obj;*
$ def/user sys$output ALIGN.H
$ r ALIGN_TEST
$ cc/pref=all/obj/incl=([],[--],[---.include])/define=("VMS") AVOFFSET.C
$ link AVOFFSET
$ delete AVOFFSET.obj;*
$ def/user sys$output AVOFFSET.H
$ r AVOFFSET
$ set default [-]
$ crea/dir [.lib]
$ copy [--.lib]*.* [.lib]
$ set default [.lib]
$ cc/pref=all/obj/incl=([--],[---.include])/define=("VMS") ASTOI.C
$ cc/pref=all/obj/incl=([--],[---.include])/define=("VMS") COMERR.C
$ cc/pref=all/obj/incl=([--],[---.include])/define=("VMS") ERROR.C
$ cc/pref=all/obj/incl=([--],[---.include])/define=("VMS") FCONV.C
$ cc/pref=all/obj/incl=([],[--],[---.include],[-.inc])/define=("VMS") FILLBYTES.C
$ cc/pref=all/obj/incl=([--],[---.include])/define=("VMS") FORMAT.C
$ cc/pref=all/obj/incl=([--],[---.include])/define=("VMS") GETARGS.C
$ cc/pref=all/obj/incl=([],[--],[---.include],[-.inc])/define=("VMS") GETAV0.C
$ cc/pref=all/obj/incl=([--],[---.include])/define=("VMS") GETERRNO.C
$ cc/pref=all/obj/incl=([--],[---.include])/define=("VMS") GETFP.C
$ cc/pref=all/obj/incl=([],[--],[---.include],[-.inc])/define=("VMS") MOVEBYTES.C
$ cc/pref=all/obj/incl=([--],[---.include])/define=("VMS") PRINTF.C
$ cc/pref=all/obj/incl=([],[--],[---.include],[-.inc])/define=("VMS") RAISECOND.C
$ cc/pref=all/obj/incl=([],[--],[---.include],[-.inc])/define=("VMS") SAVEARGS.C
$ cc/pref=all/obj/incl=([],[--],[---.include],[-.inc])/define=("VMS") jsprintf.C
$ cc/pref=all/obj/incl=([],[--],[---.include],[-.inc])/define=("VMS") jssnprintf.C
$ cc/pref=all/obj/incl=([--],[---.include])/define=("VMS") STREQL.C
$ cc/pref=all/obj/incl=([--],[---.include])/define=("VMS") SWABBYTES.C
$ libr/crea [-]lib.olb
$ libr/ins [-]lib.olb *.obj
$ delete *.obj;*
$ set default [-]
$ create/dir [.stdio]
$ copy [--.lib.stdio]*.* [.stdio]
$ set default [.stdio]
$ CC/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") CVMOD.C
$ CC/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") DAT.C
$ CC/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") FCONS.C
$ CC/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") FGETLINE.C
$ CC/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") FILEOPEN.C
$ CC/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") FILEREAD.C
$ CC/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") FILEWRITE.C
$ CC/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") FLAG.C
$ CC/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") FLUSH.C
$ CC/PREF=ALL/OBJ/INCL=([--],[---.INCLUDE])/DEFINE=("VMS") NIREAD.C
$ libr/cre [-]stdio.olb
$ libr/ins [-]stdio.olb *.obj
$ delete *.obj;*
$ set default [-]
$ libr/cre cdr.olb
$ libr/ins cdr.olb *.obj
$ link CDRECORD,cdr.olb/lib,lib.olb/lib,stdio.olb/lib
$ dir *.exe
