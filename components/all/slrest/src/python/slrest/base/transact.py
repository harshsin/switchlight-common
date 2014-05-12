#!/usr/bin/python
############################################################
#
# SLREST Transaction Management
#
############################################################
from datetime import datetime
import logging
import threading
import uuid
import time
import os
import shutil
import json
from response import SLREST

# This is the global transaction collection timer.
GARBAGE_COLLECT_TIMEOUT=1

# This is the location for TransactionTask working directories
TRANSACTION_TASK_WORKDIR_BASE="/tmp/slrest"

class TransactionManagers(object):
    """
    Singleton class to store all TransactionManagers.
    """
    managers={}

    @classmethod
    def get(klass, name, max_=None):
        """
        Get a TransactionManager by name

        This will allocate a new TransactionManager or return
        the existing one by name.

        name : The name of the transaction manager.
        max_ : Passed to _TransactionManager() if creating a new object.
               See _TransactionManager.__init__()

        """

        if not name in klass.managers:
            klass.managers[name] = _TransactionManager(name, max_)
        return klass.managers[name]

    @classmethod
    def get_task(klass, tid):
        """
        Get any task with the given transaction id.
        """
        for m in klass.managers.values():
            tt = m.get_task(tid)
            if tt:
                return tt
        return None


    @classmethod
    def get_tids_running(klass):
        """
        Return all running transaction ids.
        """
        tids = [];
        for m in klass.managers.values():
            tids = tids + m.get_tids_running()
        return tids

    @classmethod
    def get_tids_finished(klass):
        """
        Return all finished transaction ids.
        """
        tids = []
        for m in klass.managers.values():
            tids = tids + m.get_tids_finished()
        return tids

    @classmethod
    def get_tids_all(klass):
        """
        Return all transaction ids.
        """
        tids = []
        for m in klass.managers.values():
            tids = tids + m.get_tids()
        return tids

    @classmethod
    def get_tasks_running(klass):
        """
        Return all running tasks.
        """
        tasks = []
        for m in klass.managers.values():
            tasks = tasks + m.get_tasks_running()
        return tasks

    @classmethod
    def get_tasks_finished(klass):
        """
        Return all finished tasks.
        """
        tasks = []
        for m in klass.managers.values():
            tasks = tasks + m.get_tasks_finished()
        return tasks

    @classmethod
    def get_tasks_all(klass):
        """
        Return all tasks.
        """
        tasks = []
        for m in klass.managers.values():
            tasks = tasks + m.get_tasks_all()
        return tasks




class _TransactionManager(object):
    """
    Global Transaction Manager Class

    All API transactions are submitted via a TransactionManager object.

    Most _TransactionManager objects are retrieved through
    the TransactionManagers factory.
    """

    def __init__(self, name, max_=None):
        """
        Create a new TransactionManager.

        name : The name of this transaction manager.
        max_ : The maximum number of outstanding transactions that should
               be allowed by the manager.

               None means unlimited.

        """
        self.name = name
        self.max = max_
        # These are the currently running transactions
        self.running = {}
        # These are completed transactions.
        self.finished = {}
        self.logger = logging.getLogger(name)
        self.lock = threading.Lock()
        self.newlock = threading.Lock()
        self.gc_start = datetime.utcnow()

    def new_task(self, transaction_class, path, args=None):
        """
        Create a new task
        """
        with self.newlock:
            if self.max and len(self.running) >= self.max:
                return (None, None)
            tt = transaction_class(self)
            with self.lock:
                self.running[tt.tid] = tt
            tt.path = path
            tt.args = args
            tt.start()
        return (tt.tid, tt)


    def get_task(self, tid):
        """
        Get the given task.
        """
        rv = self.__get_task(tid)
        if rv:
            # Whenever a task is requested its garbage collect time is reset.
            rv.gc_start = datetime.utcnow()
        # Garbage collect
        self.__gc()
        return rv

    def finish_task(self, tid):
        """Called by a TransactionTask object when it completes itself."""
        with self.lock:
            if tid in self.running:
                self.finished[tid] = self.running[tid]
                self.finished[tid].gc_start = datetime.utcnow()
                del self.running[tid]
                return True
            elif tid in self.finished:
                # Shouldn't happen, but ok
                return True
            else:
                return False


    def get_tids_running(self):
        """Return all currently running transaction ids."""
        return self.running.keys()

    def get_tids_finished(self):
        """Return all currently finished transaction ids."""
        return self.finished.keys()

    def get_tids_all(self):
        """Return all known ids."""
        with self.lock:
            return self.running.keys() + self.finished.keys()

    def get_tasks_running(self):
        return self.running.values()

    def get_tasks_finished(self):
        return self.finished.values()

    def get_tasks_all(self):
        with self.lock:
            return self.running.values() + self.finished.values()


    def __get_task(self, tid):
        """
        Get the given task (internal lookup only).
        """
        rv = None
        with self.lock:
            if tid in self.running:
                rv = self.running[tid]
            if tid in self.finished:
                rv = self.finished[tid]
        return rv


    def __gc(self):
        """Perform garbage collection on finished tasks."""
        with self.lock:
            for k in self.finished.keys():
                tt = self.finished[k]
                t = datetime.utcnow() - tt.gc_start
                if t.total_seconds() >= GARBAGE_COLLECT_TIMEOUT:
                    del self.finished[k]
                    tt.cleanup()
        self.__gc_orphans()

    def __gc_orphans(self):
        """
        Garbage collect any transaction workspaces that have been orphaned.

        Any transaction directories that aren't associated with any current
        transaction ids are orphaned and should probably be cleaned up.
        """
        for e in os.listdir(TRANSACTION_TASK_WORKDIR_BASE):
            if self.__get_task(e) is None:
                shutil.rmtree("%s/%s" % (TRANSACTION_TASK_WORKDIR_BASE,
                                         e))

    def __str__(self):
        """String Representation"""
        s = ''
        with self.lock:
            s += "TransactionManager: %s\n" % self.name
            s += "  Maximum Tasks: %s\n" % self.max
            s += "  Running: %d\n" % len(self.running)
            s += "  Finished: %d\n" % len(self.finished)
        return s




class TransactionTask(object):
    """
    Individual Transaction Implementations.

    TransactionTasks are created for each requested transaction
    by the parent TransactionManager.

    You should derive from this class and provide the
    handler() and handler_cleanup()
    methods to implement your transaction's functionality.

    """

    def __init__(self, tm):
        """
        Create a TransactionTask object.

        tm : The parent transaction manager.
        """

        # Parent transaction manager.
        self.tm = tm;
        # Our transaction id
        self.tid = str(uuid.uuid4())
        # When we started - populated when the start() method is invoked.
        self.started = None
        # When we finished - populated when the task finishes itself.
        self.finished = None
        # Per-task logger
        self.logger = logging.getLogger("%s:%s" % (tm.name, self.tid))


        #
        # These keys are required by the response protocol.
        #
        self.status = SLREST.Status.ACCEPTED
        self.reason = "The transaction is still being processed."
        self.transaction = "/api/v1/transaction?id=%s" % self.tid
        self.data = None
        self.path = None

        #
        # All objects inherit a task-specific work area that
        # will persist for the lifetime of the task.
        #
        self.workdir = "%s/%s" % (TRANSACTION_TASK_WORKDIR_BASE, self.tid)
        os.makedirs(self.workdir)

        self.worker = None

    def start(self):
        """Start processing our task."""
        self.worker = threading.Thread(target=self.__handler)
        self.started = datetime.utcnow()
        self.worker.start()

    def finish(self):
        """Called by the transaction handler when it has completed its task."""
        self.finished = datetime.utcnow()
        self.duration = (self.finished - self.started).total_seconds()
        self.tm.finish_task(self.tid)

    def cleanup(self):
        # Call any custom cleanup
        self.__handler_cleanup()
        # Destroy our working directory
        shutil.rmtree(self.workdir)

    def __handler(self):
        """This internal method handles exception processing for derived handlers."""
        try:
            self.handler()
        except Exception, e:
            self.reason = "Internal Exception (%s): %s" % (self.handler, e)
            self.status = SLREST.Status.ERROR
            self.finish()

    def __handler_cleanup(self):
        """This internal method handles exception processing from derived handler_cleanups."""
        try:
            self.handler_cleanup()
        except:
            pass

    def handler(self):
        """
        This method handles the actual transaction.
        """
        self.status = SLREST.Status.OK
        self.reason = "Didn't do anything."
        self.data = None
        self.finish()

    def handler_cleanup(self):
        """
        This method performs custom cleanup for a handler.

        The task's work directory is automatically deallocated when
        the task is deleted. This method is only required if you have
        cleanup other than removing the contents of the work directory.
        """
        return True


    def join(self):
        """Join with the worker task."""
        if self.worker:
            self.worker.join()
            self.worker = None


    def running(self):
        """Is the task currently running."""
        if self.worker:
            if self.finished:
                self.join()
                return False
            else:
                return True
        else:
            return False


    def __str__(self):
        s = ""
        s += "Type: %s\n" % self.tm.name
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
        if self.started:
            s += "Started: %s\n" % self.started
        if self.finished:
            s += "Finished: %s\n" % self.finished
            s += "Duration: %d seconds\n" % self.duration
            s += "Status: %s\n" % self.status
            s += "Reason: %s\n" % self.reason
            if self.data:
                s += "Data:\n"
                s += repr(self.data)
            else:
                s += "Data: %s" % self.data
        return s


    def response(self):
        """
        Generate the JSON response message for our current state.
        """
        return SLREST.response(self.path,
                               self.status,
                               self.reason,
                               self.transaction,
                               self.data)


if __name__ == "__main__":

    def simple_test(t):
        class Simple(TransactionTask):
            def handler(self):
                time.sleep(self.args)
                self.status = SLREST.Status.OK
                self.reason = "Finished up in %d seconds" % (self.args)
                self.data = dict(args=self.args)
                self.finish()

        print "simple_test(%d)" % t
        tm = TransactionManagers.get("simple")
        now = datetime.utcnow()
        (tid, tt) = tm.new_task(Simple, "simple_path", t)
        r=True
        while tt.running():
            time.sleep(1/4)
            if r:
                print tt.response()
                r = False

        tt = tm.get_task(tid)

        if int(tt.duration) != t:
            raise Exception("Failed: duration=%s, arg=%d" % (tt.duration, t))
        print tt
        print tt.response()
        print tm
        print "Passed"
        return True

    simple_test(1)
    simple_test(2)
    simple_test(3)
    simple_test(4)

