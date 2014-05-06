############################################################
#
# Generate version targets.
#
############################################################
include $(SWITCHLIGHT)/make/config.mk


%.slv: FORCE
	echo "SWITCHLIGHT_BUILD_SHA1=$(SWITCHLIGHT_BUILD_SHA1)" > $@
	echo "SWITCHLIGHT_BUILD_TIMESTAMP=$(SWITCHLIGHT_BUILD_TIMESTAMP)" >> $@
	echo "SWITCHLIGHT_RELEASE_VERSION=$(SWITCHLIGHT_RELEASE_VERSION)" >> $@
	echo "SWITCHLIGHT_RELEASE=\"$(RELEASE)\"" >> $@


FORCE::


