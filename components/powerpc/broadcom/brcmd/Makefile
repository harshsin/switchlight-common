SUBDIRS := $(filter-out Makefile, $(shell ls))

deb:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir deb; done

all-clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done



