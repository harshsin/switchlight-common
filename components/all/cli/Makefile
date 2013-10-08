ARCH = powerpc
DEBUILD = debuild --prepend-path=/usr/lib/ccache -eSWITCHLIGHT -a$(ARCH) -b -us -uc
PACKAGE_DIR := $(SWITCHLIGHT)/debian/repo

all:
	ln -snf ../debuild/debian src/debian
	cd src; $(DEBUILD)
	$(SWITCHLIGHT)/tools/spkg.py --add-pkg *.deb

clean:
	ln -snf ../build/debian src/debian
	cd src; $(DEBUILD) -Tclean
	rm -f *.deb *.changes *.build src/debian

dch:
	cd build; EMAIL="$(USER)@bigswitch.com" dch -i

deb: all
