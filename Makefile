# Makefile for the CMSPixelDecoder tools
# See README for documentation

CC = g++

CFLAGS  = -Wall -Wextra -g -Wno-deprecated -ansi -pedantic -Wno-long-long
LDFLAGS = -lSpectrum
SOFLAGS = -shared

ROOTCFLAGS    = $(shell $(ROOTSYS)/bin/root-config --cflags)
ROOTLIBS      = $(shell $(ROOTSYS)/bin/root-config --libs)
ROOTGLIBS     = $(shell $(ROOTSYS)/bin/root-config --glibs)

CFLAGS       += $(ROOTCFLAGS)

OBJECTS=CMSPixelDecoder.o

all: d2h unit_tests stat_tool mc_background tentative_dec

d2h: d2h.cc
	$(CC) $(CFLAGS) $(ROOTGLIBS) d2h.cc CMSPixelDecoder.cc -o d2h -Wl,-rpath,$(ROOTGLIBS)

unit_tests: unit_tests.cc $(OBJECTS)
	$(CC) $(CFLAGS) unit_tests.cc -o unit_tests $(OBJECTS)

stat_tool: stat_tool.cc $(OBJECTS)
	$(CC) $(CFLAGS) stat_tool.cc -o stat_tool $(OBJECTS)

mc_background: mc_background.cc $(OBJECTS)
	$(CC) $(CFLAGS) mc_background.cc -o mc_background $(OBJECTS)

tentative_dec: tentative_dec.cc $(OBJECTS)
	$(CC) $(CFLAGS) tentative_dec.cc -o tentative_dec $(OBJECTS)


clean:	
	rm *.o
	rm unit_tests stat_tool d2h mc_background tentative_dec

