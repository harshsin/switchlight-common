#!/usr/bin/python
#
#  cli - switch cli shell
#  (c) in 2013 by Big Switch Networks
#  All rights reserved
#
# -----------------------------------------------------------------------------

from optparse import OptionParser
import command
import sys
import os
import re
import doc
import imp
import traceback
import url_cache
from prettyprint import PrettyPrinter
import utif
import run_config
import locale
import socket
import time
import subprocess


class MainSh():

    debug = False               # general cli debugging
    debug_backtrace = False     # backtrace on failures
    description = False         # help debug command descriptions
    display_rest = False        # display rest call details
    display_reply_rest = False  # display rest call replies details

    command_dict = {}
    mode_stack = []
    reserved_words = ['unknown', 'all']

    run = True

    # Are we running in batch mode, i.e. "cat commands | bigcli"
    batch = False

    # used for parsing as an arg
    local_name_pattern = "config://([A-Za-z0-9_:@\-\.\/]+$)"

    # contained objects
    pp = None

    warning_suppress = False            #

    #
    # --------------------------------------------------------------------------------
    # warning
    #  When a config is getting replayed, warnings are suporessed.  Other situations
    #  may also require supression of warnings.
    #
    def warning(self, message):
        if not self.warning_suppress:
            print "Warning: %s" % message
    
    #
    # --------------------------------------------------------------------------------
    # supress_warnings(self)
    #
    def suppress_warnings(self):
        self.warning_suppress = True

    #
    # --------------------------------------------------------------------------------
    # enable_warnings(self)
    #
    def enable_warnings(self):
        self.warning_suppress = False


    config_replay = False               # only true while replaying config files
    #
    # --------------------------------------------------------------------------------
    # config_replay_start
    #  Behavior is different during config replay.  Warnings are
    #  suppressed, and other error conditions may be relaxed, for example
    #  the switch submode interface command requires that the interface exist,
    #  and the interface names are discovered by floodlight, which means that during
    #  config replay for statup, an unknown-named-interface may need to be accepted
    #  since that interface may appear once the switch connected to floodlight, and
    #  floodlight writes the ports for this switch.
    #
    def config_replay_start(self):
        self.suppress_warnings()
        self.config_replay = True
    
    #
    # --------------------------------------------------------------------------------
    # config_replay_done
    #
    def config_replay_done(self):
        self.enable_warnings()
        self.config_replay = False

    #
    # --------------------------------------------------------------------------------
    # config_replay_active
    #  Returns true when a configuration is getting replayed
    #
    def config_replay_active(self):
        return self.config_replay

    #
    # --------------------------------------------------------------------------------
    # note
    #  When a config is getting replayed, warnings/notes are suporessed.  Other situations
    #  may also require supression of warnings.
    #
    def note(self, message):
        if not self.warning_suppress:
            print "Note: %s" % message

    #
    # --------------------------------------------------------------------------------
    # debug_msg
    #  Debugging message cover.  Only enabled for display once 'debug cli' is performed.
    #
    def debug_msg(self, message):
        if self.debug:
            print "debug_msg: %s" % message
            
    #
    # --------------------------------------------------------------------------------
    # error_msg
    #  Error message cover, to ensure consistent error messages
    #
    @staticmethod
    def error_msg(message):
        return "Error: %s" % message

    #
    # --------------------------------------------------------------------------------
    # syntax_msg
    #  Syntax error message cover, to ensure consistent error messages
    #
    @staticmethod
    def syntax_msg(message):
        return "Syntax: %s" % message 

    #
    # --------------------------------------------------------------------------------
    # completion_error_msg
    #  Error message cover during completion, to ensure consistent error messages
    #
    def completion_error_msg(self, message):
        self.print_completion_help("Error: %s" % message)

    #
    # --------------------------------------------------------------------------------
    # completion_syntax_msg
    #  Syntax error message cover during completion, to ensure consistent error messages
    #
    def completion_syntax_msg(self, message):
        self.print_completion_help("Syntax: %s" % message)

    #
    # --------------------------------------------------------------------------------
    # init_command_dict
    #
    def init_command_dict(self):

        #
        # command_submode_dict saves a little information about
        # submode commands.  The index is 'mode', and the resulting
        # list contains strings, each of which is a submode command
        # available in that mode.
        self.command_submode_dict = {}

        #
        # associate command names with features, so that when
        # help is provided, command names which have associated
        # features are validated to determine whether the feature
        # is enabled or not.
        self.command_name_feature = {}

        #
        # command_nested_dict is indexed by mode, and contains
        # command configured with a trailing '*', for example
        # 'mode' : 'config-*', intended to mean "all nested
        # submodes after config-'.  This can't be computed until
        # all submodes are known, which means all the command
        # describption must be configured., for these to make sense.
        self.command_nested_dict = {}

        #
        # commands which start at 'login'
        #
        self.command_nested_dict['login'] = [ 'show', 'logout', 'exit',
                                              'history', 'help',
                                              'no' ]

        self.command_nested_dict['enable'] = [ 'end' ]

        self.command_dict['config-internal'] = ['lint', 'permute']

    #
    # --------------------------------------------------------------------------------
    #
    def command_packages_path(self):
        command_descriptions = 'desc'
        desc_path = os.path.join(os.path.dirname(__file__), command_descriptions)
        if os.path.exists(desc_path):
            return desc_path
        return None


    #
    # --------------------------------------------------------------------------------
    #
    def command_packages_exists(self, version):
        desc_path = self.command_packages_path()
        if desc_path == None:
            return None
        version_path = os.path.join(desc_path, version)
        if os.path.exists(version_path):
            return version_path
        return None


    #
    # --------------------------------------------------------------------------------
    #
    def add_command_packages(self, version):
        """
        Add all command and output format components

        """
        desc_path = self.command_packages_path()
        if desc_path == None:
            print 'No Command Descriptions subdirectory'
            return

        for path_version in [x for x in os.listdir(desc_path) if x.startswith(version)]:
            result_tuple = imp.find_module(path_version, [desc_path])
            imp.load_module(version, result_tuple[0], result_tuple[1], result_tuple[2])
            new_path = result_tuple[1]
            cmds_imported = []
            for cmds in os.listdir(new_path):
                (prefix, suffix) = os.path.splitext(cmds)
                if (suffix == '.py' or suffix == '.pyc') and prefix not in cmds_imported and prefix != '__init__':
                    cmds_imported.append(prefix)
                    result_tuple = imp.find_module(prefix, [new_path])
                    module = imp.load_module(prefix, result_tuple[0], result_tuple[1], result_tuple[2])
                    command.add_commands_from_module(version, module, self.dump_syntax)
                    self.pp.add_formats_from_module(version, module)
            # print cmds_imported

    #
    # --------------------------------------------------------------------------------
    #
    def choices_text_builder(self, matches, max_len = None, col_width = None):
        """
        @param max_len integer, describes max width of any entry in matches
        @param col_width integer, colun width
        """
        # Sort the choices alphabetically, but if the token ends
        # in a number, sort that number numerically.
        try:
            entries = sorted(matches, utif.completion_trailing_integer_cmp)
        except Exception, e:
            traceback.print_exc()

        if col_width == None:
            # request to look it up
            (col_width, line_length) = self.pp.get_terminal_size()
            col_width = min(120, col_width)

        if max_len == None:
            # request to compute the max length
            max_len = len(max(matches, key=len))

        count = len(matches)
        max_len += 1 # space after each choice
        if max_len > col_width:
            # one per line?
            pass
        else:
            per_line = col_width / max_len
            lines = (count + (per_line - 1)) / per_line
            if lines == 1:
                return ''.join(['%-*s' % (max_len, m) for m in entries])
            else:
                # fill the columns, which means skipping around the entries
                result = []
                for l in range(lines):
                    result.append(['%-*s' % (max_len, entries[i])
                                   for i in range(l, count, lines)])
                lines = [''.join(x) for x in result]
                return '\n'.join(lines)


    #
    # --------------------------------------------------------------------------------
    #
    def matches_hook(self, subs, matches, max_len):

        # one shot disabler, used to disable printing of completion
        # help for two-column help display (for '?' character). 
        # completion printing here can only display the possible selections,
        # for two-column mode, the reason why each keyword was added
        # needs to be displayed, which is no longer available here.
        if self.completion_print == False:
            self.completion_print = True
            return
        
        choices_text = self.choices_text_builder(matches, max_len)
        self.print_completion_help(choices_text)

    #
    # --------------------------------------------------------------------------------
    #
    def pre_input_hook(self):
        """
        """
        pass

    
    #
    # --------------------------------------------------------------------------------
    # 
    def desc_version_to_path_elem(self, version):
        """
        Version numbers like 1.0 need to be converted to the path
        element associated with the number, like version100
        """
        try:
            version_number = float(version)
            version = 'version%03d' % int(version_number * 100)
        except:
            pass

        return version
        

    #
    # --------------------------------------------------------------------------------
    # init
    #
    def init(self):
        self.completion_print = True

        self.dump_syntax = False
        self.debug = False

        parser = OptionParser()
        parser.add_option("-S", "--syntax", dest='dump_syntax',
                          help="display syntax of loaded commands",
                          action='store_true', default=False)
        parser.add_option("-i", "--init", dest='init',
                          help="do not perform initialization checks",
                          action='store_true', default=False)
        parser.add_option("-d", "--debug", dest='debug',
                          help='enable debug for cli (debug cli)',
                          action='store_true', default=False)
        parser.add_option("-v", "--version", dest='desc_version',
                          help='select command versions (description group)',
                          default=None)
        parser.add_option('-m', "--mode", dest='starting_mode',
                          help='once the cli starts, nest into this mode')
        parser.add_option('-q', "--quiet", dest='quiet',
                          help='suppress warning messages',
                          action='store_true', default=False)
        (self.options, self.args) = parser.parse_args()
        self.dump_syntax = self.options.dump_syntax
        if not self.dump_syntax:
            self.dump_syntax = False
        self.debug = self.options.debug

        # command option, then env, then default
        self.desc_version = self.options.desc_version
        if self.desc_version == None:
            self.desc_version = os.getenv('BIGCLI_COMMAND_VERSION')
        if self.desc_version == None:
            self.desc_version = '0.01'   # 'version001'

        if self.desc_version:
            self.desc_version = self.desc_version_to_path_elem(self.desc_version)
        if self.debug:
            print "Parsing " + self.desc_version

        self.length = 0 # screen length.

        self.update_prompt()

        if not sys.stdin.isatty():
            self.batch = True

        self.init_command_dict()

        self.completion_reset()
        self.completion_skip = False
        self.completion_cache = True
        readline.set_completer(self.completer)
        readline.set_completer_delims("\t ")
        # readline.set_pre_input_hook(self.pre_input_hook)
        readline.set_completion_display_matches_hook(self.matches_hook)
        self.push_mode("login")

        # starting mode, 
        starting_mode = self.options.starting_mode
        if starting_mode == None:
            starting_mode = os.getenv('BIGCLI_STARTING_MODE')

        if starting_mode:
            if starting_mode == 'login':
                pass
            elif starting_mode == 'enable':
                self.push_mode("enable")
            elif starting_mode == 'config':
                self.push_mode("enable")
                self.push_mode("config")
            else:
                print 'Only login, enable or config allowed as starting modes'

        # process quiet option
        quiet = self.options.quiet
        if quiet == None:
            quiet = os.getenv('BIGCLI_SUPPRESS_WARNING')

        if quiet:
            self.supress_warnings()

        self.pp = PrettyPrinter(self)

        #
        # pattern matches
        #
        self.IP_ADDR_RE = re.compile(r'^(\d{1,3}\.){3}\d{1,3}$')
        self.CIDR_RE = re.compile(r'^((\d{1,3}\.){3}\d{1,3})/(\d{1,2}?)$')
        self.MAC_RE = re.compile(r'^(([A-Fa-f\d]){2}:?){5}[A-Fa-f\d]{2}$')
        self.DPID_RE = re.compile(r'^(([A-Fa-f\d]){2}:?){7}[A-Fa-f\d]{2}$')
        self.ACL_RE = re.compile(r'^\d+$')  # just the leading digits
        self.DIGITS_RE = re.compile(r'^\d+$')
        self.HEX_RE = re.compile(r'^0x[0-9a-fA-F]+$')

        run_config.init_running_config(self)

        # Initialize all doc tags, use a subdirectory based on
        # the locale.   This may be incorect if there's nothing
        # in the configured locale for the returned locale value.
        lc = locale.getdefaultlocale()
        if lc == None or lc[0] == None:
            print 'Locale not configured ', lc
            lc = ('en_US', 'UTF8')
        doc_dir = 'documentation/%s' % lc[0]
        if doc_dir is None:
            doc_dir = os.path.join(os.path.dirname(__file__), 'documentation', lc[0])
        doc.add_doc_tags(doc_dir)

        doc.add_doc_tags(os.path.dirname(__file__), 'documentation')

        # Initialize the command module, than add command packages
        command.init_command(self)
        self.add_command_packages(self.desc_version)

        # save for possible later use
        #print self.pp.format_table(self.pp.format_details())
        
        #
        if self.debug:
            for (n,v) in self.command_submode_dict.items():
                print "SUBMODE %s %s" % (n,v)
            for (n,v) in self.command_dict.items():
                print "MODE %s %s" % (n,v)


    #
    # methods to manage the mode_stack, prompts, and mode_obj/mode_obj_type
    #

    #
    # --------------------------------------------------------------------------------
    # update_prompt
    #  There are several different prompts depending on the current mode:
    #  'host'>                                  -- login mode
    #  'host'#                                  -- enable mode
    #  'host'(config)#                          -- config mode
    #
    def update_prompt(self):
        hostname = socket.gethostname()
        if self.current_mode().startswith("config"):
            current_mode = "(" + self.current_mode()
            self.prompt = hostname + current_mode + ")# "
        elif self.current_mode() == "enable":
            self.prompt = hostname + "# "
        else:
            self.prompt = hostname + "> "

    #
    # --------------------------------------------------------------------------------
    # push_mode
    #  Every pushed mode is a quad: <modeName, tableName, specificRow, exitCallback>
    #
    #  obj_type is the name of an associated table/model (tableName)
    #  obj is the key's value for the table/model's key (specificRow)
    # 
    #  The cli currently supports a mode of table/model row edit, where the
    #  name of the table/mode is entered, along with an associated value for
    #  key-column of the table.   Once in that mode, other fields of the table
    #  can be edited by entering the name of the field, along with a new value.
    #
    #  The exitCallback is the nane of a method to call when the current pushed
    #  level is getting pop'd.
    #
    def push_mode(self, mode_name, exitCallback=None):
        self.mode_stack.append( { 'mode_name' : mode_name,
                                  'exit' : exitCallback} )
        self.update_prompt()

    #
    # --------------------------------------------------------------------------------
    # pop_mode
    #  Pop the top of the stack of mode's.
    #
    def pop_mode(self): 
        m = self.mode_stack.pop()
        if len(self.mode_stack) == 0:
            self.run = False
        else:
            self.update_prompt()
        return m
    
    #
    # --------------------------------------------------------------------------------
    # mode_stack_to_rest_dict
    #  Convert the stack of pushed modes into a collection of keys.
    #  Can be used to build the rest api dictionary used for row creates
    #
    def mode_stack_to_rest_dict(self, rest_dict):
        #
        for x in self.mode_stack:
            if x['mode_name'].startswith('config-'):
                rest_dict[x['obj_type']] = x['obj']

        return rest_dict

    #
    # --------------------------------------------------------------------------------
    # current_mode
    #  Return the string describing the current (top) mode.
    #
    def current_mode(self): 
        if len(self.mode_stack) < 1:
            return ""
        return self.mode_stack[-1]['mode_name']

    #
    # --------------------------------------------------------------------------------
    # in_config_submode
    #  Return true when the current mode is editing the contents of one of the
    #  rows of the table/model store.
    #
    def in_config_submode(self, prefix = "config-"):
        return self.current_mode().startswith(prefix)

    #
    # --------------------------------------------------------------------------------
    # in_config_mode
    #  Returns true for any config mode; this is any nested edit mode,
    #  along with the base config mode
    #
    def in_config_mode(self):
        return self.current_mode().startswith("config")

    # --------------------------------------------------------------------------------
    # set_current_mode_obj
    #  Sets the name of the selected row (key's value)
    #
    def set_current_mode_obj(self, obj):
        self.mode_stack[-1]['obj'] = obj
        
    #
    # --------------------------------------------------------------------------------
    # get_current_mode_obj
    #  Gets the name of the current mode's selected row value (key's value)
    #  This can return None.
    #
    def get_current_mode_obj(self):
        return self.mode_stack[-1]['obj']

    #
    # --------------------------------------------------------------------------------
    # set_current_mode_obj_type
    #  Set the table/model name for this current mode.
    #
    def set_current_mode_obj_type(self, obj_type):
        self.mode_stack[-1]['obj_type'] = obj_type

    #
    # --------------------------------------------------------------------------------
    # get_current_mode_obj_type
    #  Get the table/model name for this current mode.
    #
    def get_current_mode_obj_type(self):
        return self.mode_stack[-1]['obj_type']

    #
    # --------------------------------------------------------------------------------
    # get_nested_mode_obj
    #  Get the id of the object that matches the given object type
    #  starting from the current mode and working backwards to the
    #  top-level mode. If there's no mode that matches the given
    #  object type, then return None (maybe should handle as exception?).
    #
    def get_nested_mode_obj(self, obj_type):
        for i in range(1, len(self.mode_stack)+1):
            # Use negative index so we search from the top/end of the stack
            mode = self.mode_stack[-i]
            if mode['obj_type'] == obj_type:
                return mode['obj']
        return None
    
    #
    # helper functions to access commands, obj_types, and fields
    #

    @staticmethod
    def title_of(command):
        return command['title'] if type(command) is dict else command

    #
    # --------------------------------------------------------------------------------
    # commands_feature_enabled
    #
    def commands_feature_enabled(self, commands):
        return [self.title_of(x) for x in commands
                if (not self.title_of(x) in self.command_name_feature) or
                    command.isCommandFeatureActive(self.title_of(x),
                           self.command_name_feature[self.title_of(x)])]

    #
    # --------------------------------------------------------------------------------
    # commands_for_mode
    #
    def commands_for_mode(self, mode):
        """
        Walk the command dict, using interior submodes and compiling
        the list of available commands (could rebuild command_dict()
        to contain all the possible commands, but its good to know
        exactly which commands apply to this submode)
        """

        # make a new list, so that items don't get added to the source
        ret_list = list(self.command_nested_dict.get('login', []))
        if mode == 'login':
            ret_list += self.command_dict.get('login', [])
            return ret_list
        ret_list += self.command_nested_dict.get('enable', [])
        if mode == 'enable':
            ret_list += self.command_dict.get('enable', [])
            return ret_list

        if mode == 'config':
            ret_list += self.command_nested_dict.get('config', [])
            ret_list += self.command_dict.get('config', [])
            return ret_list
        
        for idx in [x for x in self.command_nested_dict.keys() if mode.startswith(x)]:
            ret_list += self.command_nested_dict.get(idx, [])

        ret_list += self.command_dict.get(mode, [])

        # manage command who's names are regular expressions
        result = [x['re'] if type(x) == dict else x  for x in ret_list]

        return result

    #
    # --------------------------------------------------------------------------------
    # commands_for_current_mode_starting_with
    #
    def commands_for_current_mode_starting_with(self,
                                                start_text = "", completion = None):
        """
        One of the difficult issues here is when the first item
        isn't a token, but rather a regular expression.  This currently occur
        in a few places in the command description, and the mechanism for
        dealing with the issue here is ... uhm ... poor.  The code here is
        a stopgap, and assumes the only regular expression supported
        is the <digits> one.  This could be make a bit better based on
        the submode, but really, this entire first-token management should
        be improved.
        """
        if completion == None:
            completion = False

        mode_list = self.commands_for_mode(self.current_mode())
        ret_list = self.commands_feature_enabled(utif.unique_list_from_list(mode_list))

        def prefix(x, start_text, completion):
            if type(x) == str and x.lower().startswith(start_text.lower()):
                return True
            if not completion and type(x) == re._pattern_type:
                return x.match(start_text)
            return False

        def pattern_items(ret_list, prefix):
            matches = []
            for p in [x for x in ret_list if type(x) == re._pattern_type]:
                for c in command.command_registry:
                    if c['mode'] != self.current_mode():
                        continue
                    if type(c['name']) != dict:
                        continue
                    first_word = c['name']
                    if 'completion' not in first_word:
                        continue
                    completion = first_word['completion']
                    if first_word['pattern'] == p.pattern:
                        result = {}
                        scopes = [ first_word,
                                   {
                                    'completions' : result,
                                    'data'        : {},
                                    'text'        : prefix,
                                   },
                                 ]
                        command._call_proc(completion,
                                           command.completion_registry,
                                           scopes, c)
                        matches = result.keys()
            return matches

        matches = [x for x in ret_list if prefix(x, start_text, completion)]
        if completion:
               matches += pattern_items(ret_list, start_text)
        return matches


    #
    # --------------------------------------------------------------------------------
    # complete_optional_parameters
    #
    # Parse optional parameters.  These can occur in any order.
    # Params argument is a hash mapping a parameter name to type
    # information.
    # words argument are remaining words in the command
    # line that aren't yet parsed
    #
    def complete_optional_parameters(self, params, words, text):
        i = 0
        while i < len(words):
            final = i+1 >= len(words)
            word = words[i]
            possible = [x for x in params if x.startswith(word)]
            param_name = possible[0]
            param = params[param_name]
             
            if (param['type'] != 'flag'):
                if (final):
                    argument = text
                    if (param['type'] == 'enum'):
                        return [x 
                                for x in param['values'] 
                                if x.startswith(argument)]
                    elif argument == '':
                        if ('syntax_help' in param):
                            self.print_completion_help(param['syntax_help'])
                        else:
                            self.print_completion_help('[%s argument]' % word)
                        return
                i += 1
            i += 1

        return [x for x in params if x.startswith(text)]

    #
    # --------------------------------------------------------------------------------
    # parse_optional_parameters
    #
    @staticmethod
    def parse_optional_parameters(params, words):
        parsed = {}
        i = 0
        while i < len(words):
            word = words[i]
            possible = [x for x in params if x.startswith(word)]
            if len(possible) == 0:
                raise ParamException('unknown option: %s' % word)
            elif len(possible) > 1:
                raise ParamException('ambiguous option: %s\n%s' % 
                                     (word, "\n".join(possible)))
            else:
                param_name = possible[0]
                param = params[param_name]
                if (param['type'] == 'flag'):
                    parsed[param_name] = True
                else:
                    if i+1 < len(words):
                        argument = words[i+1]
                        if (param['type'] == 'string'):
                            parsed[param_name] = argument
                        elif (param['type'] == 'int'):
                            try:
                                parsed[param_name] = int(argument)
                            except ValueError:
                                raise ParamException('option %s requires ' + 
                                                     'integer argument'
                                                     % word)
                        elif (param['type'] == 'enum'):
                            arg_possible = [x 
                                            for x in param['values'] 
                                            if x.startswith(argument)]
                            if (len(arg_possible) == 0):
                                raise ParamException('option %s value must be in (%s)' % 
                                                     (word,", ".join(param['values'])))
                            elif (len(arg_possible) > 1):
                                raise ParamException('ambiguous option %s value:\n%s' % 
                                                     (word, "\n".join(arg_possible)))
                            else:
                                parsed[param_name] = arg_possible[0]
                        i += 1
                    else:
                        raise ParamException('option %s requires an argument' 
                                                 % word)
            i += 1
        return parsed

    #
    # --------------------------------------------------------------------------------
    #
    # Commands of the CLI. A command "foo" should have two methods:
    #   cp_foo(self, text, state) - completion for this command, return list of options
    #   do_foo(self, line) - execute the command
    #
    # Given a string 'word', not containing any spaces, nor any break
    #  characters associated with python, return the a invocable method
    #  note that '-' are permitted, and converted to '_''s
    #

    #
    # --------------------------------------------------------------------------------
    # method_from_name
    #
    def method_from_name(self, prefix, name):
        if type(name) != str:
            return None
        return getattr(self, prefix+name.replace("-","_"), None)
        
    #
    # --------------------------------------------------------------------------------
    # command_method_from_name
    #
    def command_method_from_name(self, name):
        return self.method_from_name("do_", name)               # do_XXX methods

    #
    # --------------------------------------------------------------------------------
    # command_show_method_from_name
    #
    def command_show_method_from_name(self, name):
        return self.method_from_name("do_show_", name)          # do_show_XXX methods

    #
    # --------------------------------------------------------------------------------
    # completion_method_from_name
    #
    def completion_method_from_name(self, name):
        return self.method_from_name("cp_", name)               # cp_XXX (completion) methods

    #
    # --------------------------------------------------------------------------------
    # completion_show_method_from_name
    #
    def completion_show_method_from_name(self, name):
        return self.method_from_name("cp_show_", name)          # cp_show_XXX (completion) methods

    #
    # --------------------------------------------------------------------------------
    # unique_key_from_non_unique
    #
    # Primary keys for cassandra for some keys are contenations of
    #  several non-unique keys separated by a character not used for
    #  any other purpose (in this case '|').  The concatenation
    #  of non-unique keys is intended to create a unique key.
    #
    def unique_key_from_non_unique(self, words):
        return "|".join(words)
    
    #
    # --------------------------------------------------------------------------------
    # prefix_search_key
    #  Prefix's of primary keys for keys's built through unique_key_from_non_unique()
    #

    def prefix_search_key(self, words):
        return self.unique_key_from_non_unique(words) + "|"

    def cp_exit(self, words, text, completion_char):
        self.print_completion_help("<cr>")

    def do_exit(self, words=None):
        if self.mode_stack[-1]['exit']:
            method = self.mode_stack[-1]['exit']
            method()
        self.pop_mode()

    def cp_logout(self, words, text, completion_char):
        self.print_completion_help("<cr>")

    def do_logout(self, words=None):
        self.run = False

    def cp_end(self, words, text, completion_char):
        self.print_completion_help("<cr>")

    def do_end(self, words=None):
        while self.current_mode().startswith("config"):
            self.pop_mode()


    #
    # --------------------------------------------------------------------------------
    # append_when_missing
    #
    @staticmethod
    def append_when_missing(unique_list, item):
        if not item in unique_list:
            unique_list.append(item)

    #
    # --------------------------------------------------------------------------------
    # show_command_prefix_matches
    #  show command's prefix matches, return True when the command matches
    #  fpr the prefix described, used by get_valid_show_options() during
    #  cp_show() to collect available command choice.
    #
    def show_command_prefix_matches(self, command, prefix):
        # convert any '-'s in the prefix tp "_"'s since func's have _ separators
        prefix = prefix.replace("-", "_")
        if command.startswith("do_show_"+prefix):
            return True
        return False

    #
    # --------------------------------------------------------------------------------
    # get_valid_show_options 
    #  used by cp_show to identify completion optinos
    #
    # returns a dictionary with two elements: 'commands', and 'objects'.
    # the 'commands' element is a list of possible commands while
    # the 'objects' element is a list of possible objects (tables/models in the store)
    #
    def get_valid_show_options(self, text):
        ret_hash = {}
        # first get commands
        opts = []
        matching_methods = [x for x in dir(self)
                            if self.show_command_prefix_matches(x, text)]

        for method in matching_methods:
            m = re.search("do_show_(.*)", method)
            if m:
                self.append_when_missing(opts, m.group(1).replace("_","-"))
        ret_hash["commands"] = opts

        # now get obj_types we can show
        opts = self.all_obj_types_starting_with(text)
        if self.in_config_submode() and "this".startswith(text):
            opts.append("this")
        ret_hash["objects"] = opts
        return ret_hash


    #
    # --------------------------------------------------------------------------------
    # cp_show_object
    #  show <obj_type> (words[0] == obj_type)
    #
    def cp_show_object(self, words, text, completion_char):
        if len(words) == 1:
            return objects_starting_with(words[0], text)
        else:
            self.print_completion_help("<cr>")

    #
    # --------------------------------------------------------------------------------
    # do_date
    #
    @staticmethod
    def do_date(words):
        return time.strftime("%Y-%m-%d %H:%M:%S %Z", time.localtime())

    #
    # --------------------------------------------------------------------------------
    # generate_subpprocess_output
    #
    def generate_subprocess_output(self, cmd):
        process = subprocess.Popen(cmd, shell=True,
                                   stdout=subprocess.PIPE, 
                                   stderr=subprocess.STDOUT,
                                   bufsize=1)
        while True:
            line = process.stdout.readline()
            if line != None and line != "":
                yield line
            else:
                break

    #
    # --------------------------------------------------------------------------------
    #
    def cp_help(self, words, text, completion_char):
        """
        Completion for the help command must be done using the collection
        of command descriptions; ie: help uses the command descriptions
        to complete the help commands.
        """
        if completion_char == ord('?'):
            if len(words) > 1:
                command.do_command_completion_help(words[1:], text)
            else:
                print self.help_splash([], text)
            return
        if len(words) == 1:
            items = self.commands_for_current_mode_starting_with(text)
            return utif.add_delim(items, ' ')
        else:   
            return command.do_command_completion(words[1:], text)


    #
    # --------------------------------------------------------------------------------
    # command_short_help
    #  Associate short help strings with known commands
    #  These ought to be associated with the COMMAND_DESCRIPTIONs,
    #  but since not all command use that mechanism yet, provide
    #  these short descriptions here.
    #
    command_short_help = {
        'exit'           : 'Exit current mode',
        'help'           : 'Help on commands or topics',
        'history'        : 'Display history of commands',
        'logout'         : 'Exit from cli',
        'no'             : 'Delete or disable configuration parameters',
        'end'            : 'End all nested configuration modes',
        # FIXME these commands are defined in multiple files
        'show'           : 'Display configuration or settings',
        'logging'        : 'Configure logging/syslog',
    }

    #
    # --------------------------------------------------------------------------------
    # help_splash
    #
    def help_splash(self, words, text):
        ret = ""
        if not words:
            if text == "":
                ret += "For help on specific commands type help <topic>\n"

            count = 0
            longest_command = 0
            # this submode commands
            s_ret = ""
            mode = self.current_mode()
            nested_mode_commands = [self.title_of(x) for x in
                                    self.command_nested_dict.get(mode, [])]
            possible_commands = [self.title_of(x) for x in
                                 self.command_dict.get(mode, []) ] + \
                                nested_mode_commands
            available_commands = self.commands_feature_enabled(possible_commands)
            submode_commands = sorted(utif.unique_list_from_list(available_commands))
            if len(submode_commands):
                longest_command = len(max(submode_commands, key=len))
            for i in submode_commands:
                if not i.startswith(text):
                    continue
                count += 1
                short_help = command.get_command_short_help(i)
                if not short_help:
                    short_help = self.command_short_help.get(i, None)
                if short_help:
                    s_ret += "  %s%s%s\n" % (i,
                                          ' ' * (longest_command - len(i) + 1),
                                          short_help)
                else:
                    s_ret += "  %s\n" % i

            # commands
            c_ret = ""
            upper_commands = [x for x in self.commands_for_current_mode_starting_with()
                              if not x in submode_commands]
            commands = sorted(upper_commands)
            if len(commands):
                longest_command = max([len(x) for x in commands] + [longest_command])
            for i in commands:
                if not i.startswith(text):
                    continue
                count += 1
                short_help = command.get_command_short_help(i)
                if not short_help:
                    short_help = self.command_short_help.get(i, None)
                if short_help:
                    c_ret += "  %s%s%s\n" % (i,
                                          ' ' * (longest_command - len(i) + 1),
                                          short_help)
                else:
                    c_ret += "  %s\n" % i
                
            # objects
            o_ret = ""
            if self.in_config_mode():
                obj_types = []
                if len(obj_types) > 0:
                    for i in obj_types:
                        longest_command = max([len(x) for x in commands] +
                                              [longest_command])
                    for i in obj_types:
                        if i in commands:
                            continue
                        if i.startswith(text):
                            count += 1
                            short_help = self.obj_type_short_help.get(i, None)
                            if short_help:
                                o_ret += "  %s%s%s\n" % (i,
                                            ' ' * (longest_command - len(i) + 1),
                                            short_help)
                            else:
                                o_ret += "  %s\n" % i

            # fields
            f_ret = ""
            if self.in_config_submode():
                # try to get both the fields and the commands to line up
                longest_field = longest_command
                for i in self.fields_for_current_submode_starting_with():
                    if i.startswith(text):
                        longest_field = max(longest_field, len(i))
 
                f_count = 0
                # LOOK! could be getting the help text for each of these...
                for i in sorted(self.fields_for_current_submode_starting_with()):
                    if not i.startswith(text):
                        continue
                    count += 1
                    if field_info and field_info.get('help_text', None):
                        f_ret += "  %s%s%s\n" % (i,
                                          ' ' * (longest_field - len(i) + 1),
                                          field_info.get('help_text'))
                    else:
                        f_ret += "  %s\n"% i

            if (text == "" or count > 1) and s_ret != "":
                ret += "Commands:\n"
                ret += s_ret

            if (text == "" or count > 1) and c_ret != "":
                ret += "All Available commands:\n"
                ret += c_ret

            if (text == "" or count > 1) and o_ret != "":
                ret += "\nAvailable config submodes:\n"
                ret += o_ret
            
            if (text == "" or count > 1) and f_ret != "":
                ret += "\nAvailable fields for %s:\n" % self.get_current_mode_obj_type() 
                ret += f_ret
        elif words[0] in ["show", "help", "copy", "watch" ]:
            method = self.command_method_from_name(words[0])
            if method:
                ret = method(None)
            else:
                #try:
                    #ret = command.get_command_syntax_help(words, 'Command syntax:')
                #except:
                    #if self.debug or self.debug_backtrace:
                        #traceback.print_exc()
                    #ret = "No help available for command %s" % words[0]

                try:
                    ret = command.get_command_documentation(words)
                except:
                    if self.debug or self.debug_backtrace:
                        traceback.print_exc()
                    ret = "No help available for command %s" % words[0]
        else:
            #try:
                #ret = command.get_command_syntax_help(words, 'Command syntax:')
            #except:
                #if self.debug or self.debug_backtrace:
                    #traceback.print_exc()
                #ret = "No help available for command %s" % words[0]
            try:
                ret = command.get_command_documentation(words)
            except:
                if self.debug or self.debug_backtrace:
                    traceback.print_exc()
                ret = "No help available for command %s" % words[0]
        return ret

    #
    # --------------------------------------------------------------------------------
    # do_help
    #
    def do_help(self, words):
        return self.help_splash(words, "")
        
    #
    # --------------------------------------------------------------------------------
    # cp_history
    #
    def cp_history(self, words, text, completion_char):
        if len(words) == 1:
            ret_val = "  <num>      - to display a specific number of commands (default:all)\n"
            ret_val += "  <cr>\n"
            self.print_completion_help(ret_val)
        else:
            self.print_completion_help("<cr>")

    #
    # --------------------------------------------------------------------------------
    # do_history
    #
    def do_history(self, words = None):
        ret_val = ""
        how_many = num_commands = readline.get_current_history_length()
        if words:
            how_many = words[0]
        for i in range(num_commands-int(how_many) + 1, num_commands):
            yield "%s: %s\n" % (i, readline.get_history_item(i))
        return


    #
    # generic parsing routines
    # 

    #
    # --------------------------------------------------------------------------------
    # completion_reset
    #
    def completion_reset(self):
        self.last_line = None
        self.last_options = None
        self.completion_cache = True
        self.last_completion_char = readline.get_completion_type()


    #
    # --------------------------------------------------------------------------------
    # completer
    #  This is the main function that is called in order to complete user input
    #
    def completer(self, text, state):
        question_mark = ord('?')
        if readline.get_completion_type() == question_mark:
            if len(readline.get_line_buffer()) == 0:
                #
                # manage printing of help text during command completion
                help_text = self.help_splash(None, text)
                if help_text != "":
                    self.print_completion_help(help_text)
                    return
        
        try:
            origline = readline.get_line_buffer()
            # See if we have a cached reply already
            if (self.completion_cache and origline == self.last_line and 
                self.last_completion_char == readline.get_completion_type() and
                self.last_options):

                if state < len(self.last_options):
                    return self.last_options[state]
                else:
                    # apparently, for the linux VM choice don't print 
                    if self.last_options and \
                      len(self.last_options) > 1 and \
                      self.last_completion_char == ord('\t'):
                        choices_text = self.choices_text_builder(self.last_options)
                        self.print_completion_help(choices_text)

                    if self.completion_skip:
                        self.completion_cache = False
                        self.completion_skip = False
                    return None

            self.completion_reset()

            # parse what user has typed so far

            begin = readline.get_begidx()
            end = readline.get_endidx()

            # Find which command we're in for a semicolon-separated list of single commands
            # LOOK! This doesn't handle cases where an earlier command in the line changed
            # the mode so the completion for later commands in the line should be different.
            # For example, if you typed "enable; conf" it won't detect that it should be
            # able to complete "conf" to "configure" because the enable command has not been
            # executed yet, so you're not in enable mode yet. Handling that case would be
            # non-trivial I think, at least with the current CLI framework.
            command_begin = 0
            command_end = 0
            while True:
                command_end = self.find_with_quoting(origline, ';', start_index=command_begin)
                if command_end < 0:
                    command_end = len(origline)
                    break
                if begin >= command_begin and end <= command_end:
                    break
                command_begin = command_end + 1
            
            # Skip past any leading whitespace in the command
            while command_begin < begin and origline[command_begin].isspace():
                command_begin += 1
            
            words = origline[command_begin:end].split()
            
            # remove last term if it is the one being matched
            if begin != end:
                words.pop()

            # LOOK! there are at least three places that try to parse the valid options:
            # 1. When actually handling a command
            # 2. When trying to show completions (here)
            # 3. When displaying help

            # complete the first word in a command line            
            if not words or begin == command_begin:
                options = self.commands_for_current_mode_starting_with(text, completion = True)
                if self.in_config_submode():
                    for item in self.fields_for_current_submode_starting_with(text):
                        self.append_when_missing(options, item)
                options = [x if x.endswith(' ') else x + ' '  for x in sorted(options)]
            # Complete the 2nd word or later
            else:
                commands = self.commands_for_current_mode_starting_with(words[0])
                obj_types = []
                fields = self.fields_for_current_submode_starting_with(words[0]) if self.in_config_submode() else []
                if len(commands) + len(obj_types) + len(fields) > 1:
                    if len(fields) > 1:
                        fields = [words[0]] if words[0] in fields else []
                    if len(commands) > 1:
                        commands = [words[0]] if words[0] in commands else []
                    if len(obj_types) > 1:
                        obj_types = [words[0]] if words[0] in obj_types else []
                
                if len(fields) == 1:
                    options = self.cp_conf_field(self.get_current_mode_obj_type(), words, text)
                elif len(obj_types) == 1:
                    options = self.cp_conf_object_type(obj_types + words[1:], text)
                elif len(commands) == 1: 
                    try:
                        # options[0] is expanded while words[0] is not
                        method = self.completion_method_from_name(commands[0]) 
                        if method:
                            options = method(words, text, readline.get_completion_type())
                            if not options:
                                # no match
                                return None
                        else:
                            if readline.get_completion_type() == question_mark:
                                options = command.do_command_completion_help(words, text)
                            else:
                                options = command.do_command_completion(words, text)
                            #else:
                                #if options:
                                    #print syntax_help
                                #else:
                                    #pass
                                    #self.print_completion_help(syntax_help)
                            
                    except AttributeError:
                        if self.debug or self.debug_backtrace:
                            traceback.print_exc()
                        return None

                else:
                    options = None
                    
        except Exception, e:
            if self.debug or self.debug_backtrace:
                traceback.print_exc()
            # errors in connect are caught silently, complain here
            # TODO - Maybe we should log this in a file we can review
            # at a later date?
        
        try:
            if options:
                self.last_line = origline
                self.last_options = options
                self.last_completion_char = readline.get_completion_type()
                return options[state]
        except IndexError:
            return None

        return None

    #
    # --------------------------------------------------------------------------------
    # print_completion_help
    #
    def print_completion_help(self, completion_help_text):
        origline = readline.get_line_buffer()
        end = readline.get_endidx()
        cur_command = origline[0:end]

        help_text = "\n%s\n%s%s" % ( completion_help_text, 
                                    self.prompt, 
                                    cur_command)
        self.completion_skip = True
        sys.stdout.write(help_text)
        
    #
    # --------------------------------------------------------------------------------
    # implement_write
    #
    def implement_write(self, words):
        if len(words) == 1:
            if "memory".startswith(words[0]):
                return self.implement_copy(["running-config", "startup-config"])
            elif "erase".startswith(words[0]):
                print "This will clear the startup-config and set it to be empty."
                resp = raw_input("Are you sure that want to proceed? [n]")
                if resp and "yes".startswith(resp.lower()):
                    print "Erasing startup config ..."
                    result = self.store.delete_user_data_file("startup-config/time/len/version")
                    if 'status' in result and result['status'] == 'success':
                        return None
                    elif 'message' not in result:
                        return self.error_msg("rest store result doesn't contain error message")
                    else:
                        return self.error_msg(result['message'])
                else:
                    print "Command aborted by user: write erase"
                return
            elif "terminal".startswith(words[0]):
                return self.implement_copy(["running-config", "terminal"])
        return "Syntax: write < terminal | memory | erase>"

    #
    # --------------------------------------------------------------------------------
    # 
    
    @staticmethod
    def set_clock_action(data):
        time_values = data['time'].split(':')
        if len(time_values) != 3:
            raise error.CommandError('Invalid time; must be HH:MM:SS')
        hour = int(time_values[0])
        minute = int(time_values[1])
        second = int(time_values[2])
        
        MONTH_NAMES =  ('January', 'February', 'March', 'April', 'May', 'June',
                        'July', 'August', 'September', 'October', 'November', 'December')

        day_of_month = int(data['day-of-month'])
        month_name = data['month']
        if month_name not in MONTH_NAMES:
            raise error.CommandError('Invalid month name (e.g. January, May, July)')
        month = MONTH_NAMES.index(month_name) + 1
        year = int(data['year'])
        
        date_time_info = {
            'year': year,
            'month': month,
            'day': day_of_month,
            'hour': hour,
            'minute': minute,
            'second': second
        }
        url = 'http://%s/rest/v1/system/clock/local' % cli.controller
        result = cli.store.rest_post_request(url, date_time_info)
        date_time_info = json.loads(result)
        clock_string = BigSh.get_clock_string(date_time_info, False)
        return clock_string
    
    # --------------------------------------------------------------------------------
    # handle_command
    #
    def handle_command(self, command_word, words):
        if type(command_word) == str:
            method = self.command_method_from_name(command_word)
            if method:
                return method(words)
        # XXX It would be better to only call do_command if it
        # was clear that this command actually existed.
        return command.do_command([command_word] + words)


    #
    # --------------------------------------------------------------------------------
    # find_with_quoting
    #
    #  Assumes start_index is not inside a quoted string.
    #
    @staticmethod
    def find_with_quoting(line, find_char, reverse=False, start_index=0):
        in_quoted_arg = False
        line_length = len(line)
        i = start_index
        found_index = -1;
        while i < line_length:
            c = line[i]
            if c in "\"'":
                if not in_quoted_arg:
                    quote_char = c
                    in_quoted_arg = True
                elif c == quote_char:
                    in_quoted_arg = False
                # otherwise leave in_quoted_arg True
            elif c == "\\" and in_quoted_arg:
                i += 1
            elif (c == find_char) and not in_quoted_arg:
                found_index = i
                if not reverse:
                    break
            i += 1
            
        return found_index
    
    #
    # --------------------------------------------------------------------------------
    # split_with_quoting
    #
    @staticmethod
    def split_with_quoting(line, separators=" \t"):
        word_list = []
        current_word = ""
        in_quoted_arg = False
        line_length = len(line)
        i = 0
        while i < line_length:
            c = line[i]
            i += 1
            if c in "\"'":
                if not in_quoted_arg:
                    in_quoted_arg = True
                    quote_char = c
                elif c == quote_char:
                    in_quoted_arg = False
                    word_list.append(current_word)
                    current_word = ""
                else:
                    current_word += c
            elif c == "\\" and in_quoted_arg:
                if i < line_length:
                    c = line[i]
                    current_word += c
                    i += 1
            elif (c in separators) and not in_quoted_arg:
                if current_word:
                    word_list.append(current_word)
                    current_word = ""
            else:
                current_word += c
                
        if current_word:
            word_list.append(current_word)
        
        return word_list
    
    #
    # --------------------------------------------------------------------------------
    # quote_item
    #  Some of the new model columns available as choices to select have the '|' 
    #  character as a separator.  For these choices to word, they need to be
    #  quoted
    #
    @staticmethod
    def quote_item(obj_type, item):
        if item.find("|") >= 0:
            return  '"' + str(item) + '"' 
        else:
            return str(item)

    #
    # --------------------------------------------------------------------------------
    # handle_single_line
    #
    def handle_single_line(self, line):
        ret_val = None
        if len(line) > 0 and line[0]=="!": # skip comments
            return
        words = self.split_with_quoting(line)
        if not words:
            return
        #
        self.completion_reset()

        # Look for the replay keyword, use the first two tokens if the replay
        # keyword is in the first part of the command.
        if self.debug and len(words) >= 2:
            if words[0] == 'replay':
                # replay the file, remove the first two keywords
                self.replay(words[1], command_replay = len(words) == 2)
                if len(words) == 2:
                    return
                words = words[2:]

        # the first word of a line is either:
        # - a command - dependent on mode (show anywhere but configure only in enable)
        # - an object type - if we're in a config mode (either config or config submode)
        # - a field for an object - if we're in a config submode

        matches = [(x, "command") for x in self.commands_for_current_mode_starting_with(words[0])]
        # LOOK!: robv Fix to work with field names where one name is a prefix of another

        if len(matches) > 1:
            for match_tuple in matches:
                if match_tuple[0] == words[0]:
                    matches = [match_tuple]
                    break
        if len(matches) == 1:
            match = matches[0]
            # Replace the (possibly) abbreviated argument with the full name.
            # This is so that the handlers don't need to all handle abbreviations.
            if type(match[0]) == str:
                words[0] = match[0]

            if match[1] == "field":
                ret_val = self.handle_field(words)
            elif match[1] == "config object":
                ret_val = self.handle_obj_type(words)
            else:
                ret_val = self.handle_command(words[0], words[1:])
                #ret_val = self.handle_command(match[0], words[1:])
        elif len(matches) > 1:
            ret_val = self.error_msg("%s is ambiguous\n" % words[0])
            for m in matches:
                ret_val += "%s (%s)\n" % m
        else:
            ret_val = self.error_msg("Unknown command: %s\n" % words[0])

        url_cache.command_finished(words)
        return ret_val

    #
    # --------------------------------------------------------------------------------
    # generate_pipe_output
    #
    def generate_pipe_output(self, p, output):
        fl = fcntl.fcntl(p.stdout, fcntl.F_GETFL)
        fcntl.fcntl(p.stdout, fcntl.F_SETFL, fl | os.O_NONBLOCK)

        for item in output:
            try:
                p.stdin.write(item)
            except IOError:
                break

            try:
                out_item = p.stdout.read()
                yield out_item
            except IOError:
                pass
            
        p.stdin.close()

        fcntl.fcntl(p.stdout, fcntl.F_SETFL, fl)
        while True:
            out_item = p.stdout.read()
            if (out_item):
                yield out_item
            else:
                p.stdout.close()
                break
        p.wait()

    #
    # --------------------------------------------------------------------------------
    # write_to_pipe
    #
    def write_to_pipe(self, p, output):
        for item in output:
            try:
                p.stdin.write(item)
            except IOError:
                break
        p.stdin.close()
        p.wait()
            
    #
    # --------------------------------------------------------------------------------
    # shell_escape
    #  Return a string, quoting the complete string, and correctly prefix any
    #  quotes within the string.
    #
    def shell_escape(self, arg):
        return "'" + arg.replace("'", "'\\''") + "'"

    #
    # --------------------------------------------------------------------------------
    # handle_pipe_and_redirect
    #
    def handle_pipe_and_redirect(self, pipe_cmds, redirect_target, output):
        # if redirect target is tftp/ftp/http/file, then we should actually stick
        # curl at the end of the pipe_cmds so it gets handled below
        if redirect_target:
            if redirect_target.startswith("tftp") or redirect_target.startswith("ftp") or \
               redirect_target.startswith("http") or redirect_target.startswith("file"):
                redirect_target = self.shell_escape(redirect_target)
                # add so it can be used below
                if pipe_cmds == None:
                    pipe_cmds = ""
                else:
                    pipe_cmds += " | "
                pipe_cmds += " curl -X PUT -d @- %s" % redirect_target

        if pipe_cmds:
            new_pipe_cmd_list = []
            for pipe_cmd in [x.strip() for x in pipe_cmds.split('|')]:
                # doing it this way let us handles spaces in the patterns
                # as opposed to using split/join which would compress space
                new_pipe_cmd = pipe_cmd
                m = re.search('^(\w+)(.*)$', pipe_cmd) 
                if m: 
                    first_tok = m.group(1)
                    rest_of_cmd = m.group(2).strip()
                    if first_tok.startswith("in"):
                        new_pipe_cmd = "grep -e " + rest_of_cmd
                    elif first_tok.startswith("ex"):
                        new_pipe_cmd = "grep -v -e" + rest_of_cmd
                    elif first_tok.startswith("begin"):
                        new_pipe_cmd =  "awk '/%s/,0'" % rest_of_cmd
                new_pipe_cmd_list.append(new_pipe_cmd)

            new_pipe_cmds = "|".join(new_pipe_cmd_list)
            if new_pipe_cmds:
                if redirect_target:
                    p = subprocess.Popen(new_pipe_cmds, 
                                         shell=True, 
                                         stdin=subprocess.PIPE, 
                                         stdout=subprocess.PIPE,
                                         stderr=subprocess.STDOUT)
                    output = self.generate_pipe_output(p, output)
                else:
                    p = subprocess.Popen(new_pipe_cmds, 
                                         shell=True, 
                                         stdin=subprocess.PIPE)
                    self.write_to_pipe(p, output)
                    output = None

        # only handle local file here as http/ftp were handled above via pipe
        if redirect_target:
            if redirect_target.startswith("config://"):
                m = re.search(self.local_name_pattern, redirect_target)
                if m:
                    join_output = ''.join(iter(output))
                    store_result = self.store.set_user_data_file(m.group(1), join_output)
                    if store_result:
                        result = json.loads(store_result)
                    else:
                        return self.error_msg("rest store result not json format")
                    if 'status' in result and result['status'] == 'success':
                        return None
                    elif 'message' not in result:
                        return self.error_msg("rest store result doesn't contain error message")
                    else:
                        return self.error_msg(result['message'])
                else:
                    print self.error_msg("invalid name-in-db (%s)\n" % redirect_target)
            else:
                return output

        return None

    #
    # --------------------------------------------------------------------------------
    # generate_command_output
    #
    @staticmethod
    def generate_command_output(ret_val):
        if (isinstance(ret_val, str) or \
            isinstance(ret_val, buffer) or \
            isinstance(ret_val, bytearray) or \
            isinstance(ret_val, unicode)):

            # yield ret_val
            if len(ret_val) and ret_val[-1] == '\n':
                ret_val = ret_val[:-1]
            for line in ret_val.split('\n'):
                yield line + '\n'
        elif ret_val != None:
            for item in ret_val:
                yield item

    #
    # --------------------------------------------------------------------------------
    # generate_line_output
    #
    # This is a generator that will generate the output of the 
    # command either as a string, or by iterating over a returned
    # iterable.  This way, a subcommand can return an iterable to 
    # lazily evaluate large amounts of output
    #
    def generate_line_output(self, line, dont_ask):
        while line:
            subline_index = self.find_with_quoting(line, ';')
            if subline_index < 0:
                subline_index = len(line)
            subline = line[:subline_index]
            line = line[subline_index+1:]
            ret_val = self.handle_single_line(subline)
            cnt = 1
            total_cnt = 0

            (col_width, screen_length) = self.pp.get_terminal_size()
            if type(self.length) == int:
                screen_length = self.length

            for item in self.generate_command_output(ret_val):
                if not dont_ask:
                    incr = 1 + (max((len(item.rstrip()) - 1), 0) / col_width)
                    if screen_length and cnt + incr >= screen_length:
                        raw_input('-- hit return to continue, %s) --' % total_cnt)
                        cnt = 0
                    cnt += incr
                    total_cnt += incr
                yield item

    #
    # --------------------------------------------------------------------------------
    # handle_multipart_line
    #
    # this is the outermost handler that should print
    #
    def handle_multipart_line(self, line):
        pipe_cmds = None
        redirect_target = None
        output = None

        # pattern is:
        # single line [; single line]* [| ...] [> {conf|ftp|http}]

        # first take off the potential redirect part then the pipe cmds
        redirect_index = self.find_with_quoting(line, '>', True)
        if redirect_index >= 0:
            redirect_target = line[redirect_index+1:].strip()
            line = line[:redirect_index].strip()
#        pipe_index = self.find_with_quoting(line, '|')
#        if pipe_index >= 0:
#            pipe_cmds = line[pipe_index+1:].strip()
#            line = line[:pipe_index].strip()
        # remaining text is single lines separated by ';' - handle them
        output = self.generate_line_output(line, pipe_cmds or redirect_target)

        # now output everything
        if pipe_cmds or redirect_target:
            output = self.handle_pipe_and_redirect(pipe_cmds, redirect_target, output)

        if output != None:
            for line in output:
                print line.rstrip()

    #
    #
    # --------------------------------------------------------------------------------
    # loop
    #  this is just dispatching the command and handling errors
    #
    def loop(self):
        command.action_invoke('implement-show-version', ({},))

        while self.run:
            # Get command line - this will use the command completion above
            try:
                line = raw_input(self.prompt)
                if self.batch:
                    print line
                self.handle_multipart_line(line)
            except KeyboardInterrupt:
                self.completion_reset()
                print "\nInterrupt."
            except EOFError:
                print "\nExiting."
                return
            except Exception, e:
                print "\nError running command '%s'.\n" % line
                if self.debug or self.debug_backtrace:
                    traceback.print_exc()


#
# --------------------------------------------------------------------------------
# Initialization crazyness to make it work across platforms. Many platforms don't include
# GNU readline (e.g. mac os x) and we need to compensate for this

try:
    import readline
except ImportError:
    try:
        import pyreadline as readline
    except ImportError:
        print "Can't find any readline equivalent - aborting."
else:
    if 'libedit' in readline.__doc__:
        # needed for Mac, please fix Apple
        readline.parse_and_bind ("bind ^I rl_complete")
    else:
        readline.parse_and_bind("tab: complete")
        readline.parse_and_bind("?: possible-completions")

  
#
# --------------------------------------------------------------------------------
# Run the shell

def main():
    # Try to handle sftp/scp client connections
    if not os.isatty(sys.stdout.fileno()) and ("--init" not in sys.argv):
            rcmd = "/usr/bin/rssh %s" % (" ".join(sys.argv[1:]))
            p = subprocess.Popen(rcmd.split(), stdout=sys.stdout, stdin=sys.stdin, env=os.environ)
            p.wait()
            sys.exit()

    global cli
    # Uncomment the next two lines to enable remote debugging with PyDev
    # LOOK! Should have logic here that enables/disables the pydevd stuff
    # automatically without requiring uncommenting (and, more importantly,
    # remembering to recomment before committing).
    # (e.g. checking environment variable or something like that)
    #python_path = os.environ.get('PYTHONPATH', '')
    #if 'pydev.debug' in python_path:
    try:
        import pydevd
        pydevd.settrace()
    except Exception, e:
        pass

    loading_startup = "--init" in sys.argv
    startup_loaded_file = "/run/startup-config-loaded"
    if not loading_startup:
        if not os.path.exists(startup_loaded_file):
            try:
                sys.stdout.write("Waiting for startup-config to finish loading")
                while not os.path.exists(startup_loaded_file):
                    sys.stdout.write(".")
                    sys.stdout.flush()
                    time.sleep(2)
                sys.stdout.write("\n")
            except KeyboardInterrupt:
                sys.stdout.write("\nNOTE: startup-config not loaded!  Some commands "
                                 "may cause inconsistent results.\n")

    # Start CLI
    cli = MainSh()
    cli.init()
    cli.loop()

    if loading_startup:
        file(startup_loaded_file, "w").write("")

if __name__ == '__main__':
    main()
