#!/bin/bash

# Copyright Joris Geurts 2009
#
# This shell script is to test the calc kernel module
# Before running this shell scrip, do the following:
#
#   make
#   insmod calc.ko
#
# Running the script:
#
#   ./calc_check.sh
#
# And afterwards:
#
#   rmmod calc
#
# (maybe you need to put 'sudo' in front)
#
PROC=/proc/calc
OK_FILE=calc_ok.txt
TEST_FILE=calc_test.txt

# create the OK_FILE:
cat <<EOF > $OK_FILE
0
100
50
4
20
6
216
216
216
1234568106
0
-1234
EOF

# do the test (and store the results in TEST_FILE)
cat $PROC > $TEST_FILE
echo "+100" >  $PROC
cat $PROC >> $TEST_FILE
echo "-50"  >  $PROC
cat $PROC >> $TEST_FILE
echo "/12"  >  $PROC
cat $PROC >> $TEST_FILE
echo "*5"   >  $PROC
cat $PROC >> $TEST_FILE
echo "%7"   >  $PROC
cat $PROC >> $TEST_FILE
echo "^3"   >  $PROC
cat $PROC >> $TEST_FILE
echo "/0"   >  $PROC
cat $PROC >> $TEST_FILE
echo "%0"   >  $PROC
cat $PROC >> $TEST_FILE
echo "+1234567890" > $PROC
cat $PROC >> $TEST_FILE
echo c    >  $PROC
cat $PROC >> $TEST_FILE
echo -1234 > $PROC
cat $PROC >> $TEST_FILE
echo c    >  $PROC

# compare both files
diff $OK_FILE $TEST_FILE
if test $? -eq 0
then
    echo "results are OK"
else
    echo ""
    echo "results are NOT ok"    
    echo ""
fi
