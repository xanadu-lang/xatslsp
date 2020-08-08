######
#
# A simple Makefile
#
######

CC=gcc

######

INCLUDE:=
INCLUDE+=-I"./DEPS/json.h"
INCLUDE+=-I"./DEPS/picohttpparser"

######

objects::

######

objects:: \
BUILD/json_rpc.o
BUILD/json_rpc.o: \
CATS/json_rpc.c; $(CC) $(INCLUDE) -c -o $@ $<

objects:: \
BUILD/language_server.o
BUILD/language_server.o: \
CATS/language_server.c; $(CC) $(INCLUDE) -c -o $@ $<

######

objects:: \
BUILD/picohttpparser.o
BUILD/picohttpparser.o: \
./DEPS/picohttpparser/picohttpparser.c; $(CC) $(INCLUDE) -c -o $@ $<

######

cleanall: ; rm -f BUILD/*.o

######

###### end of [Makefile] ######
