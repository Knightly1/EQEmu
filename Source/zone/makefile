
CC=gcc
LINKER=gcc
MYSQL_FLAGS=-I/usr/include/mysql
MYSQL_LIB=-L/usr/include/mysql -lmysqlclient -lz -lcrypt -lnsl -lm -lc -lnss_files -lnss_dns -lresolv -lc -lnss_files -lnss_dns -lresolv
DFLAGS=-DEQDEBUG=5 -DCATCH_CRASH -DNO_PIDLOG -DSHAREMEM -DSPELL_EFFECT_SPAM -DFIELD_ITEMS -DCOMBINED -DAPP_OPCODE_SIZE=2
#try commenting out the following three lines to disable embedded perl
PERL_FLAGS=perl -MExtUtils::Embed -e ccopts
PERL_LIB=perl -MExtUtils::Embed -e ldopts
DFLAGS+=-DEMBPERL -DEMBPERL_PLUGIN -DHAS_UNION_SEMUN
WFLAGS=-Wall -Wuninitialized -Wwrite-strings -Wcast-qual  -Wstrict-prototypes -Wno-deprecated  -Wcomment -Wcast-align
COPTS=$(WFLAGS) -O -g -pg -march=i686 -pthread -pipe -D_GNU_SOURCE -DINVERSEXY -DFX -DZONE $(DFLAGS) $(MYSQL_FLAGS)
LINKOPTS=-rdynamic -L. -lstdc++ -ldl $(MYSQL_LIB)

include makefile.common
