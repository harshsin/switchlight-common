# Copyright (c) 2013  BigSwitch Networks

import command
import run_config

import error
import subprocess
import os
import socket
import cfgfile
from datetime import timedelta

from sl_util import shell
from switchlight.platform.current import SwitchLightPlatform
import re

Platform=SwitchLightPlatform()
FW_PRINTENV = '/usr/bin/fw_printenv'

def fw_getenv(var):
    try:
        value=subprocess.check_output([FW_PRINTENV, var]);
        if value.startswith("%s=" % var):
            value = value[len(var)+1:]
        return value
    except Exception, e:
        pass
    return None

def parse_sl_version(ver):
    try:
        m = re.match(r"(SwitchLight) (.*?) (\(.*?\))", ver)
        return m.group(1,2,3)
    except Exception, e:
        return (ver, "", "")

VERSION_FILE = '/etc/sl_version'

def show_version(data):
    out = []

    with open(VERSION_FILE, "r") as f:
        sl_version = f.readline().strip()

    if not 'full' in data:
        # Single line version
        out.append(sl_version)
    else:
        # Full version information

        # UBoot version, if appliable
        uver=fw_getenv('ver')
        out.append("UBoot Version: %s" % (uver if uver else "Not available on this platform."))

        # Platform Information
        out.append(str(Platform))

        # Loader/Installer information
        sli=fw_getenv('sl_installer_version')
        if sli:
            fs_ver = parse_sl_version(sli)
            out.append("SwitchLight Loader Version: %s %s" % (fs_ver[0], fs_ver[1]))
            out.append("SwitchLight Loader Build: %s" % (fs_ver[2]))
        else:
            out.append("SwitchLight Loader Version: Not available on this platform.")
            out.append("SwitchLight Loader Build: Not available on this platform.")
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
        (sout, serr, rc) = shell.call('sensors')
        # filter out text in parens, blank lines, "Adapter: i2c-0-mux", etc
        print "\n".join([line.split('(',1)[0] for line in sout.split('\n') \
                             if ':' in line and 'Adapter' not in line])
    except:
        pass

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

SHOW_USERS_COMMAND_DESCRIPTION = {
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


# Perform 'copy tech-support' command

tech_support_tmpfile = 'tech-support.tmp'
tech_support_script = [
    'date',
    'cat /etc/sl_version',
    'cat /mnt/flash/boot-config',
    'cat /mnt/flash/startup-config',
    'cat /var/log/dmesg',
    'cat /etc/network/interfaces',
    'ifconfig ma1',
    'cat /etc/resolv.conf',
    'cat /etc/ntp.conf',
    'uptime',
    'cat /proc/meminfo',
    'lsmod',
    'ps auxww',
    'df',
    'sensors',
    'cat /etc/rsyslog.conf',
    'cat /etc/ofad.conf',
    'cat /etc/snmp/snmpd.conf',
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
]

def save_tech_support(data):
    p = subprocess.Popen(args=['/bin/bash'],
                         bufsize=1,
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE
                         )
    p.stdin.write('cd /tmp\n')
    p.stdin.write('rm -f %s*\n' % tech_support_tmpfile)
    for li in tech_support_script:
        p.stdin.write("echo ===== '%s' >>%s\n" % (li, tech_support_tmpfile))
        p.stdin.write('%s >>%s\n' % (li, tech_support_tmpfile))
    p.stdin.write('gzip -c %s >/mnt/flash2/tech-support_`date +%%y%%m%%d%%H%%M%%S`.gz\n' % tech_support_tmpfile)
    p.stdin.write('rm -f %s*\n' % tech_support_tmpfile)


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
    'doc'          : 'hostname',
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

def locate ():
    shell.call("ofad-ctl modules brcm led-flash 3 100 100 10")

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

