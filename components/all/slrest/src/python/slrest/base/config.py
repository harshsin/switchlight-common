#!/usr/bin/python
############################################################
#
# SwitchLight Config Library
#
############################################################

from slrest.base import util

import re
import urllib2

# Used for filtering out unneeded lines in config
FILTER_REGEX = re.compile(r"^SwitchLight.*|^.*?\(config\).*|^Exiting.*|^\!")

logger = None

def set_logger(logger_):
    global logger
    logger = logger_

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
        raise ValueError("Verify configs failed.\n"
                         "Missing lines:\n%s\n"
                         "Extra lines:\n%s" % (lines1, lines2))

def get_config_from_url(url):
    """
    Fetch config from a url.
    Filter out unneeded lines.
    Return config as a list of config lines (strings).
    """
    if logger:
        logger.debug("Fetching config from url: %s" % url)

    cfg = []
    try:
        f = urllib2.urlopen(url)
        for l in f.read().split("\n"):
            l = l.strip()
            if len(l) == 0 or FILTER_REGEX.match(l):
                continue
            cfg.append(l)

    except (urllib2.HTTPError, urllib2.URLError):
        if logger:
            logger.exception("Error getting config from url.\n%s" % url)
        raise

    return cfg

def get_startup_config():
    """
    Get startup config from pcli.
    Filter out unneeded lines.
    Return config as a list of config lines (strings).
    """
    cfg = []
    out = util.pcli_command("show running-config")
    for l in out.split("\n"):
        l = l.strip()
        if len(l) == 0 or FILTER_REGEX.match(l):
            continue
        cfg.append(l)
    return cfg

def get_running_config():
    """
    Get running config from pcli.
    Filter out unneeded lines.
    Return config as a list of config lines (strings).
    """
    cfg = []
    out = util.pcli_command("show running-config")
    for l in out.split("\n"):
        l = l.strip()
        if len(l) == 0 or FILTER_REGEX.match(l):
            continue
        cfg.append(l)
    return cfg

def create_patch_config(old_cfg, new_cfg):
    """
    Create patch config from old_cfg and new_cfg.
    old_cfg and new_cfg are lists of config lines (strings).
    Return patch config as a list of config lines (strings).
    """
    old_lines, new_lines = compare_configs(old_cfg, new_cfg)
    if logger:
        logger.debug("old_lines:\n%s" % old_lines)
        logger.debug("new_lines:\n%s" % new_lines)

    # create patch config:
    # - remove a config line by prepending it with a "no"
    # - add a config line as is
    # FIXME: logic might need to be revisited
    return ["no %s" % l for l in old_lines] + new_lines

def apply_config(cfg):
    """
    Apply config in cfg via pcli.
    cfg is a list of config lines (strings).
    """
    cmd = ";".join(cfg)
    out = util.pcli_command(cmd)
    if logger:
        logger.debug("pcli output:\n%s" % out)

def save_running_config():
    """
    Save running config as startup config via pcli.
    """
    out = util.pcli_command("copy running-config startup-config")
    if logger:
        logger.debug("pcli output:\n%s" % out)

def update_config_from_url(url):
    """
    Get config from url and update switch with config.
    """
    new_cfg = get_config_from_url(url)
    old_cfg = get_running_config()
    if logger:
        logger.debug("old_cfg:\n%s" % old_cfg)
        logger.debug("new_cfg:\n%s" % new_cfg)

    patch_cfg = create_patch_config(old_cfg, new_cfg)
    if logger:
        logger.debug("patch_cfg:\n%s" % patch_cfg)

    apply_config(patch_cfg)
    post_cfg = get_running_config()
    verify_configs(post_cfg, new_cfg)
    save_running_config()
