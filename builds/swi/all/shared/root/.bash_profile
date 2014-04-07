############################################################
#
# If the firmware environment contains a variable called
# $sl_bash_prompt_prefix then the value of this variable is used
# as the prefix for the system prompt when when logged in
# with bash as root.
#
# This is mostly a debug feature for identifying physical
# boxes during development and debug.
#
############################################################
prefix=`fw_printenv | grep sl_bash_prompt_prefix= | tr "=" " " | awk '{print $2}'`
if [ -n ${prefix} ]; then
    PS1="${prefix}${PS1}"
fi

