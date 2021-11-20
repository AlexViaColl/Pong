#!/bin/sh

CFLAGS=-ggdb
LIBS="-lm -lX11 -lGL -lasound"
SOURCES=pong.cpp

gcc $CFLAGS $LIBS -o pong $SOURCES
