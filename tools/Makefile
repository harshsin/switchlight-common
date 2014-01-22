############################################################
# <bsn.cl fy=2013 v=none>
# 
#        Copyright 2013, 2014 BigSwitch Networks, Inc.        
# 
# 
# 
# </bsn.cl>
############################################################
#
# Builds a debian package for the SwitchLight tools.
#
############################################################

DEBUILD = debuild --prepend-path=/usr/lib/ccache -b -us -uc

all:
	cd build; $(DEBUILD)

clean:
	cd build; $(DEBUILD) -Tclean
	rm -f *.deb *.changes *.build

dch:
	cd build; EMAIL="$(USER)@bigswitch.com" dch -i
