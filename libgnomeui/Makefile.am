## Process this file with automake to produce Makefile.in

libgnomeuiincludedir = $(includedir)/libgnomeui

INCLUDES = -I.. -I$(srcdir)/.. -I$(includedir) @GTK_CFLAGS@ @SUPPORTINCS@ \
        -DGNOMELIBDIR=\""$(libdir)"\" \
        -DGNOMEDATADIR=\""$(datadir)"\" \
        -DGNOMEBINDIR=\""$(bindir)"\" \
        -DGNOMELOCALSTATEDIR=\""$(localstatedir)"\" \
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"

noinst_PROGRAMS = ted_demo stock_demo
bin_PROGRAMS = gnome_segv
lib_LTLIBRARIES = libgnomeui.la

libgnomeui_la_SOURCES = \
	gnome-actionarea.c		\
	gnome-app.c			\
	gnome-app-helper.c		\
	gnome-color-selector.c 		\
	gnome-dialog.c                  \
	gnome-dns.c			\
	gnome-entry.c			\
	gnome-file-entry.c		\
	gnome-font-selector.c 		\
	gnome-ice.c			\
	gnome-icon-list.c		\
	gnome-init.c			\
	gnome-messagebox.c		\
	gnome-net.c			\
	gnome-about.c			\
	gnome-pixmap.c			\
	gnome-propertybox.c		\
	gnome-properties.c		\
	gnome-client.c			\
	gnome-scores.c			\
	gnome-startup.c			\
	gnome-rootwin.c			\
	gnome-stock.c			\
	gtk-clock.c			\
	gtk-ted.c			\
	gnome-stock-imlib.h		\
	gnome-stock-xpm.h		\
	gtk-plug.c			\
	gtk-socket.c			\
	gtkcalendar.c			\
	gnome-dateedit.c		\
	gnome-calculator.c

libgnomeuiinclude_HEADERS = 		\
	gnome-about.h			\
        gnome-actionarea.h     		\
	gnome-app.h			\
	gnome-app-helper.h		\
	gnome-color-selector.h 		\
	gnome-dialog.h                  \
	gnome-dns.h			\
	gnome-entry.h			\
	gnome-file-entry.h		\
	gnome-font-selector.h 		\
	gnome-ice.h			\
	gnome-icon-list.h		\
	gnome-messagebox.h		\
	gnome-net.h			\
	gnome-propertybox.h		\
	gnome-properties.h     		\
	gnome-pixmap.h			\
	gnome-client.h			\
	gnome-scores.h			\
	gnome-startup.h			\
	gnome-rootwin.h			\
	gnome-stock.h			\
	gnome-uidefs.h			\
	gtk-clock.h			\
	gtk-ted.h			\
	gtk-plug.h			\
	gtk-socket.h			\
	libgnomeui.h			\
	gtkcalendar.h			\
	gnome-dateedit.h		\
	gnome-calculator.h

gnome_segv_SOURCES = gnome_segv.c

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

gnome_segv_LDADD =	\
	$(top_builddir)/libgnomeui/libgnomeui.la 	\
	$(top_builddir)/libgnome/libgnome.la 		\
	@LIBSUPPORT@ $(LIBGNOMEUI_LIBS) \
	$(INTLLIBS)

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
