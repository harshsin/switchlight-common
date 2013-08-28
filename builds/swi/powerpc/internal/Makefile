ifndef SWITCHLIGHT
$(error $$SWITCHLIGHT is undefined.)
endif

# 
# These are the local packages that must be build
# for this SWI configuration. 
#
SWITCHLIGHT_REQUIRED_PACKAGES := kernel-85xx:powerpc \
				 kernel-e500mc:powerpc \
				 initrd-powerpc:powerpc \
				 pcli:all \
				 faultd:powerpc \
				 sofcon:powerpc \
				 ofad-5.10.0-internal:powerpc \
				 brcmd-5.10.0-internal:powerpc \
				 ofad-5.6.6-internal:powerpc \
				 brcmd-5.6.6-internal:powerpc \
				 ofad-6.2.7-internal:powerpc \
				 brcmd-6.2.7-internal:powerpc 

include $(SWITCHLIGHT)/make/component.mk


