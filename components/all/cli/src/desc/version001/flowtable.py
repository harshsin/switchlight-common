# Copyright (c) 2013  BigSwitch Networks

import command
import error

import subprocess
from sl_util import shell
from sl_util import OFConnection

import loxi.of10 as ofp
import fmtcnv


# FIXME move this to a common place
max_port = 52
max_table = 1


def display(val):
    return str(val) if val != 0xffffffffffffffff else '-'

def convert_mac_in_hex_string_to_byte_array(mac):
    return [ int(x,16) for x in mac.split(':') ]


class disp_flow_ob(object):
    """
    Defines the attributes of a printable flow
    """
    prio = "-"
    table_id = "-"
    inport = "-"
    dmac = "-"
    smac = "-"
    vid = "-"
    vpcp = "-"
    dip = "-"
    sip = "-"
    ipproto = "-"
    ipdscp = "-"
    output = "-"
    modifications = "-"
    packets = "-"
    bytes = "-"
    eth_type = "-"
    duration = "-"
    hard_to = "-"
    idle_to = "-"
    l4_src = "-"
    l4_dst = "-"
    def __init__(self,
                 prio=prio,
                 table_id=table_id,
                 inport=inport,
                 dmac=dmac,
                 smac=smac,
                 vid=vid,
                 vpcp=vpcp,
                 dip=dip,
                 sip=sip,
                 ipproto=ipproto,
                 ipdscp=ipdscp,
                 output=output,
                 modifications=modifications,
                 packets=packets,
                 bytes=bytes,
                 eth_type=eth_type,
                 duration=duration,
                 hard_to=hard_to,
                 idle_to=idle_to,
                 l4_src=l4_src,
                 l4_dst=l4_dst):
        self.prio = prio
        self.table_id = table_id
        self.inport = inport
        self.dmac = dmac
        self.smac = smac
        self.vid = vid
        self.vpcp = vpcp
        self.dip = dip
        self.sip = sip
        self.ipproto = ipproto
        self.ipdscp = ipdscp
        self.output = output
        self.modifications = modifications
        self.packets = packets
        self.bytes = bytes
        self.eth_type = eth_type
        self.duration = duration
        self.hard_to = hard_to
        self.idle_to = idle_to
        self.l4_src = l4_src
        self.l4_dst = l4_dst

flow_table_titles = disp_flow_ob(
    prio="Prio",
    table_id="T",
    inport="InP",
    dmac="Dest Mac",
    smac="Src Mac",
    vid="VLAN",
    vpcp="VP",
    dip="DstIP",
    sip="SrcIP",
    ipproto="Prot",
    ipdscp="DSCP",
    output="Output",
    modifications="Modifications",
    packets="Packets",
    bytes="Bytes",
    eth_type="EType",
    duration="Duration",
    hard_to="HardTO",
    idle_to="IdleTO",
    l4_src="L4Src",
    l4_dst="L4Dst")

def get_format_string(disp_config):
    """
    Get the format string based on the current config
    For now, just return a static string.  But we can expand to do
    this based on configuration
    """
    if disp_config == 'detail':
        return "{0.prio:<5} {0.table_id:<1} {0.inport:<3} {0.dmac:<17} {0.smac:<17} {0.eth_type:<5} {0.vid:<5} {0.vpcp:<2} {0.dip:<15} {0.sip:<15} {0.ipproto:<4} {0.ipdscp:<4} {0.l4_dst:<5} {0.l4_src:<5} {0.output:<16} {0.packets:<10} {0.hard_to:<6} {0.idle_to:<6} {0.duration:<10}"
    else:
        return "{0.prio:<5} {0.inport:<3} {0.dmac:<17} {0.smac:<17} {0.dip:<15} {0.sip:<15} {0.output:<16} {0.packets:<10} {0.duration:<10}"

def flow_entry_to_disp(entry):
    """
    Convert a flow entry to a display object
    """
    rv = disp_flow_ob()
    match = entry.match
    if not (match.wildcards & ofp.OFPFW_IN_PORT):
        rv.inport = str(match.in_port)
    if not (match.wildcards & ofp.OFPFW_DL_VLAN):
        rv.vid = str(match.vlan_vid)
    if not (match.wildcards & ofp.OFPFW_DL_VLAN_PCP):
        rv.vpcp = str(match.vlan_pcp)
    if not (match.wildcards & ofp.OFPFW_DL_TYPE):
        rv.eth_type = str(hex(match.eth_type))
    if not (match.wildcards & ofp.OFPFW_DL_SRC):
        rv.smac = fmtcnv.convert_mac_in_byte_array_to_hex_string(match.eth_src)
    if not (match.wildcards & ofp.OFPFW_DL_DST):
        rv.dmac = fmtcnv.convert_mac_in_byte_array_to_hex_string(match.eth_dst)
    # FIXME mask or shift?
    if not (match.wildcards & ofp.OFPFW_NW_TOS):
        rv.ipdscp = str(match.ip_dscp)
    if not (match.wildcards & ofp.OFPFW_NW_PROTO):
        rv.ipproto = str(match.ip_proto)
    if not (match.wildcards & ofp.OFPFW_NW_SRC_MASK):
        bits = 32-((match.wildcards & ofp.OFPFW_NW_SRC_MASK) >> ofp.OFPFW_NW_SRC_SHIFT)
        rv.sip = fmtcnv.convert_ip_in_integer_to_dotted_decimal(match.ipv4_src) + "/" + str(bits)
    if not (match.wildcards & ofp.OFPFW_NW_DST_MASK):
        bits = 32-((match.wildcards & ofp.OFPFW_NW_SRC_MASK) >> ofp.OFPFW_NW_SRC_SHIFT)
        rv.dip = fmtcnv.convert_ip_in_integer_to_dotted_decimal(match.ipv4_dst) + "/" + str(bits)
    if not (match.wildcards & ofp.OFPFW_TP_SRC):
        rv.l4_src = str(match.tcp_src)
    if not (match.wildcards & ofp.OFPFW_TP_DST):
        rv.l4_dst = str(match.tcp_dst)

    output_count = 0
    for obj in entry.actions:
        if (obj.type == ofp.OFPAT_OUTPUT or obj.type == ofp.OFPAT_ENQUEUE):
            output_count += 1
            if output_count < 3:
                if rv.output == "-":
                    rv.output = str(obj.port)
                else:
                    rv.output += " " + str(obj.port)
        elif (obj.type >= ofp.OFPAT_SET_VLAN_VID and 
              obj.type <= ofp.OFPAT_SET_TP_DST):
            rv.modifications = "yes"

    if output_count >= 3:
        rv.output += " + %d more" % (output_count - 2)

    rv.prio = str(entry.priority)
    rv.table_id = str(entry.table_id)
    rv.duration = display(entry.duration_sec)
    rv.hard_to = str(entry.hard_timeout)
    rv.idle_to = str(entry.idle_timeout)
    rv.packets = display(entry.packet_count)

    return rv


def show_flowtable(data):
    conn = OFConnection.OFConnection('127.0.0.1', 6634)

    if 'summary' not in data:
        req = ofp.message.flow_stats_request()
        req.match.wildcards = ofp.OFPFW_ALL
        #print 'starting wildcards ' + str(hex(req.match.wildcards))
        if 'in-port' in data:
            req.match.in_port = data['in-port']
            req.match.wildcards = req.match.wildcards & ~ofp.OFPFW_IN_PORT
        if 'src-mac' in data:
            req.match.eth_src = \
                convert_mac_in_hex_string_to_byte_array(data['src-mac'])
            req.match.wildcards = req.match.wildcards & ~ofp.OFPFW_DL_SRC
        if 'dst-mac' in data:
            req.match.eth_dst = \
                convert_mac_in_hex_string_to_byte_array(data['dst-mac'])
            req.match.wildcards = req.match.wildcards & ~ofp.OFPFW_DL_DST
        if 'vlan-id' in data:
            # hex-to-integer returns a string?!
            req.match.vlan_vid = int(data['vlan-id'])
            req.match.wildcards = req.match.wildcards & ~ofp.OFPFW_DL_VLAN
        #print 'ending wildcards ' + str(hex(req.match.wildcards))
        req.table_id = data['table-id'] if 'table-id' in data else 0xff
        req.out_port = data['out-port'] if 'out-port' in data else ofp.OFPP_NONE
        count = 0

        cfg = 'detail' if 'detail' in data else None
        format_str = get_format_string(cfg)

        print format_str.format(flow_table_titles)
        for x in conn.request_stats(req):
            count += 1
            if count % 20 == 0:
                print
                print format_str.format(flow_table_titles)
            entry_disp = flow_entry_to_disp(x)
            print format_str.format(entry_disp)
    else:
        for x in conn.request_stats(ofp.message.table_stats_request()):
            format_str = '%-16s  %s'
            print format_str % ('table id', str(x.table_id))
            print format_str % ('table name', x.name)
            print format_str % ('max entries', str(x.max_entries))
            print format_str % ('active entries', str(x.active_count))
            print format_str % ('lookup count', display(x.lookup_count))
            print format_str % ('matched count', display(x.matched_count))

    conn.close()

command.add_action('implement-show-flowtable', show_flowtable,
                    {'kwargs': {'data'      : '$data',}})

TABLE_ID = {
    'field'        : 'table-id',
    'tag'          : 'table-id',
    'short-help'   : 'Filter on table identifier',
    'base-type'    : 'integer',
    'range'        : (0, max_table),
    'optional'     : True,
    'doc'          : 'flowtable|table-id',
}

IN_PORT = {
    'field'        : 'in-port',
    'tag'          : 'in-port',
    'short-help'   : 'Filter on input port',
    'base-type'    : 'integer',
    'range'        : (1, max_port),
    'optional'     : True,
    'doc'          : 'flowtable|in-port',
}

OUT_PORT = {
    'field'        : 'out-port',
    'tag'          : 'out-port',
    'short-help'   : 'Filter on output port',
    'base-type'    : 'integer',
    'range'        : (1, max_port),
    'optional'     : True,
    'doc'          : 'flowtable|out-port',
}

SRC_MAC = {
    'field'        : 'src-mac',
    'tag'          : 'src-mac',
    'short-help'   : 'Filter on source MAC',
    'type'         : 'mac-address',
    'optional'     : True,
    'doc'          : 'flowtable|src-mac',
}

DST_MAC = {
    'field'        : 'dst-mac',
    'tag'          : 'dst-mac',
    'short-help'   : 'Filter on destination MAC',
    'type'         : 'mac-address',
    'optional'     : True,
    'doc'          : 'flowtable|dst-mac',
}

VLAN_ID = {
    'field'        : 'vlan-id',
    'tag'          : 'vlan-id',
    'short-help'   : 'Filter on VLAN identifier',
    'base-type'    : 'hex-or-decimal-integer',
    'range'        : (0, 4095),
    'optional'     : True,
    'data-handler' : 'hex-to-integer',
    'doc'          : 'flowtable|vlan-id',
}

SHOW_FLOWTABLE_COMMAND_DESCRIPTION = {
    'name'         : 'show',
    'mode'         : 'login',
    'action'       : 'implement-show-flowtable',
    'no-supported' : False,
    'args'         : (
        {
            'token'      : 'flowtable',
            'short-help' : 'Show the flow table',
            'doc'        : 'flowtable|show',
            'doc-example' : 'flowtable|show-example',
        },
        {
            'optional'   : True,
            'choices' : (
                {
                    'token'      : 'summary',
                    'short-help' : 'Show the flow table summary',
                    'data'       : { 'summary' : True },
                    'doc'        : 'flowtable|show-summary',
                    'optional'   : True,
                },
                (
                    TABLE_ID,
                    IN_PORT,
                    OUT_PORT,
                    SRC_MAC,
                    DST_MAC,
                    VLAN_ID,
                    {
                        'token'      : 'detail',
                        'short-help' : 'Show detailed flow table',
                        'data'       : { 'detail' : True },
                        'doc'        : 'flowtable|show-detail',
                        'optional'   : True,
                    },
                ),
             ),
        },
    )
}


def clear_flowtable_statistics(data):
    try:
        shell.call('ofad-ctl clear-flow-stats')
    except subprocess.CalledProcessError:
        raise error.ActionError('Error clearing flowtable statistics')

command.add_action('implement-clear-flowtable-statistics',
                   clear_flowtable_statistics,
                    {'kwargs': {'data'      : '$data',}})

CLEAR_FLOWTABLE_COMMAND_DESCRIPTION = {
    'name'         : 'clear',
    'mode'         : 'enable',
    'action'       : 'implement-clear-flowtable-statistics',
    'no-supported' : False,
    'args'         : (
        {
            'token'      : 'flowtable',
            'short-help' : 'Clear flow table parameters',
            'doc'        : 'flowtable|clear',
        },
        {
            'token'      : 'statistics',
            'short-help' : 'Clear flow table statistics',
            'doc'        : 'flowtable|clear-statistics',
        },
    )
}


