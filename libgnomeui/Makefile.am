## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

#
# The targets
#
noinst_PROGRAMS = ted_demo stock_demo
bin_PROGRAMS    = gnome_segv
lib_LTLIBRARIES = libgnomeui.la

#
# Install location for header files
#
libgnomeuiincludedir = $(includedir)/libgnomeui

INCLUDES = -I.. -I$(srcdir)/.. -I$(includedir) $(GTK_CFLAGS) $(SUPPORTINCS) \
	-I$(top_srcdir)/intl -I$(top_builddir)/intl \
        -DGNOMELIBDIR=\""$(libdir)"\" \
        -DGNOMEDATADIR=\""$(datadir)"\" \
        -DGNOMEBINDIR=\""$(bindir)"\" \
        -DGNOMELOCALSTATEDIR=\""$(localstatedir)"\" \
        -DGNOMELOCALEDIR=\""$(datadir)/locale"\"


# these files are `autoconf'ed in.
EXTRA_libgnomeui_la_SOURCES = \
	gnome-canvas.c			\
	gnome-canvas-image.c		\
	gnome-canvas-rect-ellipse.c	\
	gnome-canvas-text.c

EXTRA_HEADERS = \
	gnome-canvas.h			\
	gnome-canvas-image.h		\
	gnome-canvas-rect-ellipse.h	\
	gnome-canvas-text.h

libgnomeui_la_DEPENDENCIES = @GNOME_CANVAS_OBJS@

libgnomeui_la_SOURCES = \
	gnome-about.c			\
	gnome-actionarea.c		\
	gnome-app.c			\
	gnome-appbar.c			\
	gnome-app-helper.c		\
	gnome-app-util.c		\
	gnome-calculator.c              \
	@GNOME_CANVAS_SRC@		\
	gnome-client.c			\
	gnome-color-selector.c 		\
	gnome-dateedit.c		\
	gnome-dentry-edit.c		\
	gnome-dialog.c                  \
	gnome-dialog-util.c		\
	gnome-dns.c			\
	gnome-geometry.c		\
	gnome-entry.c			\
	gnome-file-entry.c		\
	gnome-font-selector.c 		\
	gnome-ice.c			\
	gnome-icon-list.c		\
	gnome-icon-sel.c		\
	gnome-init.c			\
	gnome-lamp.c			\
	gnome-less.c			\
	gnome-mdi.c			\
	gnome-mdi-child.c		\
	gnome-messagebox.c		\
	gnome-net.c			\
	gnome-number-entry.c		\
	gnome-pixmap.c			\
	gnome-popupmenu.c		\
	gnome-preferences.c		\
	gnome-propertybox.c		\
	gnome-properties.c		\
	gnome-rootwin.c			\
	gnome-scores.c			\
	gnome-startup.c			\
	gnome-stock.c			\
	gnome-stock-imlib.h		\
	gnometypes.c			\
	gtkcalendar.c			\
	gtkspell.c			\
	gtk-clock.c			\
	gtk-plug.c			\
	gtk-socket.c			\
	gtk-ted.c			\
	gtkdial.c

libgnomeuiinclude_HEADERS = 		\
	gnome-about.h			\
	gnome-actionarea.h     		\
	gnome-app.h			\
	gnome-appbar.h			\
	gnome-app-helper.h		\
	gnome-app-util.h		\
	gnome-calculator.h              \
	@GNOME_CANVAS_HED@		\
	gnome-client.h			\
	gnome-color-selector.h		\
	gnome-dateedit.h		\
	gnome-dentry-edit.h		\
	gnome-dialog.h                  \
	gnome-dialog-util.h		\
	gnome-dns.h			\
	gnome-entry.h			\
	gnome-file-entry.h		\
	gnome-font-selector.h 		\
	gnome-geometry.h		\
	gnome-ice.h			\
	gnome-icon-list.h		\
	gnome-icon-sel.h		\
	gnome-lamp.h			\
	gnome-less.h			\
	gnome-messagebox.h		\
	gnome-mdi.h			\
	gnome-mdi-child.h		\
	gnome-number-entry.h		\
	gnome-net.h			\
	gnome-popupmenu.h		\
	gnome-preferences.h		\
	gnome-propertybox.h		\
	gnome-properties.h     		\
	gnome-pixmap.h			\
	gnome-scores.h			\
	gnome-startup.h			\
	gnome-rootwin.h			\
	gnome-stock.h			\
	gnome-types.h			\
	gnome-uidefs.h			\
	gnometypebuiltins.h		\
	gtkcalendar.h			\
	gtkcauldron.h			\
	gtkdial.h			\
	gtkspell.h			\
	gtk-clock.h			\
	gtk-ted.h			\
	gtk-plug.h			\
	gtk-socket.h			\
	libgnomeui.h

.PHONY: auto-files auto-files-1
auto-files:
	rm -f $(srcdir)/gnome.defs
	$(MAKE) auto-files-1

auto-files-1: gnome.defs
auto-files-1: gnometypebuiltins_evals.c gnometypebuiltins_ids.c
auto-files-1: gnometypebuiltins_vars.c gnometypebuiltins.h

# generate gnome.defs file from gnome-boxed.defs and *.h
gnome.defs: @MAINT@ makeenums.pl gnome-boxed.defs
	srcdir=$(srcdir) \
	  perl $(srcdir)/makeenums.pl defs $(srcdir)/g*.h \
               > gd.tmp
	cat $(srcdir)/gnome-boxed.defs >> gd.tmp
	cp gd.tmp $(srcdir)/gnome.defs
	rm -f gd.tmp

# generate type identifier header (GTK_TYPE_WIDGET_FLAGS)
gnometypebuiltins.h: @MAINT@ gnome.defs maketypes.awk
	awk -f $(srcdir)/maketypes.awk $(srcdir)/gnome.defs macros > gtb.tmp
	cp gtb.tmp $(srcdir)/gnometypebuiltins.h
	rm -f gtb.tmp

# generate type identifier variables (GTK_TYPE_WIDGET_FLAGS)
gnometypebuiltins_vars.c: @MAINT@ gnome.defs maketypes.awk
	awk -f $(srcdir)/maketypes.awk $(srcdir)/gnome.defs variables > gtbv.tmp
	cp gtbv.tmp $(srcdir)/gnometypebuiltins_vars.c
	rm -f gtbv.tmp

# generate type entries for type-id registration
gnometypebuiltins_ids.c: @MAINT@ gnome.defs maketypes.awk
	awk -f $(srcdir)/maketypes.awk $(srcdir)/gnome.defs entries > gtbi.tmp
	cp gtbi.tmp $(srcdir)/gnometypebuiltins_ids.c
	rm -f gtbi.tmp

# generate enum value arrays
gnometypebuiltins_evals.c: @MAINT@ makeenums.pl gnome.defs
	srcdir=$(srcdir) \
	  perl $(srcdir)/makeenums.pl arrays $(srcdir)/g*.h \
               > gtbe.tmp
	cp gtbe.tmp $(srcdir)/gnometypebuiltins_evals.c
	rm -f gtbe.tmp

#######################

gnome_segv_SOURCES = gnome_segv.c

ted_demo_SOURCES = ted_demo.c

stock_demo_SOURCES = stock_demo.c

SGML =	\
	gtk-ted.sgml


gtkrcdir = $(datadir)
gtkrc_DATA = gtkrc

EXTRA_DIST =					\
	$(SGML)					\
	$(gtkrc_DATA)				\
	AUTHORS					\
	gnome.defs 				\
	maketypes.awk				\
	makeenums.pl				\
	gnometypebuiltins_vars.c		\
	gnometypebuiltins_ids.c			\
	gnometypebuiltins_evals.c

libgnomeui_la_LDFLAGS = -version-info 0:0:0 -rpath $(libdir)
libgnomeui_la_LIBADD = $(LIBGNOMEUI_LIBS) @GNOME_CANVAS_OBJS@

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
