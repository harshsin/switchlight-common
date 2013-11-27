#!/bin/bash
############################################################
#
#  autobuild.sh 
# 
############################################################
set -e
set -x

# Skip grep
touch abat_nogrep

# Parent Switchlight directory
SWITCHLIGHT_ROOT=`realpath $(dirname $(readlink -f $0))/../`

# The current branch
BRANCH=`cd $SWITCHLIGHT_ROOT && git symbolic-ref --short -q HEAD`

# The repository origin. 
: ${GITUSER:=`cd $SWITCHLIGHT_ROOT && git remote -v | grep origin | grep fetch | tr ':/' ' ' | awk '{print $3}'`}

SHA1=`cd $SWITCHLIGHT_ROOT && git rev-list HEAD -1`

if [ "$GITUSER" == "bigswitch" ]; then
    # Special case. 
    USERDIR=""
    ABAT_SUFFIX=".$BRANCH"
else
    USERDIR="$GITUSER/"
    ABAT_SUFFIX=".$GITUSER.$BRANCH"
fi

if [ -z "$WS_ROOT" ]; then
    echo "No workspace specified with \$WS_ROOT."
    exit 1
fi


MAILSUBJECT="autobuild: pid=$$ branch=$BRANCH user=$GITUSER"


if [ -n "$MAILTO" ]; then 
    NOW=`date`
    mail -s "$MAILSUBJECT time=`date` : start" $MAILTO < /dev/null
fi

cd $WS_ROOT

# Set one build date for all builds
export SWITCHLIGHT_BUILD_TIMESTAMP=`date +%Y.%m.%d.%H.%M`
export SWITCHLIGHT=/build/switchlight
: ${INSTALL_SERVER:=switch-nfs}
: ${INSTALL_BASE_DIR:=/var/export/switchlight/autobuilds}
INSTALL_AUTOBUILD_DIR=${INSTALL_BASE_DIR}/$USERDIR"$BRANCH"
INSTALL_DIR=${INSTALL_AUTOBUILD_DIR}/$SWITCHLIGHT_BUILD_TIMESTAMP.$SHA1

#
# Remount the current workspace to /build/switchlight
#
pwd
cat <<EOF > .chwsrc
bind_mount_dst $SWITCHLIGHT_ROOT $SWITCHLIGHT
EOF

rm -rf $SWITCHLIGHT_ROOT/builds/BUILDS

(chws make -C /build/switchlight/builds CCACHE_DIR=/mnt/cache/ccache parallel0 -j $JOBS) || true
(chws make -C /build/switchlight/builds CCACHE_DIR=/mnt/cache/ccache parallel1 -j $JOBS) || true
(chws make -C /build/switchlight/builds CCACHE_DIR=/mnt/cache/ccache parallel2 -j $JOBS) || true
(chws make -C /build/switchlight/builds CCACHE_DIR=/mnt/cache/ccache parallel3 -j $JOBS) || true
(chws make -C /build/switchlight/builds CCACHE_DIR=/mnt/cache/ccache parallel4 -j $JOBS) || true
(chws make -C /build/switchlight/builds CCACHE_DIR=/mnt/cache/ccache parallel5 -j $JOBS) || true
(chws make -C /build/switchlight/builds CCACHE_DIR=/mnt/cache/ccache parallel6 -j $JOBS) || true


function build_and_install {
    # Build Requested
    chws make -C /build/switchlight/builds CCACHE_DIR=/mnt/cache/ccache $@

    # Make the install directory
    ssh $INSTALL_SERVER mkdir -p $INSTALL_DIR

    # Copy all build products to the install directory
    scp $SWITCHLIGHT_ROOT/builds/BUILDS/* $INSTALL_SERVER:$INSTALL_DIR

    # Update latest and build manifest
    ssh $INSTALL_SERVER $INSTALL_BASE_DIR/update-latest.py --dir $INSTALL_AUTOBUILD_DIR --force
    
    if [ -n "$MAILTO" ]; then
        ARGS="$@"
	mail -s "$MAILSUBJECT time=`date` : built and installed $ARGS" $MAILTO < /dev/null
    fi
}

# Build primary targets for testing
build_and_install swi-internal swi-release installer-all-release installer-quanta-lb9a-release installer-quanta-lb9-release installer-quanta-ly2-release installer-accton-es5652bt-release 

# Copy the loader binaries (hack)
ssh $INSTALL_SERVER mkdir -p $INSTALL_DIR/loaders
LOADERS=`find $SWITCHLIGHT_ROOT/debian/installs -name "*.loader"`
scp $LOADERS $INSTALL_SERVER:$INSTALL_DIR/loaders

# Copy all debian packages
ssh $INSTALL_SERVER mkdir -p $INSTALL_DIR/repo
scp -r $SWITCHLIGHT_ROOT/debian/repo $INSTALL_SERVER:$INSTALL_DIR
ssh $INSTALL_SERVER rm $INSTALL_DIR/repo/update.sh $INSTALL_DIR/repo/.lock $INSTALL_DIR/repo/.gitignore || true

# Kick off automated tests here for primary targets
if [ -n "$ABAT_SUFFIX" ]; then 
    abat task t.sl.lb9.oftest"$ABAT_SUFFIX" --enable || true
    abat task t.sl.lb9a.oftest"$ABAT_SUFFIX" --enable || true
    abat task t.sl.ly2.oftest"$ABAT_SUFFIX" --enable || true
    abat task t.sl.accton5652.oftest"$ABAT_SUFFIX" --enable || true
fi

# Build remaining targets
# Temporarily disabled until all builds are fixed. 
# build_and_install all

# Update build manifest
ssh $INSTALL_SERVER $INSTALL_BASE_DIR/update-latest.py --update-manifest --dir $INSTALL_AUTOBUILD_DIR --force

if [ -n "$MAILTO" ]; then 
    NOW=`date`
    mail -s "$MAILSUBJECT time=`date`: finished" $MAILTO < /dev/null
fi












