APP = psg_cassandra_test
SRC = psg_cassandra_test unit/blob_record unit/cluster_meta unit/fullscan_plan \
    unit/fullscan_runner unit/cassandra_query unit/nannot_fetch unit/fetch_split_history \
    unit/cassandra_connection unit/bioseq_info_task unit/psg_api unit/load_blob_task \
    unit/get_public_comment

WATCHERS=saprykin
REQUIRES = CASSANDRA MT Linux GCC

#COVERAGE_FLAGS=-fprofile-arcs -ftest-coverage
CPPFLAGS=$(ORIG_CPPFLAGS) $(GMOCK_INCLUDE) $(CASSANDRA_INCLUDE) $(COVERAGE_FLAGS)

MY_LIB=$(XCONNEXT) xconnect connext psg_cassandra psg_diag $(COMPRESS_LIBS) \
    xobjutil id2 seqsplit seqset $(SEQ_LIBS) pub medline biblio general xser xutil
LIB=$(MY_LIB:%=%$(STATIC)) $(LOCAL_LIB) xncbi

LIBS = $(NETWORK_LIBS) -lconnect $(GMOCK_LIBS) $(CASSANDRA_STATIC_LIBS) $(ORIG_LIBS)

LDFLAGS = $(ORIG_LDFLAGS) $(FAST_LDFLAGS) $(COVERAGE_FLAGS) $(LOCAL_LDFLAGS)

#EXTRA=-fno-omit-frame-pointer -fsanitize=address
LOCAL_CPPFLAGS += $(EXTRA) -fno-delete-null-pointer-checks
#LOCAL_LDFLAGS += $(EXTRA) 

#CHECK_CMD = psg_cassandra_test