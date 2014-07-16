#!/bin/sh
############################################################
# <bsn.cl fy=2013 v=none>
#
#        Copyright 2013, 2014 BigSwitch Networks, Inc.
#
#
#
# </bsn.cl>
############################################################
set -e
set -x

if [ -z $SERVER ] ; then
	SERVER=switch-nfs.hw.bigswitch.com
fi
if [ -z $BRANCH ]; then
	BRANCH=master
fi
if [ -z $BUILD ]; then
	BUILD=latest
fi

echo Copying files from host=$SERVER branch=$BRANCH build=$BUILD

# NOTE the trailing / is important here
rsync -av $USER@${SERVER}:/var/export/switchlight/autobuilds/$BRANCH/$BUILD/repo/ .

# Any existing package extracts must be removed.
make -C ../installs clean

