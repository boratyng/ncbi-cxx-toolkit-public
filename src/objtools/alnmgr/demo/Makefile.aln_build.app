# $Id$

APP = aln_build
SRC = aln_build_app
LIB = xalnmgr xobjutil submit tables $(OBJMGR_LIBS)

LIBS = $(GENBANK_THIRD_PARTY_LIBS) $(CMPRS_LIBS) $(DL_LIBS) $(NETWORK_LIBS) $(ORIG_LIBS)


WATCHERS = grichenk