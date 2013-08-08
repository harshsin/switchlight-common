# Copyright (c) 2013  BigSwitch Networks

import command
import error

import subprocess
from PandOS import shell
from PandOS import OFConnection

import loxi.of10 as ofp
import fmtcnv


def display(val):
    return str(val) if val != 0xffffffffffffffff else '---'


class disp_flow_ob(object):
    """
    Defines the attributes of a printable flow
    """
    inport = "-"
    dmac = "-"
    smac = "-"
    prio = "-"
    dip = "-"
    sip = "-"
    output = "-"
    modifications = "-"
    packets = "-"
    bytes = "-"
    eth_type = "-"
    duration = "-"
    l4_src = "-"
    l4_dst = "-"
    def __init__(self,
                 inport=inport,
                 dmac=dmac,
                 smac=smac,
                 prio=prio,
                 dip=dip,
                 sip=sip,
                 output=output,
                 modifications=modifications,
                 packets=packets,
                 bytes=bytes,
                 eth_type=eth_type,
                 duration=duration,
                 l4_src=l4_src,
                 l4_dst=l4_dst):
        self.inport = inport
        self.dmac = dmac
        self.smac = smac
        self.prio = prio
        self.dip = dip
        self.sip = sip
        self.output = output
        self.modifications = modifications
        self.packets = packets
        self.bytes = bytes
        self.eth_type = eth_type
        self.duration = duration
        self.l4_src = l4_src
        self.l4_dst = l4_dst

flow_table_titles = disp_flow_ob(
    inport="In-Port",
    dmac="Dest Mac",
    smac="Src Mac",
    prio="Priority",
    dip="Dest IP",
    sip="Src IP",
    output="Output",
    modifications="Modifications",
    packets="Packets",
    bytes="Bytes",
    eth_type="Eth Type",
    duration="Duration",
    l4_src="L4 Src",
    l4_dst="L4 Dst")

def get_format_string(disp_config):
    """
    Get the format string based on the current config
    For now, just return a static string.  But we can expand to do
    this based on configuration
    """
    return "{0.prio:<8} {0.inport:<7} {0.dmac:<18} {0.smac:<18} {0.dip:<18} {0.sip:<18} {0.output:<16} {0.packets:<10} {0.duration:<10}"

def flow_entry_to_disp(entry):
    """
    Convert a flow entry to a display object
    """
    rv = disp_flow_ob()
    match = entry.match
    if not (match.wildcards & ofp.OFPFW_IN_PORT):
        rv.inport = str(match.in_port)
    # FIXME VLAN
    if not (match.wildcards & ofp.OFPFW_DL_VLAN):
        # str(match.vlan_vid))
        pass
    if not (match.wildcards & ofp.OFPFW_DL_VLAN_PCP):
        # str(match.vlan_pcp))
        pass
    if not (match.wildcards & ofp.OFPFW_DL_TYPE):
        rv.eth_type = str(hex(match.eth_type))
    if not (match.wildcards & ofp.OFPFW_DL_SRC):
        rv.smac = fmtcnv.convert_mac_in_byte_array_to_hex_string(match.eth_src)
    if not (match.wildcards & ofp.OFPFW_DL_DST):
        rv.dmac = fmtcnv.convert_mac_in_byte_array_to_hex_string(match.eth_dst)
    # FIXME mask or shift?
    if not (match.wildcards & ofp.OFPFW_NW_TOS):
        # str(match.ip_dscp)
        pass
    if not (match.wildcards & ofp.OFPFW_NW_PROTO):
        # str(match.ip_proto)
        pass
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
    rv.duration = str(entry.duration_sec)
    rv.packets = str(entry.packet_count)

    return rv


def show_flowtable(data):
    conn = OFConnection.OFConnection('127.0.0.1', 6634)

    if 'summary' not in data:
        req = ofp.message.flow_stats_request()
        req.match.wildcards = ofp.OFPFW_ALL
        req.table_id = 0xff
        req.out_port = ofp.OFPP_NONE
        count = 0

        format_str = get_format_string(None)
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

# FIXME specify table id?
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
            'token'      : 'summary',
            'optional'   : True,
            'short-help' : 'Show the flow table summary',
            'data'       : { 'summary' : True },
            'doc'        : 'flowtable|show-summary',
            'doc-example' : 'flowtable|show-summary-example',
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


