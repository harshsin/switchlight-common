ifndef SWITCHLIGHT
$(error $$SWITCHLIGHT is undefined.)
endif
ifndef ARCH
$(error $$ARCH is undefined.)
endif

SWITCHLIGHT_REQUIRED_SUBMODULES := onlp
SWITCHLIGHT_REQUIRED_PACKAGES := onlp-platform:$(ARCH) onlp-platform-defaults:$(ARCH)

include $(SWITCHLIGHT)/make/component.mk
