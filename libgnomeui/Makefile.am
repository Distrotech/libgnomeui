## Process this file with automake to produce Makefile.in

libgnomeuiincludedir = $(includedir)/libgnomeui

INCLUDES = -I.. -I$(srcdir)/.. -I$(includedir)

lib_LTLIBRARIES = libgnomeui.la
noinst_PROGRAMS = ted_demo stock_demo

libgnomeui_la_SOURCES = \
	gnome-actionarea.c		\
	gnome-app.c			\
	gnome-app-helper.c		\
	gnome-color-selector.c 		\
	gnome-font-selector.c 		\
	gnome-colors.c			\
	gnome-ice.c			\
	gnome-init.c			\
	gnome-messagebox.c		\
	gnome-about.c			\
	gnome-pixmap.c			\
	gnome-properties.c		\
	gnome-session.c			\
	gnome-scores.c			\
	gnome-stock.c			\
	gtk-clock.c			\
	gtk-ted.c

libgnomeuiinclude_HEADERS = 		\
        gnome-actionarea.h     		\
	gnome-app.h			\
	gnome-app-helper.h		\
	gnome-color-selector.h 		\
	gnome-font-selector.h 		\
	gnome-colors.h         		\
	gnome-ice.h			\
	gnome-messagebox.h		\
	gnome-about.h			\
	gnome-properties.h     		\
	gnome-pixmap.h			\
	gnome-session.h			\
	gnome-scores.h			\
	gnome-stock.h			\
	gtk-clock.h			\
	gtk-ted.h			\
	libgnomeui.h

ted_demo_SOURCES = ted_demo.c

stock_demo_SOURCES = stock_demo.c

SGML =	\
	gtk-ted.sgml

PIXMAPS = \
	gnome-stock-ok.xpm		\
	gnome-stock-cancel.xpm		\

EXTRA_DIST = \
	$(PIXMAPS)			\
	$(SGML)				\
	AUTHORS

libgnomeui_la_LDFLAGS = -version-info 0:0:0 -rpath $(libdir)
libgnomeui_la_LIBADD = $(GTK_LIBS) $(X_LIBS) $(x_libs)

ted_demo_LDADD =	\
	$(top_builddir)/libgnome/libgnome.la 		\
	$(top_builddir)/libgnomeui/libgnomeui.la 	\
	$(GTK_LIBS) $(X_LIBS) $(x_libs) $(INTLLIBS) $(DL_LIB)

stock_demo_LDADD =	\
	$(top_builddir)/libgnome/libgnome.la 		\
	$(top_builddir)/libgnomeui/libgnomeui.la 	\
	$(GTK_LIBS) $(X_LIBS) $(x_libs) $(INTLLIBS) $(DL_LIB)
