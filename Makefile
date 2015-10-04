include Makefile.inc

.PHONY: src

BINDIR = bin

BINS = cbftp $(BINDIR)/cbftp-debug $(BINDIR)/datafilecat $(BINDIR)/datafilewrite

SRC = $(wildcard src/*.cpp src/ui/*.cpp src/ui/screens/*.cpp)

OBJS = $(wildcard $(SRC:%.cpp=%.o))

all: ${BINS}

$(BINDIR):
	mkdir -p $@

src:
	@+${MAKE} -C src
	
$(BINDIR)/cbftp: $(OBJS)
	g++ -o $(BINDIR)/cbftp $(FINALFLAGS) $(SRC:%.cpp=%.o) $(LINKFLAGS)
	
cbftp: src | $(BINDIR)
	@+${MAKE} --no-print-directory $(BINDIR)/cbftp
	
$(BINDIR)/cbftp-debug: misc/start_with_gdb.sh | $(BINDIR)
	cp misc/start_with_gdb.sh $@; chmod +x bin/cbftp-debug

$(BINDIR)/datafilecat: src/crypto.cpp src/tools/datafilecat.cpp | $(BINDIR)
	g++ -o $@ ${FINALFLAGS} src/crypto.cpp src/tools/datafilecat.cpp -lcrypto

$(BINDIR)/datafilewrite: src/crypto.cpp src/tools/datafilewrite.cpp | $(BINDIR)
	g++ -o $@ ${FINALFLAGS} src/crypto.cpp src/tools/datafilewrite.cpp -lcrypto

linecount:
	find|grep -e '\.h$$' -e '\.cpp$$'|awk '{print $$1}'|xargs wc -l	

clean:
	@+${MAKE} -C src clean
	@if test -d $(BINDIR); then rm -rf $(BINDIR); echo rm -rf $(BINDIR); fi
