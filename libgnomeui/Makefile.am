## Process this file with automake to produce Makefile.in

libgnomeuiincludedir = $(includedir)/libgnomeui

INCLUDES = -I.. -I$(srcdir)/.. -I$(includedir)

lib_LTLIBRARIES = libgnomeui.la

libgnomeui_la_SOURCES = \
	gnome-actionarea.c		\
	gnome-app.c			\
	gnome-app-helper.c		\
	gnome-color-selector.c 		\
	gnome-colors.c			\
	gnome-ice.c			\
	gnome-init.c			\
	gnome-messagebox.c		\
	gnome-pixmap.c			\
	gnome-properties.c		\
	gnome-session.c			\
	gtk-clock.c

libgnomeuiinclude_HEADERS = 		\
        gnome-actionarea.h     		\
	gnome-app.h			\
	gnome-app-helper.h		\
	gnome-color-selector.h 		\
	gnome-colors.h         		\
	gnome-ice.h			\
	gnome-messagebox.h		\
	gnome-properties.h     		\
	gnome-pixmap.h			\
	gnome-session.h			\
	gtk-clock.h			\
	libgnomeui.h

libgnomeui_la_LDFLAGS = -version-info 0:0:0 -rpath $(libdir)
libgnomeui_la_LIBADD = $(GTK_LIBS)

files:
        @files=`ls $(DISTFILES) 2> /dev/null `; for p in $$files; do \
          echo $$p; \
        done
