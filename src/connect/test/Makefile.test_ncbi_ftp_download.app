# $Id$

APP = test_ncbi_ftp_download
SRC = test_ncbi_ftp_download
CPPFLAGS = $(CMPRS_INCLUDE) $(ORIG_CPPFLAGS)
LIB = xconnect $(COMPRESS_LIBS) xutil xncbi

LIBS = $(NETWORK_LIBS) $(CMPRS_LIBS) $(ORIG_LIBS)
#LINK = purify $(ORIG_LINK)

CHECK_CMD = test_ncbi_ftp_download.sh /CHECK_NAME=test_ncbi_ftp_download
CHECK_COPY = test_ncbi_ftp_download.sh

WATCHERS = lavr