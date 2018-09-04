3RD = 3rd
RE2_SITE = https://github.com/google/re2.git
RE2_DIR = $(3RD)/re2
PCRE_VER = pcre-8.42
PCRE_SITE = https://ftp.pcre.org/pub/pcre/$(PCRE_VER).tar.gz
PCRE_DIR = $(3RD)/$(PCRE_VER)

CC = gcc
CXX = g++
CFLAGS = -g -O3 -Wall -Werror -std=c99
CXXFLAGS = -g -O3 -Wall -Werror -std=c++11
LIBS = -pthread -L./$(PCRE_DIR)/.libs -I./$(PCRE_DIR) -lpcre
CXXLIBS = -L./$(RE2_DIR)/obj -I./$(RE2_DIR)/re2 -lre2
TARGET = bin/libregex_integrator.so
OBJS = objs/ri_pcre.o objs/ri_re2.o objs/ri_api.o

# initialize enviroment
ifeq ($(wildcard $(3RD)), )
$(shell mkdir $(3RD))
endif
ifeq ($(wildcard objs), )
$(shell mkdir objs)
endif
ifeq ($(wildcard bin), )
$(shell mkdir bin)
endif

all: build_pcre build_re2 $(TARGET) test

build_pcre:
ifeq ($(wildcard $(PCRE_DIR)), )
	@cd $(3RD) && wget $(PCRE_SITE)
	@cd $(3RD) && tar -xvzf $(PCRE_VER).tar.gz
	@cd $(PCRE_DIR) && CFLAGS=-fPIC ./configure --enable-static=true --enable-shared=false
endif
	@cd $(PCRE_DIR) && make -j

build_re2:
ifeq ($(wildcard $(RE2_DIR)), )
	@git clone $(RE2_SITE) $(RE2_DIR)
	@cd $(RE2_DIR)
else
	@cd $(RE2_DIR) && git pull
endif
	@cd $(RE2_DIR) && CXXFLAGS=-fPIC make -j;

objs/%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

objs/%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -fPIC -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -fPIC -shared -o $@ $(OBJS) $(LIBS) $(CXXLIBS)
	@echo $(TARGET)" Compile Success"

test: tester.cpp
	$(CXX) $(CXXFLAGS) $< $(shell pwd)/$(TARGET) -Wl,-rpath=$(shell pwd)/bin -o bin/$@
	@bin/$@
	@echo 'Test Success'

clean:
	rm -rf objs
	rm -rf bin
