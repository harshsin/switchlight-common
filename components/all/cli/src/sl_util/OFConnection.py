# Copyright (c) 2013  BigSwitch Networks

import error
import loxi
import loxi.of10 as ofp10
import loxi.of13 as ofp13
import socket
import select
import struct
import errno


class OFConnection(object):
    timeout = 10
    next_xid = 1

    def __init__(self, host, port):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        self.sock.setblocking(0)
        rv = self.sock.connect_ex((host, port))
        if rv not in [0, errno.EISCONN]:
            read_ready, write_ready, err = \
                select.select([self.sock], [], [self.sock], self.timeout)
            if not read_ready:
                raise error.ActionError('Timeout connecting to openflow agent')
            if err:
                raise error.ActionError('Error connecting to openflow agent')
            rv = self.sock.connect_ex((host, port))
            if rv not in [0, errno.EISCONN]:
                raise error.ActionError('%s connecting to openflow agent' % 
                                        errno.errorcode[rv])

        self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, True)
        self.sendmsg(ofp10.message.hello())
        hello = self.recvmsg()
        assert(isinstance(hello, ofp10.message.hello))
        assert(hello.version == ofp10.OFP_VERSION)

    def __enter__(self):
        return self

    def __exit__(self, exctype, excval, exctrace):
        self.close()
        return False

    def close(self):
        self.sock.close()

    def sendmsg(self, msg):
        if type(msg) != type(""):
            if msg.xid == None:
                msg.xid = self._gen_xid()
            outpkt = msg.pack()
        else:
            outpkt = msg

        if self.sock.sendall(outpkt) is not None:
            raise error.ActionError('Failed to send message to openflow agent')

    def recvmsg(self, timeout=None):
        _timeout = timeout or self.timeout
        buf = self._read_exactly(4, timeout=_timeout)
        version, _, msg_len = struct.unpack_from("!BBH", buf)
        buf += self._read_exactly(msg_len - 4, timeout=_timeout)
        return loxi.protocol(version).message.parse_message(buf)

    def request_features(self):
        request = ofp10.message.features_request()
        self.sendmsg(request)
        reply = self.recvmsg()
        return reply

    def of10_request_stats(self, request):
        self.sendmsg(request)
        stats = []
        while True:
            reply = self.recvmsg()
            stats.extend(reply.entries)
            if reply.flags & ofp10.OFPSF_REPLY_MORE == 0:
                break
        return stats

    def of10_request_stats_generator(self, request):
        self.sendmsg(request)
        while True:
            reply = self.recvmsg()
            yield reply.entries
            if reply.flags & ofp10.OFPSF_REPLY_MORE == 0:
                break

    def of13_request_stats_generator(self, request):
        self.sendmsg(request)
        while True:
            reply = self.recvmsg()
            yield reply.entries
            if reply.flags & ofp13.OFPSF_REPLY_MORE == 0:
                break

    def of13_port_desc_stats_request(self):
        request = ofp13.message.port_desc_stats_request()
        self.sendmsg(request)
        reply = self.recvmsg()
        return reply

    def _read_exactly(self, n, timeout=None):
        _timeout = timeout or self.timeout
        buf = ""
        while n > 0:
            read_ready, write_ready, err = \
                select.select([self.sock], [], [self.sock], _timeout)
            if not read_ready:
                raise error.ActionError('Timeout reading for openflow agent')
            if err:
                raise error.ActionError('Error reading from openflow agent')
            b = self.sock.recv(n)
            if len(b) == 0:
                raise error.ActionError('Lost connection to openflow agent')
            n -= len(b)
            buf += b
        return buf

    def _gen_xid(self):
        v = self.next_xid
        self.next_xid = self.next_xid + 1
        return v

