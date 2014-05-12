# Copyright (c) 2013  BigSwitch Networks

import os
import sys
import json
from sl_util.ofad import Controller, OFADConfig

REASONS = set(["REBOOT", "RENEW"])
PATH = "/etc/sl-dhcp-enabled-%s"

def saveLeaseInfo (path):
    e = os.environ
    data = {"ip-address" : e["new_ip_address"],
            "subnet-mask" : e["new_subnet_mask"],
            "default-gw" : e["new_routers"],
            "nameservers" : e["new_domain_name_servers"]}
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

