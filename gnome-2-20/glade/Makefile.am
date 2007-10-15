## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-DG_LOG_DOMAIN=\"GnomeUI\"			\
	$(WARN_CFLAGS)					\
	$(LIBGNOMEUI_GLADE_CFLAGS)

glademoduledir = $(libdir)/libglade/2.0
glademodule_LTLIBRARIES = libgnome.la

libgnome_la_LIBADD = \
	$(top_builddir)/libgnomeui/libgnomeui-2.la	\
	$(LIBGNOMEUI_GLADE_LIBS)

libgnome_la_LDFLAGS = \
	-export-dynamic -module -avoid-version -no-undefined

libgnome_la_SOURCES = \
	glade-gnome.c
