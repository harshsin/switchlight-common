#!/usr/bin/python
############################################################
#
# Generate ZTN Manifest files.
#
############################################################
import argparse
import json

ap=argparse.ArgumentParser(description="ZTN Manifest Generator.")

ap.add_argument("--manifest_version", help="Specify manifest version.",
                default="1")
ap.add_argument("--operation", help="Manifest operation.", required=True)
ap.add_argument("--platforms", nargs='+', help="Platform list.", required=True)
ap.add_argument("--release", help="Manifest release string.", required=True)
ap.add_argument("--sha1", help="Manifest SHA1.", required=True)

ops = ap.parse_args();

# Operation shortcuts
operations=dict(swi='ztn-runtime', installer='os-install')

if ops.operation in operations:
    ops.operation = operations[ops.operation]

manifest = dict(manifest_version=ops.manifest_version,
                release=ops.release,
                platform=','.join(ops.platforms),
                operation=ops.operation,
                sha1=ops.sha1)

print json.dumps(manifest, indent=2)
