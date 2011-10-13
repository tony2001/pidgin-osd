# The input Makefile for the main pidgin-psd

# The plugin directory.
libdir			=	$(PIDGIN_LIBDIR)/pidgin

# The plugin library.
lib_LTLIBRARIES		=	pidgin-osd.la

# The plugin sources.
pidgin_osd_la_SOURCES	=	pidgin-osd.c
pidgin_osd_la_CFLAGS	=	@PIDGIN_CFLAGS@
pidgin_osd_la_LDFLAGS	=	@XOSD_LDFLAGS@ \
				-module \
				-avoid-version
