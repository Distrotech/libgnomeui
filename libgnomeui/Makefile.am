## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

LIB_VERSION=0:0:0
LIB_VERSION_NUM=0.0.0

#
# The targets
#
bin_PROGRAMS    = gnome_segv gnome-remote-bootstrap
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
        -DGNOMEUILIBDIR=\""$(libdir)"\" 		\
        -DGNOMEUIDATADIR=\""$(datadir)"\" 		\
        -DGNOMEUIPIXMAPDIR=\""$(datadir)/pixmaps"\"	\
        -DGNOMEUIBINDIR=\""$(bindir)"\" 		\
        -DGNOMEUILOCALSTATEDIR=\""$(localstatedir)"\" 	\
        -DGNOMEUILOCALEDIR=\""$(gnomelocaledir)"\" 	\
	-DG_LOG_DOMAIN=\"GnomeUI\"			\
	$(WARN_CFLAGS)					\
	$(LIBGNOME_CFLAGS)				\
	$(LIBGNOMECANVAS_CFLAGS)			\
	$(LIBBONOBOUI_CFLAGS)				\
	-DGTK_VERSION=\""$(GTK_VERSION)"\"		\
	-DVERSION=\"$(VERSION)\"

EXTRA_HEADERS = 

noinst_HEADERS = \
	libgnomeuiP.h

EXTRA_PROGRAMS = unused

unused_SOURCES = \
	gnome-mdi.c			\
	gnome-mdiP.h			\
	gnome-mdi-child.c		\
	gnome-mdi-generic-child.c       \
	gnome-mdi-session.c		\
	gnome-popup-menu.c		\
	gnome-popup-help.c		\
	gnome-pouch.c			\
	gnome-pouchP.h			\
	gnome-roo.c			\
	gnome-helpsys.c			\
	gnome-textfu.c			\
	gnome-mdi.h			\
	gnome-mdi-child.h		\
	gnome-mdi-generic-child.h	\
	gnome-mdi-session.h		\
	gnome-popup-menu.h		\
	gnome-popup-help.h		\
	gnome-pouch.h			\
	gnome-roo.h			\
	gnome-helpsys.h			\
	gnome-textfu.h			\
	gnome-ditem-edit.c

libgnomeui_2_la_SOURCES = \
	gnome-stock-icons.c		\
	gnome-about.c			\
	gnome-canvas-init.c		\
	gnome-client.c			\
	gnome-color-picker.c		\
	gnome-cursors.c			\
	gnome-dateedit.c		\
	gnome-druid.c			\
	gnome-druid-page.c		\
	gnome-druid-page-edge.c		\
	gnome-druid-page-standard.c	\
	gnome-geometry.c		\
	gnome-selector-client.c		\
	gnome-selector-component.c	\
	gnome-selector-dialog-client.c	\
	gnome-selector-widget.c		\
	gnome-entry.c			\
	gnome-image-entry.c		\
	gnome-image-selector.c		\
        gnome-font-picker.c		\
	gnome-href.c			\
	gnome-ice.c			\
	gnome-init.c			\
	gnome-less.c			\
	gnome-pixmap.c			\
        gnome-winhints.c		\
	gnometypes.c			\
	gtk-clock.c			\
	gtkdial.c			\
	gnome-icon-list.c		\
	gnome-icon-item.c		\
	gnome-icon-text.c		\
	gnome-vfs-util.c		\
	gnome-window.c			\
	wap-textfu.c			\
	$(noinst_HEADERS)

## this lists all the non-generated headers
gnome_headers = \
	gnome-about.h			\
        gnome-animator.h		\
	gnome-canvas-init.h		\
	gnome-client.h			\
	gnome-color-picker.h		\
	gnome-cursors.h			\
	gnome-dateedit.h		\
	gnome-ditem-edit.h		\
	gnome-druid.h			\
	gnome-druid-page.h		\
	gnome-druid-page-edge.h		\
	gnome-druid-page-standard.h	\
	gnome-selector-client.h		\
	gnome-selector-component.h	\
	gnome-selector-dialog-client.h	\
	gnome-selector-widget.h		\
	gnome-entry.h			\
	gnome-image-entry.h		\
	gnome-image-selector.h		\
        gnome-font-picker.h		\
	gnome-gconf.h			\
	gnome-geometry.h		\
	gnome-href.h			\
	gnome-ice.h			\
	gnome-icon-item.h		\
	gnome-icon-list.h		\
	gnome-icon-text.h		\
	gnome-init.h			\
	gnome-less.h			\
	gnome-macros.h			\
	gnome-pixmap.h			\
	gnome-stock-icons.h		\
	gnome-types.h			\
	gnome-uidefs.h			\
        gnome-winhints.h		\
	gtkdial.h			\
	gtk-clock.h			\
	libgnomeui.h			\
	gnome-vfs-util.h		\
	gnome-window.h			\
	wap-textfu.h			\
	oafgnome.h

libgnomeuiinclude_HEADERS =		\
	$(gnome_headers)		\
	gnometypebuiltins.h

gnome_remote_bootstrap_SOURCES=gnome-remote-bootstrap.c

BUILT_SOURCES = \
	gnometypebuiltins.h		\
	gnometypebuiltins_evals.c	\
	gnometypebuiltins_ids.c		\
	gnometypebuiltins_vars.c

# generate gnome.defs file from gnome-boxed.defs and *.h
gnome.defs: $(GNOME_MAKEENUMS) gnome-boxed.defs $(gnome_headers)
	cd $(srcdir) \
	&& $(GNOME_MAKEENUMS) defs $(gnome_headers) > gd.tmp \
	&& cat gnome-boxed.defs >> gd.tmp \
	&& mv gd.tmp gnome.defs

# generate type identifier header (GTK_TYPE_WIDGET_FLAGS)
gnometypebuiltins.h: gnome.defs $(GNOME_MAKETYPES)
	cd $(srcdir) \
	&& LC_COLLATE=C $(GNOME_MAKETYPES) gnome.defs macros > gtb.tmp \
	&& mv gtb.tmp gnometypebuiltins.h

# generate type identifier variables (GTK_TYPE_WIDGET_FLAGS)
gnometypebuiltins_vars.c: gnome.defs $(GNOME_MAKETYPES)
	cd $(srcdir) \
	&& LC_COLLATE=C $(GNOME_MAKETYPES) gnome.defs variables > gtbv.tmp \
	&& mv gtbv.tmp gnometypebuiltins_vars.c

# generate type entries for type-id registration
gnometypebuiltins_ids.c: gnome.defs $(GNOME_MAKETYPES)
	cd $(srcdir) \
	&& LC_COLLATE=C $(GNOME_MAKETYPES) gnome.defs entries > gtbi.tmp \
	&& mv gtbi.tmp gnometypebuiltins_ids.c

# generate enum value arrays
gnometypebuiltins_evals.c: $(GNOME_MAKEENUMS) gnome.defs
	cd $(srcdir) \
	&& $(GNOME_MAKEENUMS) arrays $(gnome_headers) > gtbe.tmp \
	&& mv gtbe.tmp gnometypebuiltins_evals.c

#######################

gnome_segv_SOURCES = gnome_segv.c

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
	gnome-boxed.defs			\
	gnometypebuiltins_vars.c		\
	gnometypebuiltins_ids.c			\
	gnometypebuiltins_evals.c		\
	portrait.xpm				\
	landscape.xpm				\
	libgnomeui-2.0.pc.in

libgnomeui_2_la_LDFLAGS = \
	-version-info $(LIB_VERSION)

libgnomeui_2_la_LIBADD = \
	$(LIBGNOME_LIBS)			\
	$(LIBGNOMECANVAS_LIBS)			\
	$(LIBBONOBOUI_LIBS)

LDADD = \
	libgnomeui-2.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgnomeui-2.0.pc
