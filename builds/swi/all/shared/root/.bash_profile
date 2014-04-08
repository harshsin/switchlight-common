############################################################
#
# The following firmware environment variables can be
# used to modify the bash system prompt when the user
# logs in as root.
#
# If $sl_bash_prompt_prefix or $onl_bash_prompt_prefix is
# set then the value of that variable is prepended
# to the default $PS1 prompt.
#
# This affects only the login prompt for the root user.
#
# The purpose of this feature is to allow easy identification
# of the physical system you are currently working on
# regardless of the identification settings provided by
# default or through the current startup-config
# (such as the current hostname).
#
############################################################
#
# Check for $sl_bash_prompt_prefix first
#
# Note -- grep is used instead of "fw_printenv sl_bash_prompt_prefix"
# to avoid fw_printenv's output to stderr when the variable
# is not defined.
#
prefix=`fw_printenv | grep sl_bash_prompt_prefix= | tr "=" " " | awk '{print $2}'`

if [ -z ${prefix} ]; then
    # Check for $onl_bash_prompt_prefix if $sl_bash_prompt_prefix is not set
    prefix=`fw_printenv | grep onl_bash_prompt_prefix= | tr "=" " " | awk '{print $2}'`
fi

if [ -n ${prefix} ]; then
    PS1="${prefix} ${PS1}"
fi




