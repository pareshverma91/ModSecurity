# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

BASEPATH=$(shell pwd)

PREFIX?=/usr/local
HEADER_DEST?=$(PREFIX)/include/regex_integrator
LIB_DEST?=$(PREFIX)/lib

BIN_DIR?=$(BASEPATH)/bin
OBJ_DIR=$(BASEPATH)/objs

TARGET?=$(BIN_DIR)/libregex_integrator.so
HEADER_API?=$(BASEPATH)/ri_api.h

OBJS=$(OBJ_DIR)/ri_pcre.o $(OBJ_DIR)/ri_re2.o $(OBJ_DIR)/ri_api.o

3RD?=$(BASEPATH)/3rd

PCRE_VER?=8.42
PCRE_DOWNLOAD_NAME?=pcre-$(PCRE_VER).tar.gz
PCRE_SITE?=https://ftp.pcre.org/pub/pcre/pcre-$(PCRE_VER).tar.gz
PCRE_DIR?=$(3RD)/pcre-$(PCRE_VER)
PCRE_BUILD?=$(PCRE_DIR)/build
PCRE_LIB?=$(PCRE_BUILD)/lib/libpcre.a
PCRE_INCLUDE?=$(PCRE_DIR)
PCRE_OPT?=

RE2_VER?=2018-10-01
RE2_DOWNLOAD_NAME?=re-$(RE2_VER).tar.gz
RE2_SITE?=https://github.com/google/re2/archive/$(RE2_VER).tar.gz
RE2_DIR?=$(3RD)/re2-$(RE2_VER)
RE2_LIB?=$(RE2_DIR)/obj/libre2.a
RE2_INCLUDE?=$(RE2_DIR)

AR?=ar
CC?=gcc
CXX?=g++
MAKE?=make
CINCLUDES?=-I$(PCRE_DIR)
CXXINCLUDES=-I$(RE2_DIR) 
WARNING?=-Wall -Werror
CSTD?=-std=c99
CXXSTD?=-std=c++11
CFLAGS?=-fPIC -g -O3
CXXFLAGS?=-fPIC -g -O3
LIBS?=-pthread

CLEAN_OBJS?=$(BIN_DIR) $(OBJ_DIR)

# initialize enviroment
$(shell mkdir -p $(3RD))
$(shell mkdir -p $(OBJ_DIR))
$(shell mkdir -p $(BIN_DIR))

all: build_pcre build_re2
	$(MAKE) $(TARGET)

build_pcre:
ifeq ($(wildcard $(PCRE_DIR)), )
	@cd $(3RD) && mkdir -p $(PCRE_DIR)
	@cd $(3RD) && wget $(PCRE_SITE) -O $(PCRE_DOWNLOAD_NAME)
	@cd $(3RD) && tar -xvzf $(PCRE_DOWNLOAD_NAME) -C $(PCRE_DIR) --strip-components 1
endif
	@cd $(PCRE_DIR) && autoreconf -f -i
	@cd $(PCRE_DIR) && CFLAGS="$(CFLAGS)" ./configure --enable-jit --prefix $(PCRE_BUILD) $(PCRE_OPT)
	@cd $(PCRE_DIR) && make -j
	@cd $(PCRE_DIR) && make install

build_re2:
ifeq ($(wildcard $(RE2_DIR)), )
	@cd $(3RD) && mkdir -p $(RE2_DIR)
	@cd $(3RD) && wget $(RE2_SITE) -O $(RE2_DOWNLOAD_NAME)
	@cd $(3RD) && tar -xvzf $(RE2_DOWNLOAD_NAME) -C $(RE2_DIR) --strip-components 1
endif
	@cd $(RE2_DIR) && CXXFLAGS="$(CXXFLAGS)"  make -j;

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(CSTD) $(WARNING) $(CINCLUDES) -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(WARNING) $(CXXINCLUDES) -c $< -o $@

$(TARGET): $(OBJS)
	@cd $(OBJ_DIR) && $(AR) -x $(PCRE_LIB)
	@cd $(OBJ_DIR) && $(AR) -x $(RE2_LIB)
	$(CXX) -shared $(CXXFLAGS) $(CXXSTD) $(WARNING) -o $@ $(OBJ_DIR)/*.o
	@echo $(TARGET)" Compile Success"

test: tester.cpp
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(WARNING) -I$(BASEPATH) $< $(TARGET) $(LIBS) -o $(BIN_DIR)/$@
	@bin/$@
	@echo 'Test Success'

bench: bench.cpp
	$(CXX) $(CXXFLAGS) $(CXXSTD) $(WARNING) -I$(BASEPATH) $(CINCLUDES) $(CXXINCLUDES) $< $(TARGET) $(LIBS) -o $(BIN_DIR)/$@
	@bin/$@
	@echo 'Bench Success'

install:
	mkdir -p $(HEADER_DEST)
	install $(HEADER_API) $(HEADER_DEST)
	install $(TARGET) $(LIB_DEST)
	ldconfig

clean:
	rm -rf $(CLEAN_OBJS)
