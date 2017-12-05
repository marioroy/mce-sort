
CC = g++

CFLAGS = -Drestrict=__restrict__ -std=gnu++0x -O2 -DNDEBUG -march=native
LDFLAGS = 

executables = bs-mkqs mr-merge ng-cradix tb-radix tr-radix

all: $(executables) mce-sort1 mce-sort2

$(executables): %: %.cc
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o ../bin/$@

mce-sort1:
	( cd ../bin && ./mce-sort1 -e sort file >/dev/null 2>&1 || echo )

mce-sort2:
	( cd ../bin && ./mce-sort2 -e sort file >/dev/null 2>&1 || echo )

clean:
	( cd ../bin && rm -f $(executables) && cd .. && rm -rf .Inline )

