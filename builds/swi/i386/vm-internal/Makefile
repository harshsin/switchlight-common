ifndef SWITCHLIGHT
$(error $$SWITCHLIGHT is undefined.)
endif

# 
# These are the local packages that must be build
# for this SWI configuration. 
#
SWITCHLIGHT_REQUIRED_PACKAGES := kernel-x86-64:amd64 \
				 initrd-i386:i386 \
				 pcli:all 

include $(SWITCHLIGHT)/make/component.mk


