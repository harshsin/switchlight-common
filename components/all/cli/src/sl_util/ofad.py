# Copyright (c) 2013  BigSwitch Networks

import os
import os.path
from contextlib import contextmanager
import json
import copy

import error
import subprocess

from sl_util import shell
from sl_util.types import DPID

class Controller(object):
    def __init__ (self):
        self._addr = None
        self._port = None
        self._protocol = None
        self._cfg_static = None
        self._cfg_dhcp = None

    def __str__ (self):
        return "<%s:%s:%d>" % (self._protocol, self._addr, self._port)

    def __eq__ (self, other):
      """
      Controller objects are determined to be equal if they have the same
      (addr, port, protocol) tuple.  Configuration bits are NOT considered.
      """
      if self._addr != other._addr:
        return False
      if self._protocol != other._protocol:
        return False
      if self._port != other._port:
        return False
      return True

    def toJSON (self):
        if self._cfg_dhcp is None:
            self._cfg_dhcp = False
        if self._cfg_static is None:
            self._cfg_static = False
        return {"ip_addr" : self._addr, "port" : self._port,
                "protocol" : self._protocol, "cfg_dhcp" : self._cfg_dhcp,
                "cfg_static" : self._cfg_static}

    @property
    def addr (self):
        return self._addr

    def setAddress (self, addr):
        self._addr = addr
        return self
    
    @property
    def port (self):
        return self._port

    def setPort (self, port):
        self._port = port
        return self

    @property
    def protocol (self):
        return self._protocol

    def setProtocol (self, proto):
        self._protocol = proto
        return self

    @property
    def fromDHCP (self):
        return self._cfg_dhcp

    def setDHCP (self, dhcp):
        self._cfg_dhcp = dhcp
        return self

    @property
    def fromStatic (self):
        return self._cfg_static

    def setStatic (self, static):
        self._cfg_static = static
        return self

    def merge (self, other):
        changed = False
        if ((self._addr == other._addr) and (self._port == other._port)
            and (self._protocol == other._protocol)):
            if other._cfg_static is not None:
                if other._cfg_static != self._cfg_static:
                    self._cfg_static = other._cfg_static
                    changed = True
            if other._cfg_dhcp is not None:
                if other._cfg_dhcp != self._cfg_dhcp:
                    self._cfg_dhcp = other._cfg_dhcp
                    changed = True
        return changed

PHY_PORT_TYPE = "physical"
LAG_PORT_TYPE = "lag"
LAG_BASE_PORT_NUM = 60

class Port(object):
    """
    Base port class.

    Each object var that starts with "_" is processed by toJSON().
    """
    def __init__ (self, name, type_):
        self.port_name = name
        self._port_type = type_
        self._port_number = None

    def toJSON (self):
        d = {}
        for k, v in self.__dict__.iteritems():
            if not k.startswith("_") or v is None:
                continue
            d[k[1:]] = v
        return d

    @property
    def portName (self):
        return self.port_name

    def setPortNumber (self, portNumber):
        self._port_number = portNumber

class PhysicalPort(Port):
    def __init__ (self, name):
        Port.__init__(self, name, PHY_PORT_TYPE)
        # physical ports don't need port_type in ofad.conf
        delattr(self, "_port_type")

class LAGPort(Port):
    def __init__ (self, name):
        Port.__init__(self, name, LAG_PORT_TYPE)
        self._component_ports = None
        self._hash = None
        self._mode = None

    @property
    def componentPorts (self):
        return self._component_ports

    def setComponentPorts (self, ports):
        self._component_ports = ports

    @property
    def hash (self):
        return self._hash

    def setHash (self, hash_):
        self._hash = hash_

    def setMode (self, mode):
        self._mode = mode

class PortManager(object):
    # class variables for port base names
    phy_base = "_phy_base_"
    lag_base = "_lag_base_"

    def __init__ (self, port_list):
        # A dict of configured physical ports
        self.phys = {}

        # A dict of configured lag ports
        # A lag should contain one or more physical ports as component ports
        self.lags = {}

        # A list of all physical ports
        # This is a fixed list that should not change after init
        # This is essentially the sum of all standalone physicalports and lag component ports
        self.all_phys = []

        # Parse port_list
        self.__parsePortList(port_list)

    def __parsePortList (self, port_list):
        for k, v in port_list.iteritems():
            port_type = v.get("port_type", PHY_PORT_TYPE)

            if port_type == PHY_PORT_TYPE:
                port = PhysicalPort(k)
                self.phys[k] = port
                self.all_phys.append(k)

            elif port_type == LAG_PORT_TYPE:
                port = LAGPort(k)
                port.setComponentPorts(v["component_ports"])
                port.setPortNumber(v["port_number"])
                port.setHash(v.get("hash", None))
                port.setMode(v.get("mode", None))
                self.lags[k] = port
                self.all_phys += [PortManager.getPhysicalName(p) for p in v["component_ports"]]

    def __freePhysicalPort (self, name):
        assert name not in self.phys
        port = PhysicalPort(name)
        self.phys[name] = port

    def __usePhysicalPort (self, name):
        assert name in self.phys
        self.phys.pop(name)

    def __addLAGPort (self, name, ports, port_num, hash_=None, mode=None):
        assert name not in self.lags
        for p in ports:
            phy_port = PortManager.getPhysicalName(p)
            self.__usePhysicalPort(phy_port)

        port = LAGPort(name)
        port.setComponentPorts(ports)
        port.setPortNumber(port_num)
        port.setHash(hash_)
        port.setMode(mode)
        self.lags[name] = port

    def __removeLAGPort (self, name):
        assert name in self.lags
        port = self.lags.pop(name)
        for p in port.componentPorts:
            phy_port = PortManager.getPhysicalName(p)
            self.__freePhysicalPort(phy_port)

    @classmethod
    def setPhysicalBase (cls, base):
        cls.phy_base = base

    @classmethod
    def setLAGBase (cls, base):
        cls.lag_base = base

    @staticmethod
    def getPhysicalName (id_):
        return "%s%d" % (PortManager.phy_base, id_)

    @staticmethod
    def getLAGName (id_):
        return "%s%d" % (PortManager.lag_base, id_)

    @staticmethod
    def getLAGId (name):
        return int(name[len(PortManager.lag_base):])

    def checkValidPhysicalPort (self, port):
        name = PortManager.getPhysicalName(port)
        if name not in self.all_phys:
            raise error.ActionError("%s is not a valid interface" % name)

    # FIXME: optimize on updating an existing LAG
    def configureLAGPort (self, id_, ports, hash_=None, mode=None):
        name = PortManager.getLAGName(id_)
        port_num = LAG_BASE_PORT_NUM + id_
        if name in self.lags:
            self.__removeLAGPort(name)

        for p in ports:
            phy_port = PortManager.getPhysicalName(p)
            if phy_port not in self.phys:
                raise error.ActionError("%s is already in use" % phy_port)

        self.__addLAGPort(name, ports, port_num, hash_, mode)

    def unconfigureLAGPort (self, id_, ports=None):
        name = PortManager.getLAGName(id_)
        if name not in self.lags:
            return

        lag_port = self.lags[name]
        if ports and ports != lag_port.componentPorts:
            raise error.ActionError("Port list specified does not match configured value")

        self.__removeLAGPort(name)

    def getLAGs (self):
        return self.lags.values()

    def toJSON (self):
        port_list = {}
        for d in [self.phys, self.lags]:
            for k, port in d.iteritems():
                port_list[k] = port.toJSON()
        return port_list

class ConfigChangedError(Exception):
    def __init__ (self, warn_count):
        self.woc = warn_count
    def __str__ (self):
        return "Configuration changed by another process during edit - aborting!"

class ConfigNotFoundError(Exception):
    def __init__ (self, path):
        self.path = path
    def __str__ (self):
        "Required configuration file not found: %s" % (self.path)

class UnknownVersionError(Exception):
    def __init__ (self, version):
        self.version = version
    def __str__ (self):
        "Unknown version: %d" % (self.version)

class OFADConfig(object):
    PATH = "/etc/ofad.conf"
    VERSION = 2
    
    def __init__ (self):
        self._cache_tinfo = (0,0)
        self._data_cache = None
        self._warn_on_changed = 0

    @contextmanager
    def warnOnChanged (self, warn=True):
        if warn:
            self._warn_on_changed += 1
        try:
            yield
        except ConfigChangedError, e:
            print str(e)
        finally:
            if warn:
                self._warn_on_changed -= 1

    @property
    def failover_mode (self):
        return self._data["failover_mode"]

    @property
    def dpid (self):
        return self._data["of_datapath_id"]
    
    @dpid.setter
    def dpid (self, val):
        if not isinstance(val, DPID):
            raise TypeError(val)
        self._data["of_datapath_id"] = val

    @property
    def of_dp_desc (self):
        return self._data["of_dp_desc"]

    @of_dp_desc.setter
    def of_dp_desc (self, val):
        self._data["of_dp_desc"] = val

    @property
    def of_mac_addr_base (self):
        return self._data["of_mac_addr_base"]

    @property
    def controllers (self):
        controllers = []
        for o in self._data["controllers"]:
            cont = Controller()
            cont.setAddress(o["ip_addr"]).setPort(o["port"]).setProtocol(o["protocol"])
            cont.setDHCP(o["cfg_dhcp"]).setStatic(o["cfg_static"])
            controllers.append(cont)
        return controllers

    @controllers.setter
    def controllers (self, val):
        jcl = []
        for c in val:
            if c._cfg_static or c._cfg_dhcp:
                jcl.append(c.toJSON())
        self._data["controllers"] = jcl
            
    @property
    def logging (self):
        if "logging" in self._data:
            return self._data["logging"]
        else:
            return {}

    @logging.setter
    def logging (self, val):
        self._data["logging"] = val

    @property
    def table_miss_action (self):
        return self._data["table_miss_action"]

    @table_miss_action.setter
    def table_miss_action (self, val):
        self._data["table_miss_action"] = val

    @property
    def physical_base_name (self):
        return self._data["physical_base_name"]

    @property
    def lag_base_name (self):
        return self._data["lag_base_name"]

    @property
    def port_list (self):
        if "port_list" in self._data:
            return self._data["port_list"]
        else:
            return {}

    @port_list.setter
    def port_list (self, val):
        self._data["port_list"] = val

    @property
    def needs_update (self):
        stat = os.stat(OFADConfig.PATH)
        if ((stat.st_ctime != self._cache_tinfo[0]) or
            (stat.st_mtime != self._cache_tinfo[1])):
            return True
        else:
            return False
        
    @property
    def _data (self):
        stat = os.stat(OFADConfig.PATH)
        if self.needs_update:
            self._rebuild_cache()
            if self._warn_on_changed:
                raise ConfigChangedError(self._warn_on_changed)
        return self._data_cache

    def update (self):
        if self.needs_update:
            self._rebuild_cache()

    def _rebuild_cache (self):
        stat = os.stat(OFADConfig.PATH)
        self._cache_tinfo = (stat.st_ctime, stat.st_mtime)

        old = self._data_cache
        with open(OFADConfig.PATH, "r") as cfg:
            self._data_cache = json.load(cfg)
            self._data_cache['of_datapath_id'] = \
                DPID(int(self._data_cache['of_datapath_id'], 16))

        if self._data_cache['version'] != OFADConfig.VERSION:
            if self._data_cache['version'] == 1:
                self._doConvertV1()
            else:
                self._data_cache = old
                raise UnknownVersionError(self._data_cache['version'])

    def _doConvertV1 (self):
        for o in self._data_cache["controllers"]:
            o["cfg_static"] = True
            o["cfg_dhcp"] = False
        self._data_cache['version'] = 2

    def write (self, warn = False):
        # FIXME: Should lock the file before we start
        with self.warnOnChanged(warn):
            data = copy.copy(self._data)
            with open(OFADConfig.PATH, 'w+') as cfg:
                data['of_datapath_id'] = data['of_datapath_id'].hexstr()
                json.dump(data, cfg, sort_keys=True, indent=4, separators=(',', ': '))
            self._rebuild_cache()

    def reload (self):
        try:
            shell.call('service ofad reload')
        except subprocess.CalledProcessError:
            raise error.ActionError('Cannot reload openflow agent configuration')
