#simple root level makefile by Father Nitwit

all:
	$(MAKE) -C EMuShareMem
	$(MAKE) -C world
	$(MAKE) -C zone
	$(MAKE) -C eqlaunch
	$(MAKE) -C utils
	$(MAKE) -C eqemucollector
	$(MAKE) -C eqextractor

vc2003:
	$(MAKE) -C EMuShareMem -f makefile.vc2003
	$(MAKE) -C world -f makefile.vc2003
	$(MAKE) -C zone -f makefile.vc2003
	$(MAKE) -C eqlaunch -f makefile.vc2003

wine:
	$(MAKE) -C EMuShareMem -f makefile.wine
	$(MAKE) -C world -f makefile.wine
	$(MAKE) -C zone -f makefile.wine
	$(MAKE) -C eqlaunch -f makefile.wine

wine_clean:
	$(MAKE) -C EMuShareMem -f makefile.wine clean
	$(MAKE) -C world -f makefile.wine clean
	$(MAKE) -C zone -f makefile.wine clean
	$(MAKE) -C eqlaunch -f makefile.wine clean

clean:
	$(MAKE) -C EMuShareMem clean
	$(MAKE) -C world clean
	$(MAKE) -C zone clean
	$(MAKE) -C utils clean
	$(MAKE) -C eqlaunch clean
	$(MAKE) -C eqemucollector clean
	$(MAKE) -C eqextractor clean

