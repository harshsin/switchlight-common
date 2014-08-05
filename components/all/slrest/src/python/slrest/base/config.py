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
import shutil

# Set default logger
logger = logging.getLogger("config")

def setLogger(logger_):
    global logger
    logger = logger_

# Used for filtering out unneeded lines in config
FILTER_REGEX = re.compile(r"^SwitchLight.*|^.*?\(config\).*|^Exiting.*|^\!|^[\s]*$")

ZTN_JSON = "/mnt/flash/boot/ztn.json"
LAST_ZTN_CFG = "/var/run/last-ztn-startup-config"

# Black-listed commands where "no" is not supported
PCLI_BLIST = [
    "ntp sync",
]

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

def create_patch_config(old_cfg, new_cfg):
    """
    Create patch config from old_cfg and new_cfg.
    old_cfg and new_cfg are lists of config lines (strings).
    Return patch config as a list of config lines (strings).
    """
    old_lines, new_lines = compare_configs(old_cfg, new_cfg)
    logger.debug("old_lines:\n%s" % old_lines)
    logger.debug("new_lines:\n%s" % new_lines)

    # create patch config:
    # (1) for old lines:
    #   -if regular (non-"no") command, prepend with "no" to remove
    #   -if "no" command, reverse by removing "no" from head
    #   -skip if in PCLI_BLIST
    #
    # (2) for new lines:
    #   - apply as is
    return [l[3:] if l.startswith("no ") else "no %s" % l \
            for l in old_lines if l not in PCLI_BLIST] + new_lines

def get_config_from_ztn_server(ztn_server):
    """
    Fetch config from ZTN server.
    Filter out unneeded lines.
    Return config as a list of config lines (strings), config path and md5sum.
    """
    logger.debug("Fetching config from ztn server: %s", ztn_server)

    (rc, out) = util.bash_command("ztn --transact --server %s" % ztn_server)
    if rc:
        raise IOError("Failed to transact with ZTN server (%s): %s" % \
                      (ztn_server, out))

    (rc, out) = util.bash_command("ztn --startup")
    if rc:
        raise IOError("Failed to get ZTN startup config path: %s" % out)

    path = out.strip()
    logger.debug("startup config path: %s", path)

    md5sum = os.path.basename(path).split(".")[0]
    logger.debug("startup config md5sum: %s", md5sum)

    with open(path, "r") as f:
        cfg = [l for l in f.read().splitlines() if not FILTER_REGEX.match(l)]

    return (cfg, path, md5sum)

def read_last_ztn_config(path=LAST_ZTN_CFG):
    """
    Read last-used ZTN config.
    Return config as a list of config lines (strings).
    """
    with open(path, "r") as f:
        cfg = [l for l in f.read().splitlines() if not FILTER_REGEX.match(l)]
    return cfg

def save_last_ztn_config(src_path, dst_path=LAST_ZTN_CFG):
    """
    Save last-used ZTN config by copying file from src_path to dst_path.
    """
    shutil.copy(src_path, dst_path)

def apply_config(cfg):
    """
    Apply config in cfg via pcli.
    cfg is a list of config lines (strings).
    """
    cmd = ";".join(cfg)
    out = util.pcli_command(cmd)
    logger.debug("pcli output:\n%s", out)
    if "Error:" in out:
        raise IOError("Encountered error when applying config: %s" % out)

def revert_default_config():
    """
    Revert to default config via pcli.
    """
    out = util.pcli_command("_internal; revert-default")
    logger.debug("pcli output:\n%s", out)

def save_running_config():
    """
    Save running config as startup config via pcli.
    """
    out = util.pcli_command("copy running-config startup-config")
    logger.debug("pcli output:\n%s", out)

def reload_config(ztn_server):
    """
    Get config from ZTN server and reload config.
    Return a tuple of (rc, error), where rc is 0 on success.
    """
    rc = 0
    error = None
    success = False

    try:
        last_cfg = read_last_ztn_config()
        ztn_json = read_ztn_json()

    except Exception, e:
        logger.exception("failed to read existing ZTN states")
        rc = 1
        error = str(e)
        return (rc, error)

    try:
        new_cfg, new_cfg_path, md5sum = get_config_from_ztn_server(ztn_server)

    except Exception, e:
        logger.exception("failed to get new ZTN config")
        rc = 2
        error = str(e)
        return (rc, error)

    try:
        # hit-less reload
        patch_cfg = create_patch_config(last_cfg, new_cfg)
        apply_config(patch_cfg)
        success = True
        logger.debug("hit-less reload completed")

    except Exception, e:
        logger.exception("failed to perform hit-less reload")

        try:
            # hit-ful reload, requires revert
            revert_default_config()
            apply_config(new_cfg)
            success = True
            logger.debug("hit-ful reload completed")

        except Exception, e:
            logger.exception("failed to perform hit-ful reload")
            rc = 3
            error = str(e)

            try:
                # perform roll-back
                revert_default_config()
                apply_config(last_cfg)
                logger.debug("roll-back completed")

            except Exception, e:
                logger.exception("fatal: failed to roll-back")
                rc = 4
                error = str(e)
                return (rc, error)

    try:
        save_running_config()

        if success:
            save_last_ztn_config(new_cfg_path)
            ztn_json["startup_config_md5"] = md5sum
            write_ztn_json(ztn_json)
            logger.debug("ztn states updated")

    except Exception, e:
        # FIXME: not much we can do if things fail at this point
        logger.exception("failed to update ztn states: %s", e)

    return (rc, error)
