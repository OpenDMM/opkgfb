AM_CFLAGS = @FREETYPE_CFLAGS@ @GLIB_CFLAGS@
AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = opkgfb
opkgfb_SOURCES = main.c render.c 

opkgfb_LDADD= \
	@FREETYPE_LIBS@ \
	@GLIB_LIBS@
