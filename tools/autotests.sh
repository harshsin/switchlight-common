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
#
# Run all abat tasks for the products of the autobuild script.
#
abat task t.sl.quanta-lb9.oftest.internal"$1" --enable || true
abat task t.sl.quanta-lb9.oftest.release"$1" --enable || true
abat task t.sl.quanta-lb9a.oftest.internal"$1" --enable || true
abat task t.sl.quanta-lb9a.oftest.release"$1" --enable || true
abat task t.sl.quanta-ly2.oftest.internal"$1" --enable || true
abat task t.sl.quanta-ly2.oftest.release"$1" --enable || true
