#!/bin/sh
./oligofar -V
./oligofar -T+ -i NM_012345.reads -d NM_012345.fa -o NM_012345.reads.test -w9 -n1 -L2G "$@" || exit 11
diff NM_012345.reads.test NM_012345.reads.out > /dev/null || exit 21
echo "+ Single read alignment OK"
./oligofar -T+ -i NM_012345.pairs -d NM_012345.fa -o NM_012345.pairs.test -w9 -n1 -L2G "$@" || exit 12
diff NM_012345.pairs.test NM_012345.pairs.out > /dev/null || exit 22
echo "+ Paired read alignment OK"
rm NM_012345.reads.test NM_012345.pairs.test
exit 0
