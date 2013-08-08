# Copyright (c) 2013  BigSwitch Networks
# Portions Copyright (c) 2011-2013  Barnstormer Softworks, Ltd. (GENI Public License)

class DPID(object):
  MAX = (2 ** 64) - 1

  class OutOfRangeError(Exception):
    def __init__ (self, val):
      super(OutOfRangeError, self).__init__()
      self.val = val
    def __str__ (self):
      return "Input value (%d) out of range of valid DPIDs" % (self.val)

  class InputTypeError(Exception):
    def __init__ (self, val):
      super(InputTypeError, self).__init__()
      self.val = val
    def __str__ (self):
      return "Input value (%s) has invalid type (%s)" % (self.val, type(self.val))

  def __init__ (self, val):
    self._dpid = None

    if isinstance(val, (unicode, str)):
      val = int(val.replace(":", ""), 16)

    if isinstance(val, (int, long)):
      if val < DPID.MAX and val >= 0:
        self._dpid = val
      else:
        raise DPID.OutOfRangeError(val)
    else:
      raise DPID.InputTypeError(val)

  def __eq__ (self, other):
    return self._dpid == other._dpid

  def __hash__ (self):
    return self._dpid

  def __str__ (self):
    s = self.hexstr()
    return ":".join(["%s%s" % (s[x], s[x+1]) for x in xrange(0,15,2)])

  def __repr__ (self):
    return str(self)

  def __json__ (self):
    return str(self)
  
  def hexstr (self):
     return "%016x" % (self._dpid) 

  def intstr (self):
    return str(self._dpid)

