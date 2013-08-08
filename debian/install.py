#!/usr/bin/python

import sys
import os
import argparse
import shutil

def find_file_or_dir(basedir, filename=None, dirname=None):
    for root, dirs, files in os.walk(basedir):
        for file_ in files:
            if file_ == filename:
                print "%s/%s" % (root, file_)
                sys.exit(0)
        if dirname and os.path.basename(root) == dirname:
            print "%s" % (root)
            sys.exit(0)


ap = argparse.ArgumentParser("SwitchLight Build Package Installer")

ap.add_argument("package", 
                help="package name")
ap.add_argument("arch", 
                help="package architecture")
ap.add_argument("--force", help="Force reinstall", 
                action='store_true')
ap.add_argument("--find-file", help="Return path to given file.", 
                default=None)
ap.add_argument("--find-dir", help="Return path to the given directory.", 
                default=None)

ops = ap.parse_args()

SWITCHLIGHT = os.getenv('SWITCHLIGHT')

extract_dir = "%s/debian/installs/%s/%s" % (SWITCHLIGHT, ops.arch, ops.package)

if os.path.exists(extract_dir) and not ops.force:
    find_file_or_dir(extract_dir, ops.find_file, ops.find_dir); 
    sys.exit(0)
#
# Read available packages
#
package_dir = os.path.abspath("%s/debian/repo" % SWITCHLIGHT)
manifest = os.listdir(package_dir)

#
# Find matching
#
packages = [ x for x in manifest if ops.arch in x and "%s_" % ops.package in x ]

if len(packages) == 0:
    print "No matching packages for %s (%s)" % (ops.package, ops.arch)
    sys.exit(1)
if len(packages) > 1:
    print "Multiple packages found: %s" % packages
    sys.exit(1)

deb = packages[0]

#
# Remove existing
#
if os.path.exists(extract_dir):
    shutil.rmtree(extract_dir)
os.makedirs(extract_dir)
os.system("dpkg -x %s/%s %s" % (package_dir, deb, extract_dir))
find_file_or_dir(extract_dir, ops.find_file, ops.find_dir); 












