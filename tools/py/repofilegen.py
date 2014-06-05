#!/usr/bin/python
############################################################
#
# Generate the repo input files for the mkws script.
#
import sys
import argparse
import tempfile

ap=argparse.ArgumentParser(description="Generate mkws repo input files.")
ap.add_argument('--platforms', help="The list of platforms to include support for.",
                required=True, nargs='+')
ap.add_argument('--packages', help="The package list.", required=True, nargs='+')
ap.add_argument('--arch', help="The architecture.", required=True)
ap.add_argument('--dir', help="The source repo directory path.", required=True);
ap.add_argument('--out', help="Output file.", required=True)

ops=ap.parse_args()

if ops.out in ['-', 'stdout']:
    outh=sys.stdout
elif ops.out in ['mktemp']:
    outh=tempfile.NamedTemporaryFile(delete=False)
else:
    outh=open(ops.out, "w")

if ops.arch == 'all':
    # Automatically add all platform-config packages.
    PLATFORM_CONFIGS = [ 'platform-config-' + p for p in ops.platforms ]
    PACKAGES = ops.packages + PLATFORM_CONFIGS
else:
    # Automatically add all onlp platform packages
    ONLP_PLATFORMS = [ 'onlp-' + p for p in ops.platforms ]
    PACKAGES = ops.packages + ONLP_PLATFORMS

outh.write("[deb-%s]\n" % ops.arch)
outh.write("packages=%s\n" % " ".join(PACKAGES))
outh.write("\n")
outh.write("source=copy:%s/%s ./\n" % (ops.dir, ops.arch))
outh.write("omitdebsrc=true\n")

if ops.out in ['mktemp']:
    print outh.name



