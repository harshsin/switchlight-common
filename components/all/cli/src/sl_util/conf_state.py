# Copyright (c) 2014  Big Switch Networks

# This module is used for registering hooks for saving and reverting submodule
# states.
#
# FIXME: we might need to register the submodule hooks with ordering if saving
# or reverting of submodule states need to happen in a particular sequence.

save_registry = []
revert_registry = []

def register_save(name, proc):
    global save_registry
    save_registry.append((name, proc))

def get_save_registry():
    return save_registry

def register_revert(name, proc):
    global revert_registry
    revert_registry.append((name, proc))

def get_revert_registry():
    return revert_registry
