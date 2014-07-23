#!/usr/bin/python
############################################################
#
# SwitchLight Config Library
#
############################################################

from slrest.base import util
import logging
import re
import json
import os

# Set default logger
logger = logging.getLogger("config")

def setLogger(logger_):
    global logger
    logger = logger_

# Used for filtering out unneeded lines in config
FILTER_REGEX = re.compile(r"^SwitchLight.*|^.*?\(config\).*|^Exiting.*|^\!|^[\s]*$")

ZTN_JSON = "/mnt/flash/boot/ztn.json"

def read_ztn_json(path=ZTN_JSON):
    """
    Read ZTN JSON file.
    """
    with open(path, "r") as f:
        data = json.loads(f.read())
    return data

def write_ztn_json(data, path=ZTN_JSON):
    """
    Write ZTN JSON file.
    """
    with open(path, "w") as f:
        f.write(json.dumps(data))

def compare_configs(old_cfg, new_cfg):
    """
    Do a diff between old_cfg and new_cfg.
    old_cfg and new_cfg are lists of config lines (strings).
    Return lines that are removed and lines that are added.
    """
    lines_to_remove = [l for l in old_cfg if l not in new_cfg]
    lines_to_add = [l for l in new_cfg if l not in old_cfg]
    return (lines_to_remove, lines_to_add)

def verify_configs(expected_cfg, actual_cfg):
    """
    Verify that expected_cfg and actual_cfg are the same.
    expected_cfg and actual_cfg are lists of config lines (strings).
    """
    lines1, lines2 = compare_configs(expected_cfg, actual_cfg)
    if len(lines1) != 0 or len(lines2) != 0:
        logger.error("Missing lines:\n%s" % lines1)
        logger.error("Extra lines:\n%s" % lines2)
        raise ValueError("Verify configs failed.")

def get_config_from_ztn_server(ztn_server):
    """
    Fetch config from ZTN server.
    Filter out unneeded lines.
    Return config as a list of config lines (strings) and md5sum.
    """
    logger.debug("Fetching config from ztn server: %s" % ztn_server)

    (rc, out) = util.bash_command("ztn --transact --server %s" % ztn_server)
    if rc:
        raise IOError("Failed to transact with ZTN server (%s): %s" % \
                      (ztn_server, out))

    (rc, out) = util.bash_command("ztn --startup")
    if rc:
        raise IOError("Failed to get ZTN startup config path: %s" % out)

    path = out.strip()
    logger.debug("startup config path: %s" % path)

    md5sum = os.path.basename(path).split(".")[0]
    logger.debug("startup config md5sum: %s" % md5sum)

    with open(path, "r") as f:
        cfg = [l for l in f.read().splitlines() if not FILTER_REGEX.match(l)]

    return (cfg, md5sum)

def get_running_config():
    """
    Get running config from pcli.
    Filter out unneeded lines.
    Return config as a list of config lines (strings).
    """
    out = util.pcli_command("show running-config")
    cfg = [l for l in out.splitlines() if not FILTER_REGEX.match(l)]
    return cfg

def apply_config(cfg):
    """
    Apply config in cfg via pcli.
    cfg is a list of config lines (strings).
    """
    cmd = ";".join(cfg)
    out = util.pcli_command(cmd)
    logger.debug("pcli output:\n%s" % out)

def revert_default_config():
    """
    Revert to default config via pcli.
    """
    out = util.pcli_command("_internal; revert-default")
    logger.debug("pcli output:\n%s" % out)

def save_running_config():
    """
    Save running config as startup config via pcli.
    """
    out = util.pcli_command("copy running-config startup-config")
    logger.debug("pcli output:\n%s" % out)

def reload_config(ztn_server):
    """
    Get config from ZTN server and reload config.
    Return a tuple of (rc, error), where rc is 0 on success and 1 on failure.
    """
    rc = 0
    error = None

    try:
        # save states for roll-back
        old_cfg = get_running_config()
        old_ztn_json = read_ztn_json()

        # get startup config from ZTN
        new_cfg, md5sum = get_config_from_ztn_server(ztn_server)

        # revert to default, apply new config
        revert_default_config()
        apply_config(new_cfg)

        # verify config
        post_cfg = get_running_config()
        verify_configs(new_cfg, post_cfg)

        # update ZTN JSON file, save config
        new_ztn_json = old_ztn_json.copy()
        new_ztn_json["startup_config_md5"] = md5sum
        save_running_config()
        write_ztn_json(new_ztn_json)

        logger.debug("update config finished.")

    except Exception, e:
        logger.exception("update config failed.")
        rc = 1
        error = str(e)

        # attempt roll-back
        try:
            revert_default_config()
            apply_config(old_cfg)

            post_cfg = get_running_config()
            verify_configs(old_cfg, post_cfg)

            save_running_config()
            write_ztn_json(old_ztn_json)

            logger.debug("roll-back config finished.")

        except Exception, e2:
            logger.exception("roll-back config failed.")
            rc = 2
            error += "\n%s" % str(e2)

    return (rc, error)
