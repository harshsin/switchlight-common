# Copyright (c) 2013  BigSwitch Networks

import command
import error

import subprocess
from sl_util import shell
from sl_util import OFConnection

import loxi.of10 as of10
import loxi.of13 as of13
import fmtcnv


# FIXME move this to a common place
max_port = 52
max_table = 1

def display(val):
    return str(val) if val != 0xffffffffffffffff else '-'

def convert_mac_hex_string_to_byte_array(mac):
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
        return "{0.prio:<5} {0.table_id:<1} {0.inport:<3} {0.dmac:<17} {0.smac:<17} {0.eth_type:<6} {0.vid:<5} {0.vpcp:<3} {0.dip:<40} {0.sip:<40} {0.ipproto:<4} {0.ipdscp:<4} {0.l4_dst:<6} {0.l4_src:<6} {0.output:<16} {0.packets:<10} {0.hard_to:<6} {0.idle_to:<6} {0.duration:<10}"
    else:
        return "{0.prio:<5} {0.inport:<3} {0.dmac:<17} {0.smac:<17} {0.dip:<40} {0.sip:<40} {0.output:<16} {0.packets:<10} {0.duration:<10}"

def of10_flow_entry_to_disp(entry):
    """
    Convert a 1.0 flow entry to a display object
    """
    rv = disp_flow_ob()
    match = entry.match
    if not (match.wildcards & of10.OFPFW_IN_PORT):
        rv.inport = str(match.in_port)
    if not (match.wildcards & of10.OFPFW_DL_VLAN):
        rv.vid = str(match.vlan_vid)
    if not (match.wildcards & of10.OFPFW_DL_VLAN_PCP):
        rv.vpcp = str(match.vlan_pcp)
    if not (match.wildcards & of10.OFPFW_DL_TYPE):
        rv.eth_type = str(hex(match.eth_type))
    if not (match.wildcards & of10.OFPFW_DL_SRC):
        rv.smac = fmtcnv.convert_mac_in_byte_array_to_hex_string(match.eth_src)
    if not (match.wildcards & of10.OFPFW_DL_DST):
        rv.dmac = fmtcnv.convert_mac_in_byte_array_to_hex_string(match.eth_dst)
    # FIXME mask or shift?
    if not (match.wildcards & of10.OFPFW_NW_TOS):
        rv.ipdscp = str(match.ip_dscp)
    if not (match.wildcards & of10.OFPFW_NW_PROTO):
        rv.ipproto = str(match.ip_proto)
    if not (match.wildcards & of10.OFPFW_NW_SRC_MASK):
        bits = 32-((match.wildcards & of10.OFPFW_NW_SRC_MASK) >> of10.OFPFW_NW_SRC_SHIFT)
        rv.sip = fmtcnv.convert_ip_in_integer_to_dotted_decimal(match.ipv4_src) + "/" + str(bits)
    if not (match.wildcards & of10.OFPFW_NW_DST_MASK):
        bits = 32-((match.wildcards & of10.OFPFW_NW_DST_MASK) >> of10.OFPFW_NW_DST_SHIFT)
        rv.dip = fmtcnv.convert_ip_in_integer_to_dotted_decimal(match.ipv4_dst) + "/" + str(bits)
    if not (match.wildcards & of10.OFPFW_TP_SRC):
        rv.l4_src = str(match.tcp_src)
    if not (match.wildcards & of10.OFPFW_TP_DST):
        rv.l4_dst = str(match.tcp_dst)

    output_count = 0
    for obj in entry.actions:
        if (obj.type == of10.OFPAT_OUTPUT or obj.type == of10.OFPAT_ENQUEUE):
            output_count += 1
            if output_count < 3:
                if rv.output == "-":
                    rv.output = str(obj.port)
                else:
                    rv.output += " " + str(obj.port)
        elif (obj.type >= of10.OFPAT_SET_VLAN_VID and
              obj.type <= of10.OFPAT_SET_TP_DST):
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

def of13_flow_entry_to_disp(entry):
    """
    Convert a 1.3 flow entry to a display object
    """
    mapper = {
        str(of13.oxm.eth_src):
            ('smac', fmtcnv.convert_mac_in_byte_array_to_hex_string),
        str(of13.oxm.eth_src_masked):
            ('smac', fmtcnv.convert_mac_in_byte_array_to_hex_string),
        str(of13.oxm.eth_dst):
            ('dmac', fmtcnv.convert_mac_in_byte_array_to_hex_string),
        str(of13.oxm.eth_dst_masked):
            ('dmac', fmtcnv.convert_mac_in_byte_array_to_hex_string),
        str(of13.oxm.eth_type): ('eth_type', None),
        str(of13.oxm.eth_type_masked): ('eth_type', None),
        str(of13.oxm.vlan_vid): ('vid', None),
        str(of13.oxm.vlan_vid_masked): ('vid', None),
        str(of13.oxm.vlan_pcp): ('vpcp', None),
        str(of13.oxm.vlan_pcp_masked): ('vpcp', None),
        str(of13.oxm.ip_dscp): ('ipdscp', None),
        str(of13.oxm.ip_dscp_masked): ('ipdscp', None),
        str(of13.oxm.ip_proto): ('ipproto', None),
        str(of13.oxm.ip_proto_masked): ('ipproto', None),
        str(of13.oxm.ipv4_src):
            ('sip', fmtcnv.convert_ip_in_integer_to_dotted_decimal),
        str(of13.oxm.ipv4_src_masked):
            ('sip', fmtcnv.convert_ip_in_integer_to_dotted_decimal),
        str(of13.oxm.ipv4_dst):
            ('dip', fmtcnv.convert_ip_in_integer_to_dotted_decimal),
        str(of13.oxm.ipv4_dst_masked):
            ('dip', fmtcnv.convert_ip_in_integer_to_dotted_decimal),
        str(of13.oxm.ipv6_src):
            ('sip', fmtcnv.convert_ipv6_in_byte_array_to_hex),
        str(of13.oxm.ipv6_src_masked):
            ('sip', fmtcnv.convert_ipv6_in_byte_array_to_hex),
        str(of13.oxm.ipv6_dst):
            ('dip', fmtcnv.convert_ipv6_in_byte_array_to_hex),
        str(of13.oxm.ipv6_dst_masked):
            ('dip', fmtcnv.convert_ipv6_in_byte_array_to_hex),
        str(of13.oxm.tcp_src): ('l4_src', None),
        str(of13.oxm.tcp_src_masked): ('l4_src', None),
        str(of13.oxm.tcp_dst): ('l4_dst', None),
        str(of13.oxm.tcp_dst_masked): ('l4_dst', None),
        str(of13.oxm.udp_src): ('l4_src', None),
        str(of13.oxm.udp_src_masked): ('l4_src', None),
        str(of13.oxm.udp_dst): ('l4_dst', None),
        str(of13.oxm.udp_dst_masked): ('l4_dst', None),
        }

    rv = disp_flow_ob()
    rv_mask = None
    inport = None
    bsn_inports_mask = None
    for field in entry.match.oxm_list:
        if isinstance(field, of13.oxm.in_port):
            inport = field
            continue
        if isinstance(field, of13.oxm.bsn_in_ports_128_masked):
            bsn_inports_mask = field
            continue
        if isinstance(field, of13.oxm.in_port_masked) or \
           isinstance(field, of13.oxm.bsn_in_ports_128):
            # NOTE: these should really never be used or set
            continue

        mapval = mapper.get(str(type(field)), None)
        if mapval:
            (name, formatter) = mapval
            # masked values are returned as hex, nonmasked as decimal
            if 'masked' in str(type(field)):
                if rv_mask is None:
                    rv_mask = disp_flow_ob()
                rv_mask.__setattr__(name,
                                    formatter(field.value_mask) if formatter \
                                        else hex(field.value_mask))
                rv.__setattr__(name, 
                               formatter(field.value) if formatter \
                                   else hex(field.value))
            else:
                rv.__setattr__(name, 
                               formatter(field.value) if formatter \
                                   else str(field.value))

    rv_inports = None
    if inport:
        rv.inport = str(inport.value)
    elif bsn_inports_mask:
        # NOTE: We only care about the zero bits in the mask.
        #       We should never get 0 ports.
        ports = list(set(range(128)) - bsn_inports_mask.value_mask)
        if len(ports) == 1:
            rv.inport = str(ports[0])
        elif len(ports) > 1:
            rv.inport = "*"
            rv_inports = sorted(ports)

    output_count = 0
    for inst in entry.instructions:
        if inst.type == of13.OFPIT_APPLY_ACTIONS:
            for act in inst.actions:
                if (act.type == of13.OFPAT_OUTPUT or \
                        act.type == of13.OFPAT_ENQUEUE):
                    output_count += 1
                    if output_count < 3:
                        if rv.output == "-":
                            rv.output = str(act.port)
                        else:
                            rv.output += " " + str(act.port)
                elif (act.type >= of13.OFPAT_SET_VLAN_VID and \
                          act.type <= of13.OFPAT_SET_TP_DST):
                    rv.modifications = "yes"

    if output_count >= 3:
        rv.output += " + %d more" % (output_count - 2)

    rv.prio = str(entry.priority)
    rv.table_id = str(entry.table_id)
    rv.duration = display(entry.duration_sec)
    rv.hard_to = str(entry.hard_timeout)
    rv.idle_to = str(entry.idle_timeout)
    rv.packets = display(entry.packet_count)
    return (rv, rv_mask, rv_inports)

class FlowRequestFilter(object):
    def __init__(self,
                 in_port=None,
                 src_mac=None,
                 dst_mac=None,
                 vlan_id=None,
                 table_id=None,
                 out_port=None):
        self.in_port = in_port
        self.src_mac = src_mac
        self.dst_mac = dst_mac
        self.vlan_id = vlan_id
        self.table_id = table_id
        self.out_port = out_port

def show_of10_entries(rf, format_str):
    req = of10.message.flow_stats_request()
    req.match.wildcards = of10.OFPFW_ALL

    if rf.in_port is not None:
        req.match.in_port = rf.in_port
        req.match.wildcards = req.match.wildcards & ~of10.OFPFW_IN_PORT

    if rf.src_mac is not None:
        req.match.eth_src = rf.src_mac
        req.match.wildcards = req.match.wildcards & ~of10.OFPFW_DL_SRC

    if rf.dst_mac is not None:
        req.match.eth_dst = rf.dst_mac
        req.match.wildcards = req.match.wildcards & ~of10.OFPFW_DL_DST

    if rf.vlan_id is not None:
        req.match.vlan_vid = rf.vlan_id
        req.match.wildcards = req.match.wildcards & ~of10.OFPFW_DL_VLAN

    req.table_id = rf.table_id if rf.table_id is not None else 0xff
    req.out_port = rf.out_port if rf.out_port is not None else of10.OFPP_NONE

    count = 0
    with OFConnection.OFConnection('127.0.0.1', 6634) as conn:
        for entrylist in conn.of10_request_stats_generator(req):
            for entry in entrylist:
                count += 1
                if count % 20 == 0 or count == 1:
                    if count != 1:
                        print
                    print format_str.format(flow_table_titles)
                entry_disp = of10_flow_entry_to_disp(entry)
                print format_str.format(entry_disp)

def show_of13_entries(rf, format_str):
    match = of13.match()
    match2 = None

    if rf.in_port is not None:
        match.oxm_list.append(of13.oxm.in_port(value=rf.in_port))

        # If in_port is specified as a filter, we need to make an additional
        # query using oxm.bsn_in_ports.
        match2 = of13.match()
        match2.oxm_list.append(
            of13.oxm.bsn_in_ports_128_masked(
                value=set(),
                value_mask=set(range(128)) - set([rf.in_port])))

    if rf.src_mac is not None:
        match.oxm_list.append(of13.oxm.eth_src(value=rf.src_mac))

    if rf.dst_mac is not None:
        match.oxm_list.append(of13.oxm.eth_dst(value=rf.dst_mac))

    if rf.vlan_id is not None:
        match.oxm_list.append(of13.oxm.vlan_vid(value=rf.vlan_id))

    def get_and_display_flows(match_obj):
        req = of13.message.flow_stats_request(
            match=match_obj,
            table_id=rf.table_id if rf.table_id is not None else of13.OFPTT_ALL,
            out_port=rf.out_port if rf.out_port is not None else of13.OFPP_ANY,
            out_group=of13.OFPG_ANY)

        count = 0
        with OFConnection.OFConnection('127.0.0.1', 6634) as conn:
            for entrylist in conn.of13_request_stats_generator(req):
                for entry in entrylist:
                    count += 1
                    if count % 20 == 0 or count == 1:
                        if count != 1:
                            print
                        print format_str.format(flow_table_titles)
                    (display_val, display_mask, inports) = of13_flow_entry_to_disp(entry)
                    print format_str.format(display_val)
                    if display_mask:
                        print format_str.format(display_mask)
                    if inports:
                        print "  *In-ports: %s" % " ".join([str(p) for p in inports])

    get_and_display_flows(match)
    if match2 is not None:
        get_and_display_flows(match2)

def show_flowtable(data):
    if 'summary' not in data:
        rf = FlowRequestFilter(
            in_port=data.get('in-port', None),
            src_mac=convert_mac_hex_string_to_byte_array(data['src-mac']) if 'src-mac' in data else None,
            dst_mac=convert_mac_hex_string_to_byte_array(data['dst-mac']) if 'dst-mac' in data else None,
            vlan_id=int(data['vlan-id']) if 'vlan-id' in data else None,
            table_id=data.get('table-id', None),
            out_port=data.get('out-port', None))

        cfg = 'detail' if 'detail' in data else None
        format_str = get_format_string(cfg)

        show_of10_entries(rf, format_str)
        show_of13_entries(rf, format_str)
    else:
        with OFConnection.OFConnection('127.0.0.1', 6634) as conn:
            for x in conn.of10_request_stats(
                    of10.message.table_stats_request()):
                format_str = '%-16s  %s'
                print format_str % ('table id', str(x.table_id))
                print format_str % ('table name', x.name)
                print format_str % ('max entries', str(x.max_entries))
                print format_str % ('active entries', str(x.active_count))
                print format_str % ('lookup count', display(x.lookup_count))
                print format_str % ('matched count', display(x.matched_count))


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


