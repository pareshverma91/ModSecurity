FROM  ubuntu:16.04

RUN apt-get update
RUN apt-get install -y  \
    automake\
    build-essential \
    curl \
    unzip\
    m4 \
    libtool

# Install jsoncons 
RUN git submodule update --init --recursie 

RUN apt-get install -y \
        apache2-dev \
        g++-4.8 \
        libaprutil1 \
        libcurl4-openssl-dev \
        liblua5.1-dev \
        libluajit-5.1-dev \
        libpcre3-dev \
        libperl-dev \
        libunwind8 \
        libyajl-dev \
        libxml2\
        libxml2-dev\
        unzip

RUN apt clean \
    && rm -rf /var/lib/apt/lists/*
