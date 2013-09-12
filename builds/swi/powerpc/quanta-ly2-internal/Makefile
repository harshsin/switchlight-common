ifndef SWITCHLIGHT
$(error $$SWITCHLIGHT is undefined.)
endif

# 
# These are the local packages that must be build
# for this SWI configuration. 
#
SWITCHLIGHT_REQUIRED_PACKAGES := kernel-85xx:powerpc \
				 initrd-powerpc:powerpc \
				 pcli:all \
				 faultd:powerpc \
				 sofcon:powerpc \
				 ofad-5.10.0-internal:powerpc \
				 brcmd-5.10.0-internal:powerpc 

include $(SWITCHLIGHT)/make/component.mk


