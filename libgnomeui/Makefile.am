## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

#
# The targets
#
noinst_PROGRAMS = stock_demo winhints_demo animator_demo dock_demo
bin_PROGRAMS    = gnome_segv
lib_LTLIBRARIES = libgnomeui.la
lib_LIBRARIES = libgnomeui10compat.a
#
# Install location for header files
#
libgnomeuiincludedir = $(includedir)/libgnomeui

INCLUDES = -I.. -I$(srcdir)/.. $(SUPPORTINCS) \
	-I$(top_srcdir)/intl -I$(top_builddir)/intl \
	-I$(includedir) $(GDK_IMLIB_CFLAGS) \
	$(AUDIOFILE_CFLAGS) $(ESD_CFLAGS) \
        -DGNOMELIBDIR=\""$(libdir)"\" \
        -DGNOMEDATADIR=\""$(datadir)"\" \
        -DGNOMEBINDIR=\""$(bindir)"\" \
        -DGNOMELOCALSTATEDIR=\""$(localstatedir)"\" \
        -DGNOMELOCALEDIR=\""$(gnomelocaledir)"\" \
	-DG_LOG_DOMAIN=\"GnomeUI\"

EXTRA_HEADERS = 

libgnomeui10compat_a_SOURCES =\
	gnome-guru.c			\
	gtkcauldron.c			\
	gnome-compat.c

#libgnomeui10compat_a_HEADERS =\
#	gnome-guru.h			\
#	gtkcauldron.h			\
#	gnome-compat.h

libgnomeui_la_SOURCES = \
	gnome-about.c			\
	gnome-animator.c		\
	gnome-app.c			\
	gnome-appbar.c			\
	gnome-app-helper.c		\
	gnome-app-util.c		\
	gnome-calculator.c              \
	gnome-canvas.c			\
	gnome-canvas-image.c		\
	gnome-canvas-load.c		\
	gnome-canvas-line.c		\
	gnome-canvas-rect-ellipse.c	\
	gnome-canvas-polygon.c		\
	gnome-canvas-text.c		\
	gnome-canvas-util.c		\
	gnome-canvas-widget.c		\
	gnome-client.c			\
	gnome-color-picker.c		\
	gnome-dateedit.c		\
	gnome-dentry-edit.c		\
	gnome-dialog.c                  \
	gnome-dialog-util.c		\
	gnome-dns.c			\
	gnome-dock.c			\
	gnome-dock-band.c		\
	gnome-dock-item.c		\
	gnome-dock-layout.c		\
	gnome-druid.c			\
	gnome-druid-page.c		\
	gnome-druid-page-start.c	\
	gnome-druid-page-standard.c	\
	gnome-druid-page-finish.c	\
	gnome-geometry.c		\
	gnome-entry.c			\
	gnome-file-entry.c		\
        gnome-font-picker.c		\
	gnome-font-selector.c 		\
	gnome-href.c			\
	gnome-ice.c			\
	gnome-icon-list.c		\
	gnome-icon-item.c		\
	gnome-icon-sel.c		\
	gnome-icon-text.c		\
	gnome-icon-entry.c		\
	gnome-init.c			\
	gnome-less.c			\
	gnome-mdi.c			\
	gnome-mdi-child.c		\
	gnome-mdi-generic-child.c       \
	gnome-mdi-session.c		\
	gnome-messagebox.c		\
	gnome-number-entry.c		\
	gnome-pixmap.c			\
	gnome-pixmap-entry.c		\
	gnome-popup-menu.c		\
	gnome-popup-help.c		\
	gnome-preferences.c		\
	gnome-propertybox.c		\
	gnome-properties.c		\
	gnome-property-entries.c	\
	gnome-scores.c			\
	gnome-spell.c			\
	gnome-startup.c			\
	gnome-stock.c			\
        gnome-winhints.c		\
        gnome-paper-selector.c		\
	gnome-procbar.c			\
	gnometypes.c			\
	gtk-clock.c			\
	gtkdial.c			\
	gtkpixmapmenuitem.c

## this lists all the non-generated headers
gnome_headers = \
	gnome-about.h			\
        gnome-animator.h		\
	gnome-app.h			\
	gnome-appbar.h			\
	gnome-app-helper.h		\
	gnome-app-util.h		\
	gnome-calculator.h              \
	gnome-canvas.h			\
	gnome-canvas-image.h		\
	gnome-canvas-line.h		\
	gnome-canvas-load.h		\
	gnome-canvas-rect-ellipse.h	\
	gnome-canvas-polygon.h		\
	gnome-canvas-text.h		\
	gnome-canvas-util.h		\
	gnome-canvas-widget.h		\
	gnome-client.h			\
	gnome-color-picker.h		\
	gnome-compat.h			\
	gnome-dateedit.h		\
	gnome-dentry-edit.h		\
	gnome-dialog.h                  \
	gnome-dialog-util.h		\
	gnome-dns.h			\
	gnome-dock.h			\
	gnome-dock-band.h		\
	gnome-dock-item.h		\
	gnome-dock-layout.h		\
	gnome-druid.h			\
	gnome-druid-page.h		\
	gnome-druid-page-start.h	\
	gnome-druid-page-standard.h	\
	gnome-druid-page-finish.h	\
	gnome-entry.h			\
	gnome-file-entry.h		\
        gnome-font-picker.h		\
	gnome-font-selector.h 		\
	gnome-geometry.h		\
	gnome-href.h			\
	gnome-ice.h			\
	gnome-icon-list.h		\
	gnome-icon-item.h		\
	gnome-icon-sel.h		\
	gnome-icon-text.h		\
	gnome-icon-entry.h		\
	gnome-init.h			\
	gnome-less.h			\
	gnome-messagebox.h		\
	gnome-mdi.h			\
	gnome-mdi-child.h		\
	gnome-mdi-generic-child.h	\
	gnome-mdi-session.h		\
	gnome-number-entry.h		\
	gnome-popup-menu.h		\
	gnome-popup-help.h		\
	gnome-preferences.h		\
	gnome-propertybox.h		\
	gnome-properties.h     		\
	gnome-property-entries.h	\
	gnome-pixmap.h			\
	gnome-pixmap-entry.h		\
	gnome-scores.h			\
	gnome-spell.h			\
	gnome-startup.h			\
	gnome-stock.h			\
	gnome-types.h			\
	gnome-uidefs.h			\
        gnome-winhints.h		\
        gnome-paper-selector.h		\
	gnome-procbar.h			\
	gtkdial.h			\
	gtk-clock.h			\
	gtkpixmapmenuitem.h		\
	libgnomeui.h

libgnomeuiinclude_HEADERS = \
	$(gnome_headers)	\
	gnometypebuiltins.h

BUILT_SOURCES = \
	gnometypebuiltins.h		\
	gnometypebuiltins_evals.c	\
	gnometypebuiltins_ids.c		\
	gnometypebuiltins_vars.c

# generate gnome.defs file from gnome-boxed.defs and *.h
gnome.defs: @REBUILD@ makeenums.pl gnome-boxed.defs $(gnome_headers)
	cd $(srcdir) \
	&& $(PERL) makeenums.pl defs $(gnome_headers) > gd.tmp \
	&& cat gnome-boxed.defs >> gd.tmp \
	&& mv gd.tmp gnome.defs

# generate type identifier header (GTK_TYPE_WIDGET_FLAGS)
gnometypebuiltins.h: @REBUILD@ gnome.defs maketypes.awk
	cd $(srcdir) \
	&& $(AWK) -f maketypes.awk gnome.defs macros > gtb.tmp \
	&& mv gtb.tmp gnometypebuiltins.h

# generate type identifier variables (GTK_TYPE_WIDGET_FLAGS)
gnometypebuiltins_vars.c: @REBUILD@ gnome.defs maketypes.awk
	cd $(srcdir) \
	&& $(AWK) -f maketypes.awk gnome.defs variables > gtbv.tmp \
	&& mv gtbv.tmp gnometypebuiltins_vars.c

# generate type entries for type-id registration
gnometypebuiltins_ids.c: @REBUILD@ gnome.defs maketypes.awk
	cd $(srcdir) \
	&& $(AWK) -f maketypes.awk gnome.defs entries > gtbi.tmp \
	&& mv gtbi.tmp gnometypebuiltins_ids.c

# generate enum value arrays
gnometypebuiltins_evals.c: @REBUILD@ makeenums.pl gnome.defs
	cd $(srcdir) \
	&& $(PERL) makeenums.pl arrays $(gnome_headers) > gtbe.tmp \
	&& mv gtbe.tmp gnometypebuiltins_evals.c

#######################

gnome_segv_SOURCES = gnome_segv.c

stock_demo_SOURCES = stock_demo.c

winhints_demo_SOURCES = winhints_demo.c

animator_demo_SOURCES = animator_demo.c

dock_demo_SOURCES = dock_demo.c

gtkrcdir = $(datadir)
gtkrc_DATA = gtkrc gtkrc.el gtkrc.eo gtkrc.he gtkrc.hy gtkrc.ja \
	gtkrc.ko gtkrc.ru gtkrc.tr gtkrc.th gtkrc.uk \
	gtkrc.iso88592 gtkrc.iso88595 gtkrc.zh_CN gtkrc.zh_TW.Big5 \
	gtkrc.ka_GE.georgianacademy gtkrc.ka_GE.georgianps \
	gtkrc.vi_VN.tcvn gtkrc.vi_VN.viscii

EXTRA_DIST =					\
	$(gtkrc_DATA)				\
	AUTHORS					\
	gnome.defs 				\
	maketypes.awk				\
	makeenums.pl				\
	gnome-boxed.defs			\
	gnometypebuiltins_vars.c		\
	gnometypebuiltins_ids.c			\
	gnometypebuiltins_evals.c

libgnomeui_la_LDFLAGS = -version-info 43:0:0 -rpath $(libdir)
libgnomeui_la_LIBADD = $(LIBGNOMEUI_LIBS)

gnome_segv_LDADD =	\
	libgnomeui.la 					\
	$(top_builddir)/libgnome/libgnome.la 		\
	$(top_builddir)/libart_lgpl/libart_lgpl.la 	\
	@LIBSUPPORT@ $(LIBGNOMEUI_LIBS) \
	$(INTLLIBS)

stock_demo_LDADD =	\
	libgnomeui.la 					\
	$(top_builddir)/libgnome/libgnome.la 		\
	$(top_builddir)/libart_lgpl/libart_lgpl.la 	\
	@LIBSUPPORT@ $(LIBGNOMEUI_LIBS) \
	$(INTLLIBS)

winhints_demo_LDADD =	\
	libgnomeui.la 					\
	$(top_builddir)/libgnome/libgnome.la 		\
	$(top_builddir)/libart_lgpl/libart_lgpl.la 	\
	@LIBSUPPORT@ $(LIBGNOMEUI_LIBS) \
	$(INTLLIBS)

animator_demo_LDADD =	\
	libgnomeui.la 					\
	$(top_builddir)/libgnome/libgnome.la 		\
	$(top_builddir)/libart_lgpl/libart_lgpl.la 	\
	@LIBSUPPORT@ $(LIBGNOMEUI_LIBS) \
	$(INTLLIBS)

dock_demo_LDADD =	\
	libgnomeui.la 					\
	$(top_builddir)/libgnome/libgnome.la 		\
	$(top_builddir)/libart_lgpl/libart_lgpl.la 	\
	@LIBSUPPORT@ $(LIBGNOMEUI_LIBS) \
	$(INTLLIBS)
