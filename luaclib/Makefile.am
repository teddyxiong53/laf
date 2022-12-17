libsys_LTLIBRARIES = libsys.la
libtimer_LTLIBRARIES = libtimer.la
libtask_LTLIBRARIES = libtask.la

AM_CFLAGS = \
	-I$(top_srcdir)/lua/lua5.3 \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/include

AM_CFLAGS += -g -O0 -rdynamic -funwind-tables

libsys_la_SOURCES = \
	lsys.c

libtimer_la_SOURCES = \
	ltimer.c

libtask_la_SOURCES = \
	ltask.c

real_top = $(abspath $(top_srcdir)/luaclib)

libsysdir := $(real_top)
libtimerdir := $(real_top)
libtaskdir := $(real_top)
