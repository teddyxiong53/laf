lib_LTLIBRARIES = libtinyev.la

AM_CFLAGS = \
	-I$(top_srcdir)/include

AM_CFLAGS += -g -O0 -rdynamic -funwind-tables

libtinyev_la_SOURCES = \
	ev_epoll.c ev_poll.c ev_select.c tinyev.c

