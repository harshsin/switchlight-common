#!/usr/bin/python
############################################################
#
# CLI Interface to the SLREST APIs
#
############################################################
#
#

from slrest.base.slapi_object import SLAPIObject
from slrest.api import *
from slrest.api.v1 import *

import sys
import argparse
import logging

logger = logging.getLogger("slrest-cli")
logger.setLevel(logging.DEBUG)
parser = argparse.ArgumentParser(description='CLI Interface to the SLREST APIs',
                                 formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument("--hostname", default="localhost", help="Device running slrest server")
parser.add_argument("--port", default="")
sub_parser = parser.add_subparsers()
SLAPIObject.mount_all(logger)
SLAPIObject.mount_cmds(sub_parser)
args = parser.parse_args()
args.func(args)
