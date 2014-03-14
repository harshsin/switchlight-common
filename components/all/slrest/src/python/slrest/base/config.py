#!/usr/bin/python
############################################################
#
# SwitchLight Config Library
#
############################################################

from slrest.base import util

from datetime import datetime
import logging
import re
import urllib2
import threading
import Queue

# Used for filtering out unneeded lines in config
FILTER_REGEX = re.compile(r"^SwitchLight.*|^.*?\(config\).*|^Exiting.*|^\!|^[\s]*$")

# Set default logger
logger = logging.getLogger("config")

# worker thread and data structures
worker = None
worker_id = None
worker_lock = threading.Lock()

# worker result queue
result_queue = Queue.Queue()

class ResultQueueEntry(object):
    def __init__(self, transaction_id, exception=None):
        self.transaction_id = transaction_id
        self.finished = datetime.utcnow()
        self.exception = exception

class TransactionEntry(object):
    def __init__(self, transaction_id):
        self.lock = threading.Lock()
        self.transaction_id = transaction_id
        self.started = datetime.utcnow()
        self.finished = None
        self.error = None

    def to_json(self):
        return dict([(k, str(v)) for k, v in self.__dict__.items() if k != "lock"])

class Transactions(object):
    transactions = {}
    counter = 1
    counter_lock = threading.Lock()

    @classmethod
    def new_entry(klass):
        with klass.counter_lock:
            transaction_id = klass.counter
            klass.counter += 1

        entry = TransactionEntry(transaction_id)
        klass.transactions[transaction_id] = entry
        logger.debug("transaction created: %d" % transaction_id)
        return transaction_id

    @classmethod
    def get_entry(klass, transaction_id):
        if transaction_id not in klass.transactions:
            # FIXME: consider other exception classes?
            raise ValueError("transaction id %d is not found" % transaction_id)

        entry = klass.transactions[transaction_id]
        with entry.lock:
            return entry.to_json()

    @classmethod
    def update_entry(klass, transaction_id, finished, error=None):
        if transaction_id not in klass.transactions:
           # FIXME: consider other exception classes?
            raise ValueError("transaction id %d is not found" % transaction_id)

        entry = klass.transactions[transaction_id]
        with entry.lock:
            entry.finished = finished
            entry.error = error
        logger.debug("transaction updated: %d" % transaction_id)

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
        logger.error("Missing lines:\n%s" % lines1)
        logger.error("Extra lines:\n%s" % lines2)
        raise ValueError("Verify configs failed.")

def get_config_from_url(url):
    """
    Fetch config from a url.
    Filter out unneeded lines.
    Return config as a list of config lines (strings).
    """
    logger.debug("Fetching config from url: %s" % url)

    try:
        f = urllib2.urlopen(url)
        cfg = [l for l in f.read().splitlines() if not FILTER_REGEX.match(l)]

    except (urllib2.HTTPError, urllib2.URLError):
        logger.exception("Error getting config from url.\n%s" % url)
        raise

    return cfg

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
    logger.debug("pcli output:\n%s" % out)

def save_running_config():
    """
    Save running config as startup config via pcli.
    """
    out = util.pcli_command("copy running-config startup-config")
    logger.debug("pcli output:\n%s" % out)

def update_config_worker(url, transaction_id, result_queue):
    """
    Get config from url and update switch with config.
    """
    try:
        new_cfg = get_config_from_url(url)
        old_cfg = get_running_config()
        logger.debug("old_cfg:\n%s" % old_cfg)
        logger.debug("new_cfg:\n%s" % new_cfg)

        patch_cfg = create_patch_config(old_cfg, new_cfg)
        logger.debug("patch_cfg:\n%s" % patch_cfg)

        apply_config(patch_cfg)
        post_cfg = get_running_config()
        verify_configs(post_cfg, new_cfg)
        save_running_config()
        logger.debug("update config finished.")
        result_queue.put(ResultQueueEntry(transaction_id))

    except Exception as e:
        logger.exception("update config failed.")
        result_queue.put(ResultQueueEntry(transaction_id, e))

def start_update_config_task(url):
    """
    Start a worker thread to update switch config.
    Returns the transaction id associated with the worker.
    """
    global worker
    global worker_id

    with worker_lock:
        if worker is not None:
            if worker.is_alive():
                raise Exception("Update config is already running. Transaction id: %d" % worker_id)
            else:
                logger.debug("Joining worker. Transaction id: %d" % worker_id)
                worker.join()
                worker = None
                worker_id = None

        worker_id = Transactions.new_entry()
        worker = threading.Thread(target=update_config_worker, args=(url, worker_id, result_queue,))
        logger.debug("Starting worker. Transaction id: %d" % worker_id)
        worker.start()

    return worker_id

def wait_for_update_config_task():
    """
    Wait until the current worker thread finishes, if it exists.
    """
    global worker
    global worker_id

    with worker_lock:
        if worker is not None:
            logger.debug("Joining worker. Transaction id: %d" % worker_id)
            worker.join()
            worker = None
            worker_id = None

def get_update_config_status(transaction_id):
    """
    Get update status associated with transaction_id.
    """
    # check result queue and update transactions.
    while True:
        try:
            result = result_queue.get(True, 1) # block for 1 second
            result_queue.task_done()
            Transactions.update_entry(result.transaction_id, result.finished, result.exception)

        except Queue.Empty:
            logger.debug("result queue is empty")
            break

    return Transactions.get_entry(transaction_id)
