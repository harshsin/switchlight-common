# Copyright (c) 2013  BigSwitch Networks

import os
import sys
import json
from PandOS.ofad import Controller, OFADConfig

REASONS = set(["REBOOT", "RENEW"])
PATH = "/etc/pandos-dhcp-enabled-%s"

def saveLeaseInfo (path):
    e = os.environ
    data = {"ip-address" : e["new_ip_address"], "subnet-mask" : e["new_subnet_mask"],
            "default-gw" : e["new_routers"], "nameservers" : e["new_domain_name_servers"]}
    with open(path, "w+") as lease_file:
        json.dump(data, lease_file)

if __name__ == '__main__':
    if os.environ["reason"] not in REASONS:
        sys.exit(0)

    intf = os.environ("interface")
    if not os.path.exists(PATH % (intf)):
        # This interface isn't configured for DHCP config data
        sys.exit(0)

    saveLeaseInfo(PATH % (intf))

    opts = os.environ["new_vendor_encapsulated_options"].split(":")
    if int(opts[0], 16) != 1:
        # This isn't us.  Option 43 is stupid, we should probably keep looking
        # Even if it's 1, it might not be us...we'll probably die later
        sys.exit(0)

    olen = int(opts[1], 16)
    if olen % 4 != 0:
        # Something is wrong
        sys.exit(1)

    cfg = OFADConfig()
    controllers = cfg.controllers

    offset = 2
    for x in range(0, olen, 4):
        addr = "%d.%d.%d.%d" % (
            int(opts[offset+x], 16),
            int(opts[offset+x+1], 16),
            int(opts[offset+x+2], 16),
            int(opts[offset+x+3], 16))
        con = Controller().setAddress(addr).setPort(6633).setProtocol("tcp").setDHCP(True)
        matched = False
        for c in controllers:
            if c.merge(con):
                matched = True
        if not matched:
            controllers.append(con)

    cfg.controllers = controllers
    cfg.write()
