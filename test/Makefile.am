
bin_PROGRAMS = test-lua test-lua-mylib


AM_CFLAGS = \
	-I$(top_srcdir)/libev \
	-I$(top_srcdir)/lua/lua5.3 \
	-I$(top_srcdir)/luaclib \
	-I$(top_srcdir)/include

AM_CFLAGS += -g -O0 -rdynamic -funwind-tables

LDADD = -L$(top_srcdir)/libev -L$(top_srcdir)/lua -L$(top_srcdir)/luaclib \
	-lev \
	-llua \
	-lsys \
	-ltimer \
	-lpthread -lm -ldl -lreadline