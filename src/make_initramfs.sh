#!/bin/bash

DIR=$1
INITRAMFS=$2

if [ ! -d "$DIR" ]
then
	echo "input directory name expected"
	exit 1
fi

if [ "x$INITRAMFS" == "x" ]
then
	echo "output initramfs image name expected"
	exit 1
fi

find $DIR -print0 | cpio --null -ov --format=newc > $INITRAMFS
