# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

BASEPATH=$(shell pwd)

PREFIX?=/usr/local
HEADER_DEST?=$(PREFIX)/include/regex_integrator
LIB_DEST?=$(PREFIX)/lib

BIN_DIR?=$(BASEPATH)/bin
OBJ_DIR=$(BASEPATH)/objs

TARGET?=$(BIN_DIR)/libregex_integrator.a
HEADER_API?=$(BASEPATH)/ri_api.h

OBJS=$(OBJ_DIR)/ri_pcre.o $(OBJ_DIR)/ri_re2.o $(OBJ_DIR)/ri_api.o

3RD?=$(BASEPATH)/3rd

PCRE_VER?=pcre-8.42
PCRE_SITE?=https://ftp.pcre.org/pub/pcre/$(PCRE_VER).tar.gz
PCRE_DIR?=$(3RD)/$(PCRE_VER)
PCRE_LIB?=$(PCRE_DIR)/.libs/libpcre.a
PCRE_INCLUDE?=$(PCRE_DIR)

RE2_SITE?=https://github.com/google/re2.git
RE2_DIR?= $(3RD)/re2
RE2_LIB?=$(RE2_DIR)/obj/libre2.a
RE2_INCLUDE?=$(RE2_DIR)

AR?=ar
CC?=gcc
CXX?=g++
CINCLUDES?=-I$(PCRE_DIR)
CXXINCLUDES=-I$(RE2_DIR) 
CFLAGS?=-g -O3 -Wall -Werror -std=c99
CXXFLAGS?=-g -O3 -Wall -Werror -std=c++11
LIBS?=-pthread

CLEAN_OBJS?=$(BIN_DIR) $(OBJ_DIR)

# initialize enviroment
$(shell mkdir -p $(3RD))
$(shell mkdir -p $(OBJ_DIR))
$(shell mkdir -p $(BIN_DIR))

all: build_pcre build_re2 $(TARGET)

build_pcre:
ifeq ($(wildcard $(PCRE_DIR)), )
	@cd $(3RD) && wget $(PCRE_SITE)
	@cd $(3RD) && tar -xvzf $(PCRE_VER).tar.gz
	@cd $(PCRE_DIR) && ./configure --enable-jit
endif
	@cd $(PCRE_DIR) && make -j

build_re2:
ifeq ($(wildcard $(RE2_DIR)), )
	@git clone $(RE2_SITE) $(RE2_DIR)
	@cd $(RE2_DIR)
else
	@cd $(RE2_DIR) && git pull
endif
	@cd $(RE2_DIR) && make -j;

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(CINCLUDES) -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) $(CXXINCLUDES) -c $< -o $@

$(TARGET): $(OBJS)
	@cd $(OBJ_DIR) && $(AR) -x $(PCRE_LIB)
	@cd $(OBJ_DIR) && $(AR) -x $(RE2_LIB)
	$(AR) rcs -o $@ $(OBJ_DIR)/*.o
	@echo $(TARGET)" Compile Success"

test: tester.cpp
	$(CXX) $(CXXFLAGS) -I$(BASEPATH) $< $(TARGET) $(LIBS) -o $(BIN_DIR)/$@
	@bin/$@
	@echo 'Test Success'

bench: bench.cpp
	$(CXX) $(CXXFLAGS) -I$(BASEPATH) $(CINCLUDES) $(CXXINCLUDES) $< $(TARGET) $(LIBS) -o $(BIN_DIR)/$@
	@bin/$@
	@echo 'Bench Success'

install:
	mkdir -p $(HEADER_DEST)
	install $(HEADER_API) $(HEADER_DEST)
	install $(TARGET) $(LIB_DEST)
	ldconfig

clean:
	rm -rf $(CLEAN_OBJS)
