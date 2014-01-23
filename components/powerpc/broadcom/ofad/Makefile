############################################################
# <bsn.cl fy=2013 v=none>
# 
#        Copyright 2013, 2014 BigSwitch Networks, Inc.        
# 
# 
# 
# </bsn.cl>
############################################################
SUBDIRS := $(filter-out Makefile, $(shell ls))

deb:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir deb; done

all-clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done



