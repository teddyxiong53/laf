libsys_LTLIBRARIES = libsys.la
libtimer_LTLIBRARIES = libtimer.la

AM_CFLAGS = \
	-I$(top_srcdir)/lua/lua5.3 \
	-I$(top_srcdir)/include

AM_CFLAGS += -g -O0 -rdynamic -funwind-tables

libsys_la_SOURCES = \
	lsys.c

libtimer_la_SOURCES = \
	ltimer.c
real_top = $(abspath $(top_srcdir)/luaclib)

libsysdir := $(real_top)
libtimerdir := $(real_top)

