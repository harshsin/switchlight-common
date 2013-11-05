# Copyright (c) 2013  BigSwitch Networks

from sl_util import const

platform_ = None

def get_platform():
    global platform_
    if platform_ is not None:
        return platform_

    try:
        with open(const.PLATFORM_PATH) as f:
            platform_ = f.read().strip()
    except:
        platform_ = ""

    return platform_
