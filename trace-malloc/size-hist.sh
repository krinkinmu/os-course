#!/bin/bash

TRACE=$1
HIST=hist.eps
BUCKET=16
LIMIT=1024

if [ ! -f "$TRACE" ]
then
    echo "no input file found"
    exit 1
fi

DAT=`mktemp`

cat $TRACE | python size-dist.py --bucket $BUCKET --limit $LIMIT > $DAT
gnuplot -e "inputfile='$DAT'; outputfile='$HIST'" hist.gp
