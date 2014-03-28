
CC = g++

CFLAGS = -Drestrict=__restrict__ -std=c++0x -O2 -DNDEBUG -march=native
LDFLAGS = 

executables = bs-mkqs mr-merge ng-cradix tb-radix tr-radix

all: $(executables) mce-sort

$(executables): %: %.cc
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o ../bin/$@

mce-sort:
	( cd ../bin && ./mce-sort -e sort file >/dev/null 2>&1 || echo )

distclean:
	( cd ../bin && rm -f $(executables) && cd .. && rm -rf _Inline )

