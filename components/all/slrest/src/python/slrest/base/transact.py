#!/usr/bin/python
############################################################
#
# SwitchLight Transactions
#
############################################################

from datetime import datetime
import logging
import re
import urllib2
import threading
import Queue
import uuid
import time

class TransactionManager(object):
    managers={}

    def __str__(self):
        s = ''
        with self.lock:
            s += "TransactionManager: %s\n" % self.name
            s += "  Maximum Tasks: %d\n" % self.max
            s += "  Running: %d\n" % len(self.running)
            s += "  Finished: %d\n" % len(self.finished)
        return s

    @classmethod
    def get(klass, name, max_=0):
        if not name in klass.managers:
            klass.managers[name] = TransactionManager(name, max_)
        return klass.managers[name]

    def __init__(self, name, max_):
        self.name = name
        self.max = max_
        self.running = {}
        self.finished = {}
        self.logger = logging.getLogger(name)
        self.lock = threading.Lock()
        self.newlock = threading.Lock()

    def userlock(self):
        self.userlock.lock()

    def userunlock(self):
        self.userlock.unlock()

    def outstanding(self):
        return len(self.transactions)

    def new_task(self, task, args):
        self.logger.info("new task")
        with self.newlock:
            if self.max and len(self.running) >= self.max:
                return (None, None)
            tt = TransactionTask(self, task, args)
            with self.lock:
                self.running[tt.tid] = tt
            tt.start()
        return (tt.tid, tt)

    def finish_task(self, tid):
        with self.lock:
            if tid in self.running:
                self.finished[tid] = self.running[tid]
                del self.running[tid]
                return True
            elif tid in self.finished:
                # Shouldn't happen, but ok
                return True
            else:
                return False

    def get_task(self, tid, clean=False):
        with self.lock:
            if tid in self.running:
                return self.running[tid]
            if tid in self.finished:
                tt = self.finished[tid]
                if clean:
                    del self.finished[tid]
                return tt
        return None

class TransactionTask(object):
    def __init__(self, parent, task, args):
        self.parent = parent
        self.tid = str(uuid.uuid4())
        self.started = datetime.utcnow()
        self.finished = None
        self.exception = None
        self.logger = logging.getLogger("%s:%s" % (parent.name, self.tid))
        self.result = None
        self.rc = None
        self.worker = threading.Thread(target=task, args=(self,)+args)

    def start(self):
        self.worker.start()

    def __str__(self):
        s = ""
        s += "Type: %s\n" % self.parent.name
        s += "ID: %s\n" % self.tid
        s += "Started: %s\n" % self.started
        s += "Status: "
        if self.worker:
            if self.worker.is_alive():
                s += "Running."
            else:
                s += "Completed."
        else:
            s += "Completed."
        s += '\n'
        if self.finished:
            s += "Finished: %s\n" % self.finished
        if self.exception:
            s += 'Exception: %s\n' % self.exception
        if self.result:
            s += 'Result: %s\n' % self.result
        if self.rc:
            s += 'Returned: %s\n' % self.rc

        return s

    def _worker_finished(self, rc, result, exception=None):
        self.finished = datetime.utcnow()
        self.result = result
        self.rc = rc
        self.exception = exception
        self.parent.finish_task(self.tid)

    def join(self):
        if self.worker is not None:
            self.logger.debug("Joining worker")
            self.worker.join()
            self.worker = None

    def running(self):
        if self.worker:
            if self.finished:
                self.join()
                return False
            else:
                return True
        else:
            return False


if __name__ == "__main__":
    def transhandler(parent, sleep, prnt):
        parent.logger.info("sleep=%d, prnt=%s" % (sleep, prnt))
        time.sleep(sleep)
        parent._worker_finished(True, "Finished")

    tm = TransactionManager.get("ZTN")
    (tid,tt) = tm.new_task(transhandler, (5, "Hello, World!"))

    while tt.running():
        print tt
        print tm
        time.sleep(1)

    print tt
    print tm


