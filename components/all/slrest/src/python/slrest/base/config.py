#!/usr/bin/python
############################################################
#
# SwitchLight Config Library
#
############################################################

from slrest.base import util
import logging
import re
import yaml

from datetime import datetime

# Set default logger
logger = logging.getLogger("config")

def set_logger(logger_):
    global logger
    logger = logger_

# Used for filtering out unneeded lines in config
FILTER_REGEX = re.compile(r"^SwitchLight.*|^.*?\(config\).*|^Exiting.*|^\!|^[\s]*$")

# Used for converting date string into datetime object
DATETIME_FMT_STR = "%a %b %d %X %Z %Y"

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
    Return config as a list of config lines (strings).
    """
    logger.debug("Fetching config from ztn server: %s" % ztn_server)

    (rc, out) = util.bash_command("ztn --transact --server %s" % ztn_server)
    if rc:
        raise IOError("Failed to transact with ZTN server (%s): %s" % \
                      (ztn_server, out))

    (rc, out) = util.bash_command("ztn --inventory")
    if rc:
        raise IOError("Failed to get ZTN inventory: %s" % out)

    inv = yaml.load(out)
    logger.debug("ztn inv: %s" % inv)

    # get the latest entry
    md5sum = None
    path = None
    latest = None
    for k, v in inv.get("startup-config", {}).items():
        curr = datetime.strptime(str(v["date"]), DATETIME_FMT_STR)
        if (latest is None) or (curr > latest):
            latest = curr
            md5sum = k
            path = str(v["path"])

    if latest is None:
        raise IOError("Failed to find startup-config entries in ZTN inv")

    # parse config
    f = open(path)
    cfg = [l for l in f.read().splitlines() if not FILTER_REGEX.match(l)]
    return (cfg, md5sum)

def get_startup_config():
    """
    Get startup config from pcli.
    Filter out unneeded lines.
    Return config as a list of config lines (strings).
    """
    out = util.pcli_command("show running-config")
    cfg = [l for l in out.splitlines() if not FILTER_REGEX.match(l)]
    return cfg

def get_running_config():
    """
    Get running config from pcli.
    Filter out unneeded lines.
    Return config as a list of config lines (strings).
    """
    out = util.pcli_command("show running-config")
    cfg = [l for l in out.splitlines() if not FILTER_REGEX.match(l)]
    return cfg

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
    #
    # (2) for new lines:
    #   - apply as is
    #
    # FIXME: logic might need to be revisited
    return [l[3:] if l.startswith("no ") else "no %s" % l \
            for l in old_lines] + new_lines

def apply_config(cfg):
    """
    Apply config in cfg via pcli.
    cfg is a list of config lines (strings).
    """
    cmd = ";".join(cfg)
    out = util.pcli_command(cmd)
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
        new_cfg, md5sum = get_config_from_ztn_server(ztn_server)
        old_cfg = get_running_config()
        logger.debug("old_cfg:\n%s" % old_cfg)
        logger.debug("new_cfg:\n%s" % new_cfg)

        patch_cfg = create_patch_config(old_cfg, new_cfg)
        logger.debug("patch_cfg:\n%s" % patch_cfg)

        apply_config(patch_cfg)
        post_cfg = get_running_config()
        verify_configs(new_cfg, post_cfg)
        save_running_config()
        logger.debug("update config finished.")

    except Exception, e:
        logger.exception("update config failed.")
        rc = 1
        error = str(e)

    return (rc, error)
