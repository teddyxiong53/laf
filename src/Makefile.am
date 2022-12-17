
bin_PROGRAMS = cfadmin
cfadmin_DEPENDENCIES = $(lib_LTLIBRARIES)

cfadmin_SOURCES = core_start.c core.c core_ev.c

real_top := $(abspath $(top_srcdir)/)
entry_script := $(real_top)/script/main.lua

AM_CFLAGS = \
	-I$(top_srcdir)/libev \
	-I$(top_srcdir)/lua/lua5.3 \
	-I$(top_srcdir)/luaclib \
	-I$(top_srcdir)/include

AM_CFLAGS += -g -O0 -rdynamic -funwind-tables

AM_CFLAGS += -DLUALIBS_PATH="\"$(real_top)/lualib/?.lua;$(real_top)/lualib/?/init.lua;;\""
AM_CFLAGS += -DLUACLIBS_PATH="\"$(real_top)/luaclib/.libs/lib?.so;;\""
AM_CFLAGS += -DLUA_ENTRY="\"$(entry_script)\""
AM_CPPFLAGS = $(AM_CFLAGS)

LDADD = -L$(top_srcdir)/libev -L$(top_srcdir)/lua -L$(top_srcdir)/luaclib \
	-lev \
	-llua \
	-lsys \
	-ltimer \
	-lpthread -lm -ldl -lreadline