## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

LIB_VERSION=0:0:0
LIB_VERSION_NUM=0.0.0

#
# The targets
#
bin_PROGRAMS    = gnome_segv gnome-remote-bootstrap gnome-rexec-server
lib_LTLIBRARIES = libgnomeui-2.la

#
# Install location for header files
#
libgnomeuiincludedir = $(includedir)/gnome/$(GNOME_INTERFACE_VERSION)/libgnomeui

INCLUDES = \
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/intl				\
	-I$(top_builddir)/intl				\
	$(WARN_CFLAGS)					\
	$(GNOMESUPPORT_CFLAGS)				\
	$(GLIB_CFLAGS)					\
	$(GTK_CFLAGS)					\
	$(OAF_CFLAGS)					\
	$(LIBART_CFLAGS)				\
        -DGNOMELIBDIR=\""$(libdir)"\" 			\
        -DGNOMEDATADIR=\""$(datadir)"\" 		\
        -DGNOMEBINDIR=\""$(bindir)"\" 			\
        -DGNOMELOCALSTATEDIR=\""$(localstatedir)"\" 	\
        -DGNOMELOCALEDIR=\""$(gnomelocaledir)"\" 	\
	-DG_LOG_DOMAIN=\"GnomeUI\"			\
	$(ORBIT_CFLAGS)					\
	$(GCONF_CFLAGS)					\
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

$(gnome_corba_rexec_built): $(top_srcdir)/idl/gnome-corba-rexec.idl
	$(ORBIT_IDL) $(IDL_FLAGS) $(top_srcdir)/idl/gnome-corba-rexec.idl

$(gnome_unknown_built): $(top_srcdir)/idl/gnome-unknown.idl
	$(ORBIT_IDL) $(IDL_FLAGS) $(top_srcdir)/idl/gnome-unknown.idl

gnorba_built = $(gnome_unknown_built) $(gnome_corba_rexec_built)

EXTRA_HEADERS = 

noinst_HEADERS = \
	libgnomeuiP.h

libgnomeui_2_la_SOURCES = \
	gnome-about.c			\
	gnome-animator.c		\
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
	gnome-cursors.c			\
	gnome-dateedit.c		\
	gnome-ditem-edit.c		\
	gnome-dialog.c                  \
	gnome-dialog-util.c		\
	gnome-dock.c			\
	gnome-dock-band.c		\
	gnome-dock-item.c		\
	gnome-dock-layout.c		\
	gnome-druid.c			\
	gnome-druid-page.c		\
	gnome-druid-page-edge.c		\
	gnome-druid-page-standard.c	\
	gnome-geometry.c		\
	gnome-entry.c			\
	gnome-selector.c		\
	gnome-selectorP.h		\
	gnome-icon-selector.c		\
	gnome-file-selector.c		\
	gnome-file-entry.c		\
        gnome-font-picker.c		\
	gnome-gconf.c			\
	gnome-helpsys.c			\
	gnome-href.c			\
	gnome-ice.c			\
	gnome-icon-entry.c		\
	gnome-init.c			\
	gnome-less.c			\
	gnome-mdi.c			\
	gnome-mdiP.h			\
	gnome-mdi-child.c		\
	gnome-mdi-generic-child.c       \
	gnome-mdi-session.c		\
	gnome-messagebox.c		\
	gnome-number-entry.c		\
	gnome-pixmap.c			\
	gnome-pixmap-entry.c		\
	gnome-popup-menu.c		\
	gnome-popup-help.c		\
	gnome-pouch.c			\
	gnome-pouchP.h			\
	gnome-preferences.c		\
	gnome-propertybox.c		\
	gnome-roo.c			\
	gnome-scores.c			\
	gnome-stock.c			\
	gnome-stock-ids.c		\
        gnome-winhints.c		\
        gnome-paper-selector.c		\
	gnome-unit-spinner.c		\
	gnometypes.c			\
	gtk-clock.c			\
	gtkdial.c			\
	gtkpixmapmenuitem.c		\
	oafgnome.c			\
	gnome-icon-list.c		\
	gnome-icon-item.c		\
	gnome-icon-sel.c		\
	gnome-icon-text.c		\
	gnome-vfs-util.c		\
	gnome-textfu.c			\
	gnome-window.c			\
	gnomemarshal-main.c		\
	wap-textfu.c			\
	$(noinst_HEADERS)		\
	$(gnorba_built)

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
	gnome-canvas-line.h		\
	gnome-canvas-rect-ellipse.h	\
	gnome-canvas-pixbuf.h		\
	gnome-canvas-polygon.h		\
	gnome-canvas-text.h		\
	gnome-canvas-util.h		\
	gnome-canvas-widget.h		\
	gnome-client.h			\
	gnome-color-picker.h		\
	gnome-cursors.h			\
	gnome-dateedit.h		\
	gnome-ditem-edit.h		\
	gnome-dialog.h                  \
	gnome-dialog-util.h		\
	gnome-dock.h			\
	gnome-dock-band.h		\
	gnome-dock-item.h		\
	gnome-dock-layout.h		\
	gnome-druid.h			\
	gnome-druid-page.h		\
	gnome-druid-page-edge.h		\
	gnome-druid-page-standard.h	\
	gnome-entry.h			\
	gnome-file-entry.h		\
	gnome-file-selector.h		\
        gnome-font-picker.h		\
	gnome-gconf.h			\
	gnome-geometry.h		\
	gnome-helpsys.h			\
	gnome-href.h			\
	gnome-ice.h			\
	gnome-icon-entry.h		\
	gnome-icon-item.h		\
	gnome-icon-list.h		\
	gnome-icon-sel.h		\
	gnome-icon-selector.h		\
	gnome-icon-text.h		\
	gnome-init.h			\
	gnome-less.h			\
	gnome-macros.h			\
	gnome-messagebox.h		\
	gnome-mdi.h			\
	gnome-mdi-child.h		\
	gnome-mdi-generic-child.h	\
	gnome-mdi-session.h		\
	gnome-number-entry.h		\
	gnome-popup-menu.h		\
	gnome-popup-help.h		\
	gnome-pouch.h			\
        gnome-paper-selector.h		\
	gnome-preferences.h		\
	gnome-propertybox.h		\
	gnome-pixmap.h			\
	gnome-pixmap-entry.h		\
	gnome-roo.h			\
	gnome-scores.h			\
	gnome-selector.h		\
	gnome-stock.h			\
	gnome-stock-ids.h		\
	gnome-types.h			\
	gnome-uidefs.h			\
	gnome-unit-spinner.h		\
        gnome-winhints.h		\
	gtkdial.h			\
	gtk-clock.h			\
	gtkpixmapmenuitem.h		\
	libgnomeui.h			\
	gnome-vfs-util.h		\
	gnome-textfu.h			\
	gnome-window.h			\
	wap-textfu.h			\
	oafgnome.h

libgnomeuiinclude_HEADERS =		\
	$(gnome_headers)		\
	gnome-unknown.h			\
	gnome-corba-rexec.h		\
	gnometypebuiltins.h

ORBIT_IDL=orbit-idl

IDL_FLAGS=-D__LIBOAFGNOME_COMPILATION -D__LIBGNORBA_COMPILATION

gnome_remote_bootstrap_SOURCES=gnome-remote-bootstrap.c
gnome_remote_bootstrap_LDADD=../libgnome/libgnome-$(GNOME_INTERFACE_VERSION).la

gnome_rexec_server_SOURCES=gnome-rexec-server.c
gnome_rexec_server_LDADD=../libgnome/libgnome-$(GNOME_INTERFACE_VERSION).la gnome-corba-rexec-skels.o gnome-corba-rexec-common.o $(OAF_LIBS)

BUILT_SOURCES = \
	gnometypebuiltins.h		\
	gnometypebuiltins_evals.c	\
	gnometypebuiltins_ids.c		\
	gnometypebuiltins_vars.c	\
	gnomemarshal.h			\
	gnomemarshal.c			\
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
	&& LC_COLLATE=C $(AWK) -f maketypes.awk gnome.defs macros > gtb.tmp \
	&& mv gtb.tmp gnometypebuiltins.h

# generate type identifier variables (GTK_TYPE_WIDGET_FLAGS)
gnometypebuiltins_vars.c: @REBUILD@ gnome.defs maketypes.awk
	cd $(srcdir) \
	&& LC_COLLATE=C $(AWK) -f maketypes.awk gnome.defs variables > gtbv.tmp \
	&& mv gtbv.tmp gnometypebuiltins_vars.c

# generate type entries for type-id registration
gnometypebuiltins_ids.c: @REBUILD@ gnome.defs maketypes.awk
	cd $(srcdir) \
	&& LC_COLLATE=C $(AWK) -f maketypes.awk gnome.defs entries > gtbi.tmp \
	&& mv gtbi.tmp gnometypebuiltins_ids.c

# generate enum value arrays
gnometypebuiltins_evals.c: @REBUILD@ makeenums.pl gnome.defs
	cd $(srcdir) \
	&& $(PERL) makeenums.pl arrays $(gnome_headers) > gtbe.tmp \
	&& mv gtbe.tmp gnometypebuiltins_evals.c

stamp-gnomemarshal.h: @REBUILD@ gnomemarshal.list
	cd $(srcdir) \
	&& glib-genmarshal --prefix=gnome_marshal gnomemarshal.list --header >> xgen-gmh \
	&& (cmp -s xgen-gmh gnomemarshal.h || cp xgen-gmh gnomemarshal.h) \
	&& rm -f xgen-gmh xgen-gmh~ \
	&& echo timestamp > $(@F)

gnomemarshal.h gnomemarshal.c: @REBUILD@ stamp-gnomemarshal.h
	cd $(srcdir) \
	&& glib-genmarshal --prefix=gnome_marshal gnomemarshal.list --body >> xgen-gmc \
	&& cp xgen-gmc gnomemarshal.c \
	&& rm -f xgen-gmc xgen-gmc~

#######################

gnome_segv_SOURCES = gnome_segv.c

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
	gnomemarshal.list			\
	gnome-boxed.defs			\
	gnometypebuiltins_vars.c		\
	gnometypebuiltins_ids.c			\
	gnometypebuiltins_evals.c		\
	portrait.xpm				\
	landscape.xpm				\
	$(oafinfo_DATA)

libgnomeui_2_la_LDFLAGS = \
	-version-info $(LIB_VERSION)				\
	$(OAF_LIBS)						\
	$(GCONF_LIBS)						\
	$(LIBGNOMEUI_LIBS)

#		-L`cd $(top_builddir); pwd`/libgnome/.libs -lgnome-$(GNOME_INTERFACE_VERSION)

libgnomeui_2_la_LIBADD = $(LIBGNOMEUI_LIBS) -lICE -lSM -lz -lxml

LDADD =	\
	libgnomeui-$(GNOME_INTERFACE_VERSION).la 			\
	$(top_builddir)/libgnome/libgnome-$(GNOME_INTERFACE_VERSION).la	\
        $(LIBGNOMEUI_LIBS)						\
	$(LIBART_LIBS)							\
	$(INTLLIBS)
