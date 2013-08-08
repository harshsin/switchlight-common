#!/usr/bin/env python
# PYTHONPATH should include BigCode/Modules/pyloxi
import loxi.of10 as ofp
import socket
import struct
import fmtcnv

# to run against LRI:
# controller add LISTEN 127.0.0.1:6634

class OFConnection(object):
    next_xid = 1

    def __init__(self, host, port):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((host, port))
        self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, True)
        self.sendmsg(ofp.message.hello())
        hello = self.recvmsg()
        assert(isinstance(hello, ofp.message.hello))
        assert(hello.version == ofp.OFP_VERSION)

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
            raise AssertionError("failed to send message to switch")

    def recvmsg(self):
        buf = self._read_exactly(4)
        _, _, msg_len = struct.unpack_from("!BBH", buf)
        buf += self._read_exactly(msg_len - 4)
        return ofp.message.parse_message(buf)

    def request_stats(self, request):
        self.sendmsg(request)
        stats = []
        while True:
            reply = self.recvmsg()
            stats.extend(reply.entries)
            if reply.flags & 1 == 0:
                break
        return stats

    def _read_exactly(self, n):
        buf = ""
        while n > 0:
            b = self.sock.recv(n)
            n -= len(b)
            buf += b
        return buf

    def _gen_xid(self):
        v = self.next_xid
        self.next_xid = self.next_xid + 1
        return v


def get_10_match_str(match):
    outarr = []
    if not (match.wildcards & ofp.OFPFW_IN_PORT):
        outarr.append("in_port " + str(match.in_port))
    # FIXME vlan interpretation
    if not (match.wildcards & ofp.OFPFW_DL_VLAN):
        outarr.append("dl_vlan " + str(match.vlan_vid))
    if not (match.wildcards & ofp.OFPFW_DL_VLAN_PCP):
        outarr.append("dl_vlan_pcp " + str(match.vlan_pcp))
    if not (match.wildcards & ofp.OFPFW_DL_TYPE):
        outarr.append("dl_type " + str(hex(match.eth_type)))
    if not (match.wildcards & ofp.OFPFW_DL_SRC):
        outarr.append("dl_src " + 
               fmtcnv.convert_mac_in_byte_array_to_hex_string(match.eth_src))
    if not (match.wildcards & ofp.OFPFW_DL_DST):
        outarr.append("dl_dst " + 
               fmtcnv.convert_mac_in_byte_array_to_hex_string(match.eth_dst))
    # FIXME mask or shift?
    if not (match.wildcards & ofp.OFPFW_NW_TOS):
        outarr.append("nw_tos " + str(match.ip_dscp))
    if not (match.wildcards & ofp.OFPFW_NW_PROTO):
        outarr.append("nw_proto " + str(match.ip_proto))
    if not (match.wildcards & ofp.OFPFW_NW_SRC_MASK):
        bits = 32-((match.wildcards & ofp.OFPFW_NW_SRC_MASK) >> ofp.OFPFW_NW_SRC_SHIFT)
        outarr.append("nw_src " + 
               fmtcnv.convert_ip_in_integer_to_dotted_decimal(match.ipv4_src) +
                      "/" + str(bits))
    if not (match.wildcards & ofp.OFPFW_NW_DST_MASK):
        bits = 32-((match.wildcards & ofp.OFPFW_NW_SRC_MASK) >> ofp.OFPFW_NW_SRC_SHIFT)
        outarr.append("nw_dst " +
               fmtcnv.convert_ip_in_integer_to_dotted_decimal(match.ipv4_dst) +
                      "/" + str(bits))
    if not (match.wildcards & ofp.OFPFW_TP_SRC):
        outarr.append("tp_src " + str(match.tcp_src))
    if not (match.wildcards & ofp.OFPFW_TP_DST):
        outarr.append("tp_dst " + str(match.tcp_dst))
    return ",".join(outarr)

# FIXME should use obj.type to print action-specific parameters 
def get_10_actions_str(actions):
    outarr = []
    for obj in actions:
        outarr.append(type(obj).__name__)
    return ",".join(outarr)

def get_10_flow_entry_str(entry):
    """
    User-friendly show command  in place of flow_stats_entry show() method.
    """
    outarr = []

    # TODO print table_id?

    outarr.append(get_10_match_str(entry.match))
    outarr.append(get_10_actions_str(entry.actions))

    # FIXME print duration
    outarr.extend(["prio " + str(entry.priority),
                   "idle " + str(entry.idle_timeout),
                   "hard " + str(entry.hard_timeout),
                   "cookie " + str(hex(entry.cookie)),
                   "pkts " + str(entry.packet_count),
                   "bytes " + str(entry.byte_count)])

    return ",".join(outarr)


if __name__ == "__main__":
    conn = OFConnection('127.0.0.1', 6634)

    """
    print "Table stats:"
    for x in conn.request_stats(ofp.message.table_stats_request()):
        print x.show()
    """

    print "Flow stats:"
    req = ofp.message.flow_stats_request()
    req.match.wildcards = ofp.OFPFW_ALL
    req.table_id = 0xff
    req.out_port = ofp.OFPP_NONE
    for x in conn.request_stats(req):
        #print x.show()
        print get_10_flow_entry_str(x)

    """
    print "Port stats:"
    for x in conn.request_stats(ofp.message.port_stats_request(port_no=ofp.OFPP_ALL)):
        print x.show()
    """

    conn.close()
