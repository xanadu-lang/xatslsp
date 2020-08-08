######
#
# A simple Makefile
#
######
CC=gcc
MAKE=make
######
RMF=rm -f
######

CATS="./../CATS"
DEPS="./../DEPS"
BUILD="./../BUILD"

######

INCLUDE:=
INCLUDE+=-I"$(CATS)"
INCLUDE+=-I"$(DEPS)/json.h"
INCLUDE+=-I"$(DEPS)/picohttpparser"

######

all::
all:: json_rpc_tests

######
OBJECTS:=
######
OBJECTS+=$(BUILD)/picohttpparser.o
######
OBJECTS+=$(BUILD)/json_rpc.o
OBJECTS+=$(BUILD)/language_server.o
######

json_rpc_tests:: \
json_rpc_tests.c ; $(CC) $(INCLUDE) -o $@ $< $(OBJECTS)

######

objects:: ; \
$(MAKE) -C ".." objects

######

cleanall:: ; $(RMF) json_rpc_tests

######

###### end of [Makefile] ######
