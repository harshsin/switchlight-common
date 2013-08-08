
class CommandError(Exception):
    """
    Base class for exceptions thrown by the CLI command module
    """
    def __init__(self, kind, message = None):
        if kind:
            message = kind + ': ' + message
        super(CommandError, self).__init__(message)

class ArgumentValidationError(CommandError):
    def __init__(self, message=None, expected_tokens=None):
        kind = "Invalid argument"
        super(ArgumentValidationError, self).__init__(kind, message)
        self.expected_tokens = expected_tokens


class CommandSyntaxError(CommandError):
    def __init__(self, message):
        #kind = 'Error'
        kind = None
        super(CommandSyntaxError,self).__init__(kind, message)


class RangeSyntaxError(CommandError):
    def __init__(self, message):
        kind = "Value outside length/range"
        super(RangeSyntaxError,self).__init__(kind, message)


class CommandDescriptionError(CommandError):
    def __init__(self, message, command = None):
        self.command = None
        kind = "Bad command description"
        if command:
            self.command = command
            message += ': ' + command['self']
        super(CommandDescriptionError,self).__init__(kind, message)
    

class CommandCompletionError(CommandError):
    def __init__(self, message):
        kind = "Command completion"
        super(CommandCompletionError,self).__init__(kind, message)


class CommandAmbiguousError(CommandError):
    def __init__(self, message):
        kind = "Ambiguous command"
        super(CommandAmbiguousError,self).__init__(kind, message)


class CommandMissingError(CommandError):
    def __init__(self, message):
        kind = "No such command"
        super(CommandMissingError,self).__init__(kind, message)


class CommandInvocationError(CommandError):
    def __init__(self, message):
        kind = "Invalid command invocation"
        super(CommandInvocationError,self).__init__(kind, message)


class CommandSemanticError(CommandError):
    def __init__(self, message):
        kind = "Invalid Use"
        super(CommandSemanticError,self).__init__(kind, message)


class CommandInternalError(CommandError):
    def __init__(self, message):
        kind = "Internal (bug)"
        super(CommandInternalError,self).__init__(kind, message)


class CommandRestError(CommandError):
    def __init__(self, rest_info=None, message=None):
        s = 'Error: REST API'
        if rest_info:
            error_type = rest_info.get('error_type')
            if error_type:
                s += '; type = ' + error_type
            description = rest_info.get('description')
            if description:
                s += '; ' + description
        if message:
            s += ': ' + message
        super(CommandRestError,self).__init__("REST", s)
        

class ActionError(CommandError):
    def __init__(self, message):
        kind = None
        super(ActionError,self).__init__(kind, message)


