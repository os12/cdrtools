#ident %W% %E% %Q%
###########################################################################
# Sample makefile for general application programs
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		devdump
#CPPOPTS +=	-DADD_FILES
CPPOPTS +=	-DUSE_LIBSCHILY
CFILES=		dump.c
LIBS=		-lschily
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
