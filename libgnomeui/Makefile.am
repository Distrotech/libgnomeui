## Process this file with automake to produce Makefile.in

libgnomeuiincludedir = $(includedir)/libgnomeui

INCLUDES = -I.. -I$(srcdir)/.. -I$(includedir) @SUPPORTINCS@

lib_LTLIBRARIES = libgnomeui.la
noinst_PROGRAMS = ted_demo stock_demo

libgnomeui_la_SOURCES = \
	gnome-actionarea.c		\
	gnome-app.c			\
	gnome-app-helper.c		\
	gnome-color-selector.c 		\
	gnome-dns.c			\
	gnome-entry.c			\
	gnome-font-selector.c 		\
	gnome-ice.c			\
	gnome-init.c			\
	gnome-messagebox.c		\
	gnome-net.c			\
	gnome-about.c			\
	gnome-pixmap.c			\
	gnome-properties.c		\
	gnome-client.c			\
	gnome-scores.c			\
	gnome-startup.c			\
	gnome-stock.c			\
	gtk-clock.c			\
	gtk-ted.c			\
	gnome-stock-imlib.h		\
	gnome-stock-xpm.h

libgnomeuiinclude_HEADERS = 		\
	gnome-about.h			\
        gnome-actionarea.h     		\
	gnome-app.h			\
	gnome-app-helper.h		\
	gnome-color-selector.h 		\
	gnome-dns.h			\
	gnome-entry.h			\
	gnome-font-selector.h 		\
	gnome-ice.h			\
	gnome-messagebox.h		\
	gnome-net.h			\
	gnome-properties.h     		\
	gnome-pixmap.h			\
	gnome-client.h			\
	gnome-scores.h			\
	gnome-startup.h			\
	gnome-stock.h			\
	gtk-clock.h			\
	gtk-ted.h			\
	libgnomeui.h

ted_demo_SOURCES = ted_demo.c

stock_demo_SOURCES = stock_demo.c

SGML =	\
	gtk-ted.sgml


gtkrcdir = $(datadir)
gtkrc_DATA = gtkrc

EXTRA_DIST = \
	$(SGML)				\
	$(gtkrc_DATA)			\
	AUTHORS

libgnomeui_la_LDFLAGS = -version-info 0:0:0 -rpath $(libdir)
libgnomeui_la_LIBADD = $(LIBGNOMEUI_LIBS)

ted_demo_LDADD =	\
	$(top_builddir)/libgnomeui/libgnomeui.la 	\
	$(top_builddir)/libgnome/libgnome.la 		\
	@LIBSUPPORT@ $(LIBGNOMEUI_LIBS) \
	$(INTLLIBS)

stock_demo_LDADD =	\
	$(top_builddir)/libgnomeui/libgnomeui.la 	\
	$(top_builddir)/libgnome/libgnome.la 		\
	@LIBSUPPORT@ $(LIBGNOMEUI_LIBS) \
	$(INTLLIBS)

