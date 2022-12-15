
bin_PROGRAMS = test-tinyev test-lua test-lua-mylib


AM_CFLAGS = \
	-I$(top_srcdir)/tinyev \
	-I$(top_srcdir)/lua/lua5.3 \
	-I$(top_srcdir)/luaclib \
	-I$(top_srcdir)/include

AM_CFLAGS += -g -O0 -rdynamic -funwind-tables

LDADD = -L$(top_srcdir)/tinyev -L$(top_srcdir)/lua -L$(top_srcdir)/luaclib \
	-ltinyev \
	-llua \
	-lsys \
	-ltimer \
	-lpthread -lm