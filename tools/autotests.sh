#!/bin/bash
############################################################
# <bsn.cl fy=2013 v=none>
#
#        Copyright 2013, 2014 BigSwitch Networks, Inc.
#
#
#
# </bsn.cl>
############################################################
#
# Run all abat tasks for the products of the autobuild script.
#
platforms="quanta-lb9 quanta-lb9a quanta-ly2 hoth"

for platform in $platforms; do
    echo abat task t.sl."$platform".oftest.internal"$1" --enable || true
    echo abat task t.sl."$platform".oftest.release"$1" --enable || true
done
