
.PHONY: all

all: dynthr dynthr_mod.so

test: all
	./dynthr

dynthr: dynthr.c
	cc -Wall -o dynthr dynthr.c

dynthr_mod.so: dynthr.c
	cc -Wall -DMODULE -fPIC -shared -pthread -o dynthr_mod.so dynthr.c
