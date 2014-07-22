# Copyright (c) 2013-2014 BigSwitch Networks
# All rights reserved.

import command
import run_config

import error
import subprocess
import os
import shutil
import socket
import cfgfile
from datetime import timedelta,datetime

from sl_util import shell, const, conf_state
from sl_util.ofad import OFADConfig, PortManager

from switchlight.platform.current import SwitchLightPlatform
import re
import pytz

import sys

Platform=SwitchLightPlatform()
FW_PRINTENV = '/usr/bin/fw_printenv'

OFAgentConfig = OFADConfig()
PortManager.setPhysicalBase(OFAgentConfig.physical_base_name)
PortManager.setLAGBase(OFAgentConfig.lag_base_name)

def fw_getenv(var):
    try:
        value=subprocess.check_output([FW_PRINTENV, var], stderr=subprocess.STDOUT);
        if value.startswith("%s=" % var):
            value = value[len(var)+1:]
        return value
    except Exception, e:
        pass
    return None

def parse_sl_version(ver):
    try:
        m = re.match(r"(Switch Light OS) (.*?) \((.*?)\)", ver)
        return m.group(1,2,3)
    except Exception, e:
        return (ver, "", "")

def show_version(data):
    out = []

    with open(const.VERSION_PATH, "r") as f:
        sl_version = f.readline().strip()

    if not 'full' in data:
        # Single line version
        out.append(sl_version)
    else:
        # Full version information

        # UBoot version, if available.
        uver=fw_getenv('ver')
        if uver:
            out.append("UBoot Version: %s" % (uver))

        # Platform Information
        out.append(str(Platform))

        # Loader/Installer information
        sli=fw_getenv('sl_installer_version')
        if sli:
            fs_ver = parse_sl_version(sli)
            out.append("Loader Version: %s %s" % (fs_ver[0], fs_ver[1]))
            out.append("Loader Build: %s" % (fs_ver[2]))
        else:
            out.append("Loader Version: Not available on this platform.")
            out.append("Loader Build: Not available on this platform.")
        out.append("")

        fs_ver = parse_sl_version(sl_version);
        out.append("Software Image Version: %s %s" % (fs_ver[0], fs_ver[1]))
        out.append("Internal Build Version: %s " % (fs_ver[2]))
        out.append("")

        # Uptime
        out.append("")
        try:
            with open('/proc/uptime', 'r') as data:
                secs = float(data.readline().split()[0])
                out.append("Uptime is %s" %
                           (str(timedelta(seconds = secs))[:-7]))
        except Exception, e:
            pass

        # Load
        try:
            with open('/proc/loadavg', 'r') as data:
                avgs = data.readline().split()[0:3]
                out.append("Load average:  %s" % " ".join(avgs))
        except:
            pass

        # Memory
        try:
            with open('/proc/meminfo', 'r') as data:
                memtotal = data.readline().split(':')[1].strip()
                memfree = data.readline().split(':')[1].strip()
                out.append("Memory:  %s of %s free" % (memfree, memtotal))
        except:
            pass

    print "\n".join(out)


command.add_action('implement-show-version', show_version,
                    {'kwargs': {'data'      : '$data',}})

SHOW_VERSION_COMMAND_DESCRIPTION = {
    'name'         : 'show',
    'mode'         : 'login',
    'all-help'     : 'Show switch parameters',  # base for all 'show' commands
    'action'       : 'implement-show-version',
    'no-supported' : False,
    'args'         : (
        {
            'token'       : 'version',
            'short-help'  : 'Show switch version information',
            'doc'         : 'core|show-version',
            'data'        : { 'full' : True },
        },
    )
}


def show_environment(data):
    try:
        print Platform.get_environment()
    except:
        print "Environment data is not implemented on this platform."

command.add_action('implement-show-environment', show_environment,
                    {'kwargs': {'data'      : '$data',}})

SHOW_ENVIRONMENT_COMMAND_DESCRIPTION = {
    'name'         : 'show',
    'mode'         : 'login',
    'action'       : 'implement-show-environment',
    'no-supported' : False,
    'args'         : (
        {
            'token'       : 'environment',
            'short-help'  : 'Show the current health of the device',
            'doc'         : 'core|show-environment',
        },
    )
}


def show_users(data):
    try:
        shell.call('who', show_output=True)
    except:
        pass

command.add_action('implement-show-users', show_users,
                    {'kwargs': {'data'      : '$data',}})

SHOW_USERS_COMMAND_DESCRIPTION = {
    'name'         : 'show',
    'mode'         : 'login',
    'action'       : 'implement-show-users',
    'no-supported' : False,
    'args'         : (
        {
            'token'       : 'users',
            'short-help'  : 'Show the users who are currently logged in',
            # FIXME needs doc
            'doc'         : 'core|show-users',
        },
    )
}


def show_log(data):
    try:
        shell.call('cat /var/log/syslog', show_output=True)
    except:
        pass

command.add_action('implement-show-log', show_log,
                    {'kwargs': {'data'      : '$data',}})

SHOW_LOG_COMMAND_DESCRIPTION = {
    'name'         : 'show',
    'mode'         : 'login',
    'action'       : 'implement-show-log',
    'no-supported' : False,
    'args'         : (
        {
            'token'       : 'log',
            'short-help'  : 'Show the log file',
            'doc'         : 'core|show-log',
        },
    )
}


def ping(data):
    if 'count' in data:
        count = data['count']
    else:
        count = 4

    shell.call("ping %s -b -c %d -i0.25" % (data['target'], count),
               show_output = True)

command.add_action('implement-ping', ping,
                    {'kwargs': {'data'      : '$data',}})

PING_COMMAND_DESCRIPTION = {
    'name'         : 'ping',
    'mode'         : 'enable',
    'short-help'   : 'Ping from the management interface',
    'action'       : 'implement-ping',
    'no-supported' : False,
    'doc'          : 'core|ping',
    'doc-example'  : 'core|ping-example',
    'args'         : (
        {
            'field'           : 'target',
            'type'            : 'ip-address-or-domain-name',
            'optional'        : False,
            'syntax-help'     : 'IP address or domain name of the ping target',
            'doc'             : 'core|ping-target',
        },
        {
            'optional'        : True,
            'optional-for-no' : True,
            'args' : (
                {
                    'token'        : 'count',
                    'short-help'   : 'Number of pings to send',
                    'doc'          : 'core|ping-count'
                },
                {
                    'field'        : 'count',
                    'type'         : 'ping-count',
                    'completion-text' : 'Number of pings',
                    'syntax-help'  : 'From 1 to 1800 inclusive',
                    'doc'          : 'core|ping-count'
                },
            ),
        },
    ),
}

def reload(data):
    if 'now' not in data:
        val = raw_input("Proceed with reload [confirm]").lower()
        if val not in ['y', '']:
            print "Aborting reload"
            return
    os.execlp('reboot', 'reboot')

command.add_action('implement-reload', reload,
                    {'kwargs': {'data'      : '$data',}})

RELOAD_COMMAND_DESCRIPTION = {
    'name'         : 'reload',
    'mode'         : 'enable',
    'short-help'   : 'Reload configuration and reboot device',
    'action'       : 'implement-reload',
    'no-supported' : False,
    'doc'          : 'core|reload',
    #'doc-example'  : 'reload-example',
    'args'         : (
        {
            'token'           : 'now',
            'optional'        : True,
            'data'            : { 'now' : True },
            'short-help'      : 'Reboot the device now',
            'doc'             : 'core|reload-now'
        },
    ),
}


def show_running_config(data):
    if 'running-config' in data:
        arg = [ data['running-config'] ]
    else:
        arg = []
    print run_config.implement_show_running_config(arg)

command.add_action('implement-show-running-config', show_running_config,
                    {'kwargs': {'data'      : '$data',}})

SHOW_RUNNING_CONFIG_COMMAND_DESCRIPTION = {
    'name'         : 'show',
    'mode'         : 'login',
    'action'       : 'implement-show-running-config',
    'no-supported' : False,
    'args'         : (
        {
            'token'       : 'running-config',
            'short-help'  : 'Show the current active configuration',
            'doc'         : 'running-config|show',
        },
        run_config.running_config_command_choices,
    )
}


STARTUP_CONFIG_FILE = "/mnt/flash/startup-config"

def show_startup_config(data):
    if os.path.isfile(STARTUP_CONFIG_FILE):
        shell.call('cat %s' % STARTUP_CONFIG_FILE, show_output=True)

command.add_action('implement-show-startup-config', show_startup_config,
                    {'kwargs': {'data'      : '$data',}})

SHOW_STARTUP_CONFIG_COMMAND_DESCRIPTION = {
    'name'         : 'show',
    'mode'         : 'login',
    'action'       : 'implement-show-startup-config',
    'no-supported' : False,
    'args'         : (
        {
            'token'       : 'startup-config',
            'short-help'  : 'Show the saved startup configuration',
            'doc'         : 'core|show-startup-config',
        },
    )
}


def save_config(data):
    cfgstr = run_config.implement_show_running_config([]) + '\n'
    try:
        shell.call('touch ' + STARTUP_CONFIG_FILE)
    except subprocess.CalledProcessError:
        raise error.ActionError('Unable to write startup-config')
    with cfgfile.FileLock(STARTUP_CONFIG_FILE) as f:
        f.truncate(0)
        f.write(cfgstr)

command.add_action('implement-save', save_config,
                    {'kwargs': {'data'      : '$data',}})


def show_tech_support(data):
    print 'Use "enable; copy tech-support flash2" instead.'

command.add_action('implement-show-tech-support', show_tech_support,
                    {'kwargs': {'data'      : '$data',}})

SHOW_TECH_SUPPORT_COMMAND_DESCRIPTION = {
    'name'         : 'show',
    'mode'         : 'login',
    'action'       : 'implement-show-tech-support',
    'no-supported' : False,
    'args'         : (
        {
            'token'       : 'tech-support',
            'short-help'  : 'Show tech support information',
        },
    )
}


# Perform 'copy tech-support' command

tech_support_tmpfile = 'tech-support.tmp'
tech_support_scripts = [
    'date',
    'cat ' + const.VERSION_PATH,
    'cat /mnt/flash/boot-config',
    'cat /mnt/flash/startup-config',
    'cat /var/log/dmesg',
    'cat /etc/network/interfaces',
    'ifconfig ma1',
    'cat ' + const.DNS_CFG_PATH,
    'cat ' + const.NTP_CFG_PATH,
    'cat ' + const.RLOG_CFG_PATH,
    'uptime',
    'cat /proc/meminfo',
    'lsmod',
    'ps auxww',
    'df',
    'sensors',
    'ntpdc -p',
    'cat /etc/rsyslog.conf',
    'cat ' + const.OFAD_CFG_PATH,
    'cat ' + const.SNMP_CFG_PATH,
    'cat /var/log/syslog',
    # Older, compressed logs, too?
    'ofad-ctl controller show CONNECT',
    'ofad-ctl controller show LISTEN',
    'ofad-ctl cxn show-log',
    'ofad-ctl core flowtable',
    'ofad-ctl core dump-flows',
    'ofad-ctl interface show',
    'ofad-ctl port',
    'ofad-ctl show-snmp',
    'ofad-ctl inventory',
    'ofad-ctl portcfg',
    'ofad-ctl crc status',
    'ofad-ctl l2cache status',
    'ofad-ctl pause status',
    'ofad-ctl pimu status',
    'ofad-ctl flow-stats',
    'ofad-ctl stats',
    'ofad-ctl bigwire summary',
    'ofad-ctl bigwire tenant',
    'ofad-ctl bigwire uplink',
    'ofad-ctl bigwire l2',
    'ofad-ctl bigwire qinq',
]

DUMP_FLOWS_PATH = '/usr/local/bin/dump-flows'

def save_tech_support(data):
    tech_support_file = '/mnt/flash2/tech-support_%s.gz' % \
        datetime.now().strftime('%y%m%d%H%M%S')

    portManager = PortManager(OFAgentConfig.port_list)
    scripts = list(tech_support_scripts)
    for lag in portManager.getLAGs():
        scripts.append('ofad-ctl port %s' % lag.portName)

    if os.path.exists(DUMP_FLOWS_PATH):
        scripts.append('%s/dumpt6.py -p 6634' % DUMP_FLOWS_PATH) 
        scripts.append('%s/dumpflows.py -p 6634' % DUMP_FLOWS_PATH) 
        scripts.append('%s/dumpdebugcounters.py -p 6634' % DUMP_FLOWS_PATH) 
        scripts.append('%s/dumpgentables.py -p 6634' % DUMP_FLOWS_PATH) 
        scripts.append('%s/dumpgroups.py -p 6634' % DUMP_FLOWS_PATH) 

    p = subprocess.Popen(args=['/bin/bash'],
                         bufsize=1,
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE
                         )
    p.stdin.write('cd /tmp\n')
    p.stdin.write('rm -f %s*\n' % tech_support_tmpfile)
    for li in scripts:
        p.stdin.write("echo ===== '%s' >>%s\n" % (li, tech_support_tmpfile))
        p.stdin.write('%s &>>%s\n' % (li, tech_support_tmpfile))
    p.stdin.write('gzip -c %s > %s\n' % \
                      (tech_support_tmpfile, tech_support_file))
    p.stdin.write('rm -f %s*\n' % tech_support_tmpfile)
    p.stdin.write('exit\n')
    print 'Writing %s...' % tech_support_file,
    sys.stdout.flush()
    p.wait()
    print 'done.'


command.add_action('implement-tech-support', save_tech_support,
                    {'kwargs': {'data'      : '$data',}})

COPY_COMMAND_DESCRIPTION = {
    'name'         : 'copy',
    'mode'         : 'enable',
    'short-help'   : 'Copy file or configuration',
    'no-supported' : False,
    'doc'          : 'core|copy',
    'doc-example'  : 'core|copy-example',
    'args'         : (
        {
            'choices' : (
                (
                    {
                        'token'           : 'running-config',
                        'optional'        : False,
                        'short-help'      : 'Current configuration',
                        'doc'             : 'core|copy-running-startup',
                    },
                    {
                        'token'           : 'startup-config',
                        'optional'        : False,
                        'short-help'      : 'Save running config',
                        'doc'             : 'core|copy-running-startup',
                        'action'          : 'implement-save',
                    },
                ),
                (
                    {
                        'token'           : 'tech-support',
                        'optional'        : False,
                        'short-help'      : 'Tech support information',
                        'doc'             : 'core|copy-tech-support',
                    },
                    {
                        'token'           : 'flash2',
                        'optional'        : False,
                        'short-help'      : 'CF card',
                        'doc'             : 'core|copy-tech-support',
                        'action'          : 'implement-tech-support',
                    },
                ),
            ),
        },
    ),
}


def delete_file(data):
    if 'startup-config' in data:
        os.remove(STARTUP_CONFIG_FILE)
    else:
        print "STUB: not yet implemented"

command.add_action('implement-delete', delete_file,
                    {'kwargs': {'data'      : '$data',}})

DELETE_COMMAND_DESCRIPTION = {
    'name'         : 'delete',
    'mode'         : 'enable',
    'short-help'   : 'Delete file or configuration',
    'action'       : 'implement-delete',
    'no-supported' : False,
    'doc'          : 'core|delete',
    'doc-example'  : 'core|delete-example',
    'args'         : (
        {
            'token'           : 'startup-config',
            'optional'        : False,
            'data'            : { 'startup-config' : True },
            'short-help'      : 'Saved configuration',
            'doc'             : 'core|delete-startup-config'
        },
    ),
}


VERSION_COMMAND_DESCRIPTION = {
    'name'                : 'version',
    'no-supported'        : False,
    'short-help'          : 'Move to a specific version of command syntax',
    # FIXME
    #'doc'                 : 'core|version',
    #'doc-example'         : 'core|version-example',
    'mode'                : 'config*',
    'action'              : 'version',
    'args': {
        'field'      : 'version',
        'type'       : 'string',
        'completion' : 'description-versions'
    }
}


CLEARTERM_COMMAND_DESCRIPTION = {
    'name'                : 'clearterm',
    'no-supported'        : False,
    'short-help'          : 'Clear and reset the terminal screen',
    # no documentation, other than default
    #'doc'                 : 'clearterm',
    #'doc-example'         : 'clearterm-example',
    'mode'                : 'login',
    'action'              : 'clearterm',
    'args'                : {}
}


def top_command():
    os.system('clear')
    print '\nATTENTION:\n\n' + \
          'You are about run the "top" program.\n' + \
          'When in top, press <q> or <CTRL-C> to exit and return to CLI.\n'
    raw_input('Press <Enter> to continue.\n')
    os.system('top')

command.add_action('top-command', top_command)

TOP_COMMAND_DESCRIPTION = {
    'name'                : 'top',
    'mode'                : 'login',
    'no-supported'        : False,
    'short-help'          : 'Monitor process CPU and memory status',
    'action'              : 'top-command',
    'args'                : {},
}


ENABLE_SUBMODE_COMMAND_DESCRIPTION = {
    'name'                : 'enable',
    'mode'                : 'login',
    'no-supported'        : False,
    'help'                : 'Enter enable mode',
    'short-help'          : 'Enter enable mode',
    'doc'                 : 'enable',
    #'doc-example'         : 'enable-example',
    'command-type'        : 'config-submode',
    'submode-name'        : 'enable',
    'args'                : (),
}

CONFIGURE_SUBMODE_COMMAND_DESCRIPTION = {
    'name'                : 'configure',
    'mode'                : 'enable',
    'no-supported'        : False,
    'help'                : 'Enter configure mode',
    'short-help'          : 'Enter configure mode',
    'doc'                 : 'config',
    #'doc-example'         : 'config-example',
    'command-type'        : 'config-submode',
    'submode-name'        : 'config',
    'args'                : {
        'token'           : 'terminal',
        'optional'        : 'true',
    },
}


DEBUG_CLI_COMMAND_DESCRIPTION = {
    'name'                : 'debug',
    'mode'                : ['login', 'enable', 'config*'],
    'short-help'          : 'Manage various cli debugging features',
    'doc'                 : 'debug|debug-cli',
    'doc-example'         : 'debug|debug-cli-example',
    'args'                : {
        'choices' : (
            {
                'token'      : 'cli',
                'action'     : 'cli-set',
                'no-action'  : 'cli-unset',
                'variable'   : 'debug',
                'short-help' : 'Display more detailed information on errors',
                'doc'        : 'debug|cli',
            },
            {
                'token'      : 'cli-backtrace',
                'action'     : 'cli-set',
                'no-action'  : 'cli-unset',
                'variable'   : 'cli-backtrace',
                'short-help' : 'Display backtrace information on errors',
                'doc'        : 'debug|cli-backtrace',
            },
            {
                'token'      : 'cli-batch',
                'action'     : 'cli-set',
                'no-action'  : 'cli-unset',
                'variable'   : 'cli-batch',
                'short-help' : 'Disable any prompts to allow simpler batch processing',
                'doc'        : 'debug|cli-batch',
            },
            {
                'token'      : 'description',
                'action'     : 'cli-set',
                'no-action'  : 'cli-unset',
                'variable'   : 'description',
                'short-help' : 'Display verbose debug information while processing commands',
                'doc'        : 'debug|description',
            },
        )
    }
}

DEBUG_SHELL_CLI_COMMAND_DESCRIPTION = {
    'name'                : 'debug',
    'mode'                : ['enable', 'config*'],
    'short-help'          : 'Manage various cli debugging features',
    'doc'                 : 'debug|debug-cli',
    'doc-example'         : 'debug|debug-cli-example',
    'no-supported'        : False,
    'args'                : {
        'choices' : (
            {
                'token'      : 'python',
                'action'     : 'shell-command',
                'command'    : 'python',
                'short-help' : 'Enter a python shell',
                'doc'        : 'debug|python',
            },
            {
                'token'      : 'bash',
                'action'     : 'shell-command',
                'command'    : 'bash',
                'short-help' : 'Enter a bash shell',
                'doc'        : 'debug|bash',
            },
            (
                {
                    'token'      : 'ofad',
                    'action'     : 'ofad-ctl-command',
                    'command'    : None,
                    'short-help' : 'Run ofad-ctl command',
                    'doc'        : 'debug|ofad',
                },
                {
                    'field'       : 'command',
                    'type'        : 'string',
                    'syntax-help' : 'Command input. Use quotes if input has spaces',
                },
            ),
        )
    }
}


def set_hostname(no_command, data):
    if no_command:
        if 'hostname' in data:
            hostname = socket.gethostname()
            if data['hostname'] != hostname:
                raise error.ActionError('Hostname does not match existing name')
        hostname = 'localhost'
    else:
        hostname = data['hostname']
    try:
        shell.call('hostname ' + hostname)
    except subprocess.CalledProcessError:
        raise error.ActionError('Unable to set hostname')

command.add_action('implement-set-hostname', set_hostname,
                    {'kwargs': {
                                 'no_command' : '$is-no-command',
                                 'data'       : '$data',
                               } } )

HOSTNAME_COMMAND_DESCRIPTION = {
    'name'         : 'hostname',
    'mode'         : 'config*',
    'short-help'   : 'Set the switch name',
    'action'       : (
        { 'proc': 'implement-set-hostname' },
        { 'proc': 'prompt-update' },
    ),
    'no-action'    : (
        { 'proc': 'implement-set-hostname' },
        { 'proc': 'prompt-update' },
    ),
    'doc'          : 'core|hostname',
    #'doc-example'  : 'hostname-example',
    'args'         : (
        {
            'field'       : 'hostname',
            'type'        : 'string',
            'syntax-help' : 'Name for this switch',
            'optional-for-no' : True,
        },
    ),
}

DEFAULT_TIMEZONE =  'Etc/UTC'

def set_timezone(no_command, data, is_init):
    if no_command:
        if 'timezone' in data:
            with open(const.TZ_CFG_PATH, 'r') as fd:
                timezone = fd.readline().strip()
            if data['timezone'] != timezone:
                raise error.ActionError('Timezone does not match existing zone')
        timezone = DEFAULT_TIMEZONE
    else:
        timezone = data['timezone']
        if timezone not in pytz.all_timezones:
            raise error.ActionError('Invalid timezone')
    try:
        with open(const.TZ_CFG_PATH, 'w') as fd:
            fd.write(timezone)
        shell.call('dpkg-reconfigure -f noninteractive tzdata')
        command.action_invoke('implement-rsyslog-restart',
                              ({'is_init': is_init},))
    except subprocess.CalledProcessError:
        raise error.ActionError('Unable to set timezone')

command.add_action('implement-set-timezone', set_timezone,
                    {'kwargs': {
                                 'no_command' : '$is-no-command',
                                 'data'       : '$data',
                                 'is_init'    : '$is-init',
                               } } )

TIMEZONE_COMMAND_DESCRIPTION = {
    'name'         : 'timezone',
    'mode'         : 'config*',
    'short-help'   : 'Set the switch timezone',
    'action'       : 'implement-set-timezone',
    'no-action'    : 'implement-set-timezone',
    'doc'          : 'core|timezone',
    'doc-example'  : 'core|timezone-example',
    'args'         : (
        {
            'field'       : 'timezone',
            'type'        : 'string',
            'syntax-help' : 'Timezone for this switch',
            'optional-for-no' : True,
        },
    ),
}

def save_default_timezone():
    ws = os.path.join(const.DEFAULT_DIR, 'tz')
    if os.path.exists(ws):
        print 'Default timezone already exists.'
        return

    os.makedirs(ws)
    dst = os.path.join(ws, os.path.basename(const.TZ_CFG_PATH))
    with open(const.TZ_CFG_PATH, 'r') as f:
        print "default tz: %s" % f.read().strip()

    shutil.copy(const.TZ_CFG_PATH, dst)

def revert_default_timezone():
    ws = os.path.join(const.DEFAULT_DIR, 'tz')
    if not os.path.exists(ws):
        print 'Default timezone does not exist.'
        return

    src = os.path.join(ws, os.path.basename(const.TZ_CFG_PATH))
    with open(src, 'r') as f:
        print "default tz: %s" % f.read().strip()

    shutil.copy(src, const.TZ_CFG_PATH)
    shell.call('dpkg-reconfigure -f noninteractive tzdata')
    command.action_invoke('implement-rsyslog-restart',
                          ({'is_init': False},))

conf_state.register_save("tz", save_default_timezone)
conf_state.register_revert("tz", revert_default_timezone)

def locate ():
    shell.call("ofad-ctl beacon")

command.add_action('implement-locate', locate)

LOCATE_COMMAND_DESCRIPTION = {
    'name'         : 'locate',
    'mode'         : 'enable',
    'short-help'   : 'Flash the front panel LEDs',
    'action'       : 'implement-locate',
    'no-supported' : False,
    'doc'          : 'core|locate',
    'args'         : {}
}


def running_config_hostname(context, runcfg, words):
    comp_runcfg = []

    # collect component-specific config
    hostname = socket.gethostname()
    if hostname != 'localhost':
        comp_runcfg.append('hostname %s\n' % hostname)

    # attach component-specific config
    if len(comp_runcfg) > 0:
        runcfg.append('!\n')
        runcfg += comp_runcfg

hostname_running_config_tuple = (
    (
        {
            'optional'   : False,
            'field'      : 'running-config',
            'type'       : 'enum',
            'values'     : 'hostname',
            'short-help' : 'Configuration for hostname',
            'doc'        : 'running-config|show-hostname',
        },
    ),
)

run_config.register_running_config('hostname', 500,  None,
                                   running_config_hostname,
                                   hostname_running_config_tuple)

def running_config_timezone(context, runcfg, words):
    comp_runcfg = []

    # collect component-specific config
    try:
        with open(const.TZ_CFG_PATH, 'r') as fd:
            timezone = fd.readline().strip()
        comp_runcfg.append('timezone %s\n' % timezone)
    except:
        pass

    # attach component-specific config
    if len(comp_runcfg) > 0:
        runcfg.append('!\n')
        runcfg += comp_runcfg

timezone_running_config_tuple = (
    (
        {
            'optional'   : False,
            'field'      : 'running-config',
            'type'       : 'enum',
            'values'     : 'timezone',
            'short-help' : 'Configuration for timezone',
            'doc'        : 'running-config|show-timezone',
        },
    ),
)

run_config.register_running_config('timezone', 600,  None,
                                   running_config_timezone,
                                   timezone_running_config_tuple)
