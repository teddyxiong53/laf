
bin_PROGRAMS = test-tinyev


AM_CFLAGS = \
	-I$(top_srcdir)/tinyev \
	-I$(top_srcdir)/include

AM_CFLAGS += -g -O0 -rdynamic -funwind-tables

LDADD = -L$(top_srcdir)/tinyev \
	-ltinyev \
	-lpthread -lm