## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

#
# The targets
#
noinst_PROGRAMS = stock_demo winhints_demo  dock_demo #animator_demo
bin_PROGRAMS    = gnome_segv gnome-remote-bootstrap gnome-rexec-server
lib_LTLIBRARIES = libgnomeui.la

if COMPILE_GNOME_COMPAT
lib_LIBRARIES = libgnomeui10compat.a
endif

#
# Install location for header files
#
libgnomeuiincludedir = $(includedir)/libgnomeui

INCLUDES = -I$(top_srcdir)/gdk-pixbuf			\
	-I.. -I$(srcdir)/.. $(SUPPORTINCS) 		\
	$(OAF_CFLAGS)					\
	-I$(top_srcdir)/intl -I$(top_builddir)/intl 	\
        -DGNOMELIBDIR=\""$(libdir)"\" 			\
        -DGNOMEDATADIR=\""$(datadir)"\" 		\
        -DGNOMEBINDIR=\""$(bindir)"\" 			\
        -DGNOMELOCALSTATEDIR=\""$(localstatedir)"\" 	\
        -DGNOMELOCALEDIR=\""$(gnomelocaledir)"\" 	\
	-DG_LOG_DOMAIN=\"GnomeUI\"			\
	$(GTK_CFLAGS) $(ORBIT_CFLAGS)			\
	-DVERSION=\"$(VERSION)\"

gnome_corba_rexec_built = 		\
	gnome-corba-rexec.h		\
	gnome-corba-rexec-skels.c	\
	gnome-corba-rexec-stubs.c	\
	gnome-corba-rexec-common.c

gnome_unknown_built = \
	gnome-unknown.h					\
	gnome-unknown-skels.c				\
	gnome-unknown-stubs.c				\
	gnome-unknown-common.c

history_db_built = \
	HistoryDB.h \
	HistoryDB-stubs.c \
	HistoryDB-skels.c \
	HistoryDB-common.c

$(gnome_corba_rexec_built): $(top_srcdir)/idl/gnome-corba-rexec.idl
	$(ORBIT_IDL) $(IDL_FLAGS) $(top_srcdir)/idl/gnome-corba-rexec.idl

$(gnome_unknown_built): $(top_srcdir)/idl/gnome-unknown.idl
	$(ORBIT_IDL) $(IDL_FLAGS) $(top_srcdir)/idl/gnome-unknown.idl

$(history_db_built): $(top_srcdir)/idl/HistoryDB.idl
	$(ORBIT_IDL) $(IDL_FLAGS) $(top_srcdir)/idl/HistoryDB.idl

gnorba_built = $(gnome_unknown_built) $(history_db_built) $(gnome_corba_rexec_built)

if COMPILE_GNOME_COMPAT
libgnomeui10compatincludedir = $(includedir)/libgnomeui/compat10

libgnomeui10compat_a_SOURCES =\
	gnome-font-selector.c 		\
	gnome-guru.c			\
	gnome-spell.c			\
	gnome-startup.c			\
	gtkcauldron.c			\
	gnome-canvas-image.c		\
	gnome-canvas-load.c		\
	gnomeui10-compat.c		\
	gnome-druid-page-start.c	\
	gnome-druid-page-finish.c	\
	gnome-icon-list.c		\
	gnome-icon-item.c		\
	gnome-icon-sel.c		\
	gnome-icon-text.c

libgnomeui10compatinclude_HEADERS = \
	libgnomeui10compat.h		\
	gnome-font-selector.h 		\
	gnome-guru.h			\
	gnome-spell.h			\
	gnome-startup.h			\
	gtkcauldron.h			\
	gnome-canvas-image.h		\
	gnome-canvas-load.h		\
	gnomeui10-compat.h		\
	gnome-druid-page-start.h	\
	gnome-druid-page-finish.h	\
	gnome-icon-list.h		\
	gnome-icon-item.h		\
	gnome-icon-sel.h		\
	gnome-icon-text.h

endif

EXTRA_HEADERS = 

libgnomeui_la_SOURCES = \
	gnome-about.c			\
##	gnome-animator.c		\
	gnome-app.c			\
	gnome-appbar.c			\
	gnome-app-helper.c		\
	gnome-app-util.c		\
	gnome-calculator.c              \
	gnome-canvas.c			\
	gnome-canvas-line.c		\
	gnome-canvas-rect-ellipse.c	\
	gnome-canvas-pixbuf.c		\
	gnome-canvas-polygon.c		\
	gnome-canvas-text.c		\
	gnome-canvas-util.c		\
	gnome-canvas-widget.c		\
	gnome-client.c			\
	gnome-color-picker.c		\
	gnome-dateedit.c		\
##	gnome-dentry-edit.c		\
	gnome-dialog.c                  \
	gnome-dialog-util.c		\
	gnome-dns.c			\
	gnome-dock.c			\
	gnome-dock-band.c		\
	gnome-dock-item.c		\
	gnome-dock-layout.c		\
	gnome-druid.c			\
	gnome-druid-page.c		\
	gnome-druid-page-edge.c	\
	gnome-druid-page-standard.c	\
	gnome-geometry.c		\
	gnome-entry.c			\
	gnome-file-entry.c		\
        gnome-font-picker.c		\
	gnome-helpsys.c			\
	gnome-href.c			\
	gnome-ice.c			\
##	gnome-icon-entry.c		\
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
##	gnome-properties.c		\
##	gnome-property-entries.c	\
	gnome-scores.c			\
	gnome-stock.c			\
	gnome-stock-ids.c		\
        gnome-winhints.c		\
        gnome-paper-selector.c		\
	gnome-procbar.c			\
	gnometypes.c			\
	gtk-clock.c			\
	gtkdial.c			\
	gtkpixmapmenuitem.c		\
	oafgnome.c			\
	$(gnorba_built)

## this lists all the non-generated headers
gnome_headers = \
	gnome-about.h			\
##        gnome-animator.h		\
	gnome-app.h			\
	gnome-appbar.h			\
	gnome-app-helper.h		\
	gnome-app-util.h		\
	gnome-calculator.h              \
	gnome-canvas.h			\
	gnome-canvas-line.h		\
	gnome-canvas-rect-ellipse.h	\
	gnome-canvas-pixbuf.h		\
	gnome-canvas-polygon.h		\
	gnome-canvas-text.h		\
	gnome-canvas-util.h		\
	gnome-canvas-widget.h		\
	gnome-client.h			\
	gnome-color-picker.h		\
	gnome-dateedit.h		\
##	gnome-dentry-edit.h		\
	gnome-dialog.h                  \
	gnome-dialog-util.h		\
	gnome-dns.h			\
	gnome-dock.h			\
	gnome-dock-band.h		\
	gnome-dock-item.h		\
	gnome-dock-layout.h		\
	gnome-druid.h			\
	gnome-druid-page.h		\
	gnome-druid-page-edge.h	\
	gnome-druid-page-standard.h	\
	gnome-entry.h			\
	gnome-file-entry.h		\
        gnome-font-picker.h		\
	gnome-geometry.h		\
	gnome-helpsys.h			\
	gnome-href.h			\
	gnome-ice.h			\
##	gnome-icon-entry.h		\
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
	gnome-stock.h			\
	gnome-stock-ids.h		\
	gnome-types.h			\
	gnome-uidefs.h			\
        gnome-winhints.h		\
        gnome-paper-selector.h		\
	gnome-procbar.h			\
	gtkdial.h			\
	gtk-clock.h			\
	gtkpixmapmenuitem.h		\
	libgnomeui.h			\
	oafgnome.h

libgnomeuiinclude_HEADERS =		\
	$(gnome_headers)		\
	gnome-unknown.h			\
	HistoryDB.h			\
	gnome-corba-rexec.h		\
	gnometypebuiltins.h

ORBIT_IDL=orbit-idl

IDL_FLAGS=-D__LIBOAFGNOME_COMPILATION -D__LIBGNORBA_COMPILATION

gnome_remote_bootstrap_SOURCES=gnome-remote-bootstrap.c
gnome_remote_bootstrap_LDADD=../libgnome/libgnome.la

gnome_rexec_server_SOURCES=gnome-rexec-server.c
gnome_rexec_server_LDADD=../libgnome/libgnome.la gnome-corba-rexec-skels.o gnome-corba-rexec-common.o $(OAF_LIBS)

BUILT_SOURCES = \
	gnometypebuiltins.h		\
	gnometypebuiltins_evals.c	\
	gnometypebuiltins_ids.c		\
	gnometypebuiltins_vars.c	\
	$(gnorba_built)

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

#animator_demo_SOURCES = animator_demo.c

dock_demo_SOURCES = dock_demo.c

gtkrcdir = $(datadir)
gtkrc_DATA = gtkrc gtkrc.el gtkrc.eo gtkrc.he gtkrc.hy gtkrc.ja \
	gtkrc.ko gtkrc.ru gtkrc.tr gtkrc.th gtkrc.uk \
	gtkrc.iso88592 gtkrc.iso88595 gtkrc.zh_CN gtkrc.zh_TW.Big5 \
	gtkrc.ka_GE.georgianacademy gtkrc.ka_GE.georgianps \
	gtkrc.vi_VN.tcvn gtkrc.vi_VN.viscii

oafinfodir=$(datadir)/oaf
oafinfo_DATA=gnome-rexec-server.oafinfo

EXTRA_DIST =					\
	$(gtkrc_DATA)				\
	AUTHORS					\
	gnome.defs 				\
	maketypes.awk				\
	makeenums.pl				\
	gnome-boxed.defs			\
	gnometypebuiltins_vars.c		\
	gnometypebuiltins_ids.c			\
	gnometypebuiltins_evals.c		\
	$(oafinfo_DATA)

libgnomeui_la_LDFLAGS = -version-info 43:0:0 -rpath $(libdir) \
		-L`cd $(top_builddir); pwd`/gdk-pixbuf/src/.libs -lgdk_pixbuf \
		-L`cd $(top_builddir); pwd`/libgnome/.libs -lgnome \
		-L`cd $(top_builddir); pwd`/libart_lgpl/.libs -lart_lgpl \
		$(OAF_LIBS)					\
		$(LIBGNOMEUI_LIBS)
libgnomeui_la_LIBADD = $(LIBGNOMEUI_LIBS)

gnome_segv_LDADD =	\
	libgnomeui.la 					\
	$(top_builddir)/libgnome/libgnome.la 		\
	$(top_builddir)/libart_lgpl/libart_lgpl.la 	\
	$(top_builddir)/gdk-pixbuf/src/libgdk_pixbuf.la \
        $(LIBGNOMEUI_LIBS) \
	$(INTLLIBS)


stock_demo_LDADD =	\
	libgnomeui.la 					\
	$(top_builddir)/libgnome/libgnome.la 		\
	$(top_builddir)/libart_lgpl/libart_lgpl.la 	\
	$(top_builddir)/gdk-pixbuf/src/libgdk_pixbuf.la \
	$(LIBGNOMEUI_LIBS) \
	$(INTLLIBS)

#	$(top_builddir)/libgnome/libgnome.la 		\
#	$(top_builddir)/libart_lgpl/libart_lgpl.la 	\
#	$(LIBGNOMEUI_LIBS)

winhints_demo_LDADD =	\
	libgnomeui.la 					\
	$(top_builddir)/libgnome/libgnome.la 		\
	$(top_builddir)/libart_lgpl/libart_lgpl.la 	\
	$(top_builddir)/gdk-pixbuf/src/libgdk_pixbuf.la \
	$(LIBGNOMEUI_LIBS) \
	$(INTLLIBS)

#animator_demo_LDADD =	\
#	libgnomeui.la 					\
#	$(top_builddir)/libgnome/libgnome.la 		\
#	$(top_builddir)/libart_lgpl/libart_lgpl.la 	\
#	$(top_builddir)/gdk-pixbuf/src/libgdk_pixbuf.la \
#	$(LIBGNOMEUI_LIBS) \
#	$(INTLLIBS)

dock_demo_LDADD =	\
	libgnomeui.la 					\
	$(top_builddir)/libgnome/libgnome.la 		\
	$(top_builddir)/libart_lgpl/libart_lgpl.la 	\
	$(top_builddir)/gdk-pixbuf/src/libgdk_pixbuf.la \
	$(LIBGNOMEUI_LIBS) \
	$(INTLLIBS)
