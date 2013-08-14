import logging
from collections import deque
import pprint
from zlib import compress as zcompress

class CircularBufferHandler(logging.Handler):
  def __init__ (self, count=200, compress=False):
    super(CircularBufferHandler, self).__init__()
    if compress:
      self.emit = self._emit_compress
    else:
      self.emit = self._emit_fast
    self._store = deque(maxlen=count)

  def _emit_fast (self, record):
    self._store.append(self.format(record))

  def _emit_compress (self, record):
    self._store.append(zcompress(self.format(record), 1))

  def dump (self):
    return self._store


### Test Code ###

EXC = """
  File "<stdin>", line 2, in <module>
  File "/System/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/logging/__init__.py", line 1158, in error
    self._log(ERROR, msg, args, **kwargs)
  File "/System/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/logging/__init__.py", line 1250, in _log
    self.handle(record)
  File "/System/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/logging/__init__.py", line 1260, in handle
    self.callHandlers(record)
  File "/System/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/logging/__init__.py", line 1299, in callHandlers
    if record.levelno >= hdlr.level:
AttributeError: 'CircularBufferHandler' object has no attribute 'level'
"""

if __name__ == '__main__':
  # Never, ever, ever, EVER, use %(asctime)s - for small log messages creating the time string can be
  # up to 25% of the total runtime
  formatter = logging.Formatter('%(created)f - %(levelname)s - %(message)s')

  l = logging.getLogger("CircTest")

  cb = CircularBufferHandler(count=20)
  cb.setFormatter(formatter)
  l.addHandler(cb)

  cb2 = CircularBufferHandler(count=20, compress=True)
  cb2.setFormatter(formatter)
  l.addHandler(cb2)

  for x in xrange(250000):
    l.error("%d: Test Message that's a bit longer than before so maybe it can be compressed" % (x))
    l.error(EXC)

  uncl = cb.dump()
  unclen = 0
  for msg in uncl:
    unclen += len(msg)
  print "Uncompressed Bytes: %d" % (unclen)

  cl = cb2.dump()
  clen = 0
  for msg in cl:
    clen += len(msg)
  print "Compressed Bytes: %d" % (clen)
