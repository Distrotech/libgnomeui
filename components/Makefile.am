#
# The targets
#
componentdir = $(libdir)/gnomeui-2.0/components

component_LTLIBRARIES = libgnomeui-components-2.la

LIB_VERSION=0:0:0
LIB_VERSION_NUM=0.0.0

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
	-DGNOMEUIXMLDBFILE=\""$(componentdir)/selector-components.xmldb"\" \
	-DG_LOG_DOMAIN=\"GnomeUI-Components\"		\
	$(WARN_CFLAGS)					\
	$(LIBGNOMEUI_COMPONENTS_CFLAGS)			\
	-DGTK_VERSION=\""$(GTK_VERSION)"\"		\
	-DVERSION=\"$(VERSION)\"

OAF_FILES = 						\
	GNOME_UI_Components.oaf

oafdir = $(datadir)/oaf
oaf_DATA = $(OAF_FILES)

GNOME_UI_Components.oaf : $(srcdir)/GNOME_UI_Components.oaf.in $(top_builddir)/config.status
	sed -e "s|\@COMPONENTS_LIBDIR\@|$(componentdir)|" \
	$(srcdir)/GNOME_UI_Components.oaf.in > GNOME_UI_Components.oaf 

clean-local:
	-rm -f $(OAF_FILES)

libgnomeui_components_2_la_SOURCES = \
	libgnomeui-components.c

libgnomeui_components_2_la_LDFLAGS = \
	-version-info $(LIB_VERSION)

libgnomeui_components_2_la_LIBADD = \
	../libgnomeui/libgnomeui-2.la		\
	$(LIBGNOMEUI_COMPONENTS_LIBS)

