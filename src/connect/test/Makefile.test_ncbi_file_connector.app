# $Id$

APP = test_ncbi_file_connector
SRC = test_ncbi_file_connector
LIB = connect $(NCBIATOMIC_LIB)

LIBS = $(NETWORK_LIBS) $(ORIG_LIBS)
LINK = $(C_LINK)

CHECK_CMD = test_ncbi_file_connector test_ncbi_file_connector.dat /CHECK_NAME=test_ncbi_file_connector
CHECK_COPY = test_ncbi_file_connector.dat

WATCHERS = lavr