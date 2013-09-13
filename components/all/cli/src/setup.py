#!/usr/bin/python

import distutils.core

distutils.core.setup(
    name="pcli",
    version="0.1.0",
    package_dir={"pcli": ""},
    packages=["pcli", "pcli.desc", "pcli.desc.version001", "sl_util", 
              "loxi", "loxi.of10", "loxi.of11", "loxi.of12", "loxi.of13"],
    package_data={"pcli": ["documentation/en_US/*/*"]},
    scripts=["pcli"]
    )
