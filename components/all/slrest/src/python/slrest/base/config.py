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
FILTER_REGEX_LIST = [re.compile(r"^SwitchLight.*"),
                     re.compile(r"^.*?\(config\).*"),
                     re.compile(r"^Exiting.*"),
                     re.compile(r"^\!")]

# FIXME: polish/refactor
# FIXME: connection will break if new config changes mgmt network settings
class SLConfig(object):
    @staticmethod
    def compare_configs(old_cfg, new_cfg):
        """
        Do a diff between old_cfg and new_cfg.
        old_cfg and new_cfg are lists of config lines (strings).
        Return lines that are removed and lines that are added.
        """
        lines_to_remove = [l for l in old_cfg if l not in new_cfg]
        lines_to_add = [l for l in new_cfg if l not in old_cfg]
        return (lines_to_remove, lines_to_add)

    @staticmethod
    def verify_configs(cfg1, cfg2, logger=None):
        """
        Verify that cfg1 and cfg2 are the same.
        cfg1 and cfg2 are lists of config lines (strings).
        """
        lines1, lines2 = SLConfig.compare_configs(cfg1, cfg2)
        if logger:
            logger.debug("lines1:\n%s" % lines1)
            logger.debug("lines2:\n%s" % lines2)
        if len(lines1) != 0 or len(lines2) != 0:
            if logger:
                logger.error("Verify configs failed.")
            raise Exception("Verify configs failed.")

    @staticmethod
    def match_regex_list(s, regex_list):
        """
        Match s (string) against a list of regex.
        Return True on the first match, and False if there are no matches.
        """
        for regex in regex_list:
            if regex.match(s):
                return True
        return False

    @staticmethod
    def get_config_from_url(url, logger=None):
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
                if len(l) == 0 or SLConfig.match_regex_list(l, FILTER_REGEX_LIST):
                    continue
                cfg.append(l)

        except Exception as e:
            if logger:
                logger.error("Error getting config from url:\n%s" % e)
            raise

        return cfg

    @staticmethod
    def get_startup_config(logger=None):
        """
        Get startup config from pcli.
        Filter out unneeded lines.
        Return config as a list of config lines (strings).
        """
        cfg = []
        try:
            out = util.pcli_command("show running-config")
            for l in out.split("\n"):
                l = l.strip()
                if len(l) == 0 or SLConfig.match_regex_list(l, FILTER_REGEX_LIST):
                    continue
                cfg.append(l)

        except Exception as e:
            if logger:
                logger.error("Error getting config from pcli:\n%s" % e)
            raise

        return cfg

    @staticmethod
    def get_running_config(logger=None):
        """
        Get running config from pcli.
        Filter out unneeded lines.
        Return config as a list of config lines (strings).
        """
        cfg = []
        try:
            out = util.pcli_command("show running-config")
            for l in out.split("\n"):
                l = l.strip()
                if len(l) == 0 or SLConfig.match_regex_list(l, FILTER_REGEX_LIST):
                    continue
                cfg.append(l)

        except Exception as e:
            if logger:
                logger.error("Error getting config from pcli:\n%s" % e)
            raise

        return cfg

    @staticmethod
    def create_patch_config(old_cfg, new_cfg, logger=None):
        """
        Create patch config from old_cfg and new_cfg.
        old_cfg and new_cfg are lists of config lines (strings).
        Return patch config as a list of config lines (strings).
        """
        old_lines, new_lines = SLConfig.compare_configs(old_cfg, new_cfg)
        if logger:
            logger.debug("old_lines:\n%s" % old_lines)
            logger.debug("new_lines:\n%s" % new_lines)

        # create patch config:
        # - remove a config line by prepending it with a "no"
        # - add a config line as is
        # FIXME: logic might need to be revisited
        return ["no %s" % l for l in old_lines] + new_lines

    @staticmethod
    def apply_config(cfg, logger=None):
        """
        Apply config in cfg via pcli.
        cfg is a list of config lines (strings).
        """
        try:
            cmd = ";".join(cfg)
            out = util.pcli_command(cmd)
            if logger:
                logger.debug("pcli output:\n%s" % out)

        except Exception as e:
            if logger:
                logger.error("Error apply config:\n%s" % e)
            raise

    @staticmethod
    def save_running_config(logger=None):
        """
        Save running config as startup config via pcli.
        """
        try:
            out = util.pcli_command("copy running-config startup-config")
            if logger:
                logger.debug("pcli output:\n%s" % out)

        except Exception as e:
            if logger:
                logger.error("Error saving config:\n%s" % e)
            raise

    @staticmethod
    def update_config_from_url(url, logger=None):
        """
        Get config from url and update switch with config.
        """
        new_cfg = SLConfig.get_config_from_url(url, logger)
        old_cfg = SLConfig.get_running_config(logger)
        if logger:
            logger.debug("old_cfg:\n%s" % old_cfg)
            logger.debug("new_cfg:\n%s" % new_cfg)

        patch_cfg = SLConfig.create_patch_config(old_cfg, new_cfg, logger)
        if logger:
            logger.debug("patch_cfg:\n%s" % patch_cfg)

        SLConfig.apply_config(patch_cfg, logger)
        post_cfg = SLConfig.get_running_config(logger)
        SLConfig.verify_configs(post_cfg, new_cfg, logger)
        SLConfig.save_running_config(logger)
