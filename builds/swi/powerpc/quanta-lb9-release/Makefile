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
				 ofad-6.2.7-release:powerpc 

include $(SWITCHLIGHT)/make/component.mk


