#!/usr/bin/python
###############################################################################
# 
# SwitchLight Build Package Manager
#
###############################################################################
import sys
import os
import argparse
import shutil
from debian import deb822

def find_file_or_dir(basedir, filename=None, dirname=None):
    """Find and return the path to a given file or directory below the given root"""
    for root, dirs, files in os.walk(basedir):
        for file_ in files:
            if file_ == filename:
                print "%s/%s" % (root, file_)
                sys.exit(0)
        if dirname and os.path.basename(root) == dirname and root != basedir:
            print "%s" % (root)
            sys.exit(0)

def find_component_dir(basedir, package_name):
    """Find the local component directory that builds the given package."""
    for root, dirs, files in os.walk(basedir):
        for file_ in files:
            if file_ == "Makefile" or file_ == "makefile":
                with open("%s/%s" % (root,file_), "r") as f:
                    data = f.read()
                    if "Package:%s" % package_name in data:
                        # By convention - this is the component directory. 
                        return os.path.abspath(root)
            if file_ == "control":
                with open("%s/%s" % (root,file_), "r") as f:
                    control = f.read()
                    if "Package: %s" % package_name in control:
                        # By convention - find the parent directory
                        # That has a 'debian' directory in it. 
                        print "Found at %s" % root
                        while not 'Makefile.comp' in os.listdir(root):
                            root += "/.."
                        return os.path.abspath(root)

def find_package(repo, package, arch):
    """Find a package by name and architecture in the given repo dir"""
    manifest = os.listdir(repo)
    return [ x for x in manifest if arch in x and "%s_" % package in x ]
    

def find_all_packages(basedir):
    """Find all local component packages."""
    all_ = []; 

    for root, dirs, files in os.walk(basedir):
        for file_ in files:
            if file_ == "control":
                f = file("%s/%s" % (root,file_))
                d = deb822.Deb822(f)
                while d:
                    if 'Package' in d:
                        arch = 'all'
                        if 'Architecture' in d:
                            arch = d['Architecture']
                        all_.append("%s:%s" % (d['Package'],arch))
                    d = deb822.Deb822(f)

    return all_
                    
                    
###############################################################################

ap = argparse.ArgumentParser("SwitchLight Build Package Manager")

ap.add_argument("packages", nargs='*', action='append',
                help="package:arch", default=None)

ap.add_argument("--force", help="Force reinstall", 
                action='store_true')
ap.add_argument("--find-file", help="Return path to given file.", 
                default=None)
ap.add_argument("--find-dir", help="Return path to the given directory.", 
                default=None)
ap.add_argument("--build", help="Attempt to build local package if it exists.", 
                action='store_true')
ap.add_argument("--add-pkg", nargs='+', action='append', 
                default=None, help="Install new package files and invalidate corresponding installs.")
ap.add_argument("--list-all", action='store_true', help="List all available component packages"); 
ap.add_argument("--force-build", help="Force rebuild from source.", 
                action='store_true')

ops = ap.parse_args()

SWITCHLIGHT = os.getenv('SWITCHLIGHT')
package_dir = os.path.abspath("%s/debian/repo" % SWITCHLIGHT)

if SWITCHLIGHT is None:
    raise Exception("$SWITCHLIGHT is not defined.")

if ops.list_all:
    all_ = find_all_packages(os.path.abspath("%s/components" % (SWITCHLIGHT)))
    for p in all_:
        if not ":any" in p:
            print p
    sys.exit(0)

if ops.add_pkg:
    for pa in ops.add_pkg[0]:
        # Copy the package into the repo
        print "Adding new package %s..." % pa
        shutil.copy(pa, package_dir); 
        # Determine package name and architecture
        underscores = pa.split('_')
        # Package name is the first entry
        package = underscores[0]
        # Architecture is the last entry (.deb)
        arch = underscores[-1].split('.')[0]
        extract_dir = "%s/debian/installs/%s/%s" % (SWITCHLIGHT, arch, package)
        if os.path.exists(extract_dir):
            # Make sure the package gets reinstalled the next time it's needed
            print "Removed previous install directory %s..." % extract_dir
            shutil.rmtree(extract_dir)
    sys.exit(0)
        

for pa in ops.packages[0]:

    try:
        (package, arch) = pa.split(":")
    except ValueError:
        print "invalid package specification: ", pa
        sys.exit(1)

    packages = []
    if not ops.force_build:
        packages = find_package(package_dir, package, arch)
    else:
        ops.build = True

    if len(packages) == 0:
        print "No matching packages for %s (%s)" % (package, arch)
        # Look for package builder
        buildpath = find_component_dir(os.path.abspath("%s/components/%s" % (SWITCHLIGHT,arch)), 
                                       package)
        if buildpath is not None:
            print "Can be built locally at %s" % buildpath
            if ops.build:
                if os.system("make -C %s deb" % buildpath) != 0:
                    print "Build failed."
                    sys.exit(1)
                else:
                    packages = find_package(package_dir, package, arch)
        if len(packages) == 0:
            sys.exit(1)
        
    if len(packages) > 1:
        print "Multiple packages found: %s" % packages
        sys.exit(1)

    deb = packages[0]

    extract_dir = "%s/debian/installs/%s/%s" % (SWITCHLIGHT, arch, package)

    if os.path.exists("%s/PKG.TIMESTAMP" % extract_dir):
        if (os.path.getmtime("%s/PKG.TIMESTAMP" % extract_dir) == 
            os.path.getmtime("%s/%s" % (package_dir, deb))):
            # Existing extract is identical
            sys.stderr.write("Existing extract for %s:%s matches the package file.\n" % (deb,arch))
        else:
            # Extract must be updated. 
            ops.force = True
            sys.stderr.write("Existing extract for %s:%s does not match the package file. Forcing extract.\n" % (deb,arch))

        if ops.force:
            sys.stderr.write("Force removing %s...\n" % extract_dir)
            shutil.rmtree(extract_dir)


    if not os.path.exists(extract_dir):
        sys.stderr.write("Extracting %s/%s...\n" % (package_dir, deb))
        os.makedirs(extract_dir)
        os.system("dpkg -x %s/%s %s" % (package_dir, deb, extract_dir))
        os.system("touch -r %s/%s %s/PKG.TIMESTAMP" % (package_dir, deb, extract_dir))

    find_file_or_dir(extract_dir, ops.find_file, ops.find_dir); 














