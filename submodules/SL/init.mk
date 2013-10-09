#
# The root of of our repository is here:
#
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

#
# Resolve submodule dependencies. 
#
ifndef SUBMODULES
SUBMODULES := $(ROOT)/..
endif

ifndef SUBMODULE_INFRA
SUBMODULE_INFRA := $(SUBMODULES)/infra
endif

ifndef SUBMODULE_INDIGO
SUBMODULE_INDIGO := $(SUBMODULES)/indigo
endif

ifndef SUBMODULE_BIGCODE
SUBMODULE_BIGCODE := $(SUBMODULES)/bigcode
endif

ifndef SUBMODULE_BIGCODE_INTERNAL
SUBMODULE_BIGCODE_INTERNAL := $(SUBMODULES)/bigcode-internal
endif

export SUBMODULE_INFRA
export BUILDER := $(SUBMODULE_INFRA)/builder/unix

MODULE_DIRS := $(ROOT)/modules $(SUBMODULE_INFRA)/modules $(SUBMODULE_BIGCODE)/modules $(SUBMODULE_INDIGO)/modules $(SUBMODULE_BIGCODE_INTERNAL)/modules















