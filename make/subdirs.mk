############################################################
# <bsn.cl fy=2014 v=none>
# </bsn.cl>
############################################################
DIRECTORIES := $(notdir $(wildcard $(CURDIR)/*))
FILTER := make Makefile Makefile~
DIRECTORIES := $(filter-out $(FILTER),$(DIRECTORIES))

$(MAKECMDGOALS):
	$(foreach d,$(DIRECTORIES),$(MAKE) -C $(d) $(MAKECMDGOALS) || exit 1;)




