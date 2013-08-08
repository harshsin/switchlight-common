DEBUILD = debuild --prepend-path=/usr/lib/ccache -b -us -uc

all:
	cd build; $(DEBUILD)

clean:
	cd build; $(DEBUILD) -Tclean
	rm -f *.deb *.changes *.build

dch:
	cd build; EMAIL="$(USER)@bigswitch.com" dch -i
