## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

#
# The targets
#
lib_LTLIBRARIES = libgnomeui-2.la

gnomehdir = $(includedir)/libgnomeui-2.0
gnomeh_HEADERS = gnome.h

#
# Install location for header files
#
libgnomeuiincludedir = $(includedir)/libgnomeui-2.0/libgnomeui

INCLUDES = \
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-DLIBGNOMEUI_PREFIX=\""$(prefix)"\"		\
        -DLIBGNOMEUI_DATADIR=\""$(datadir)"\" 		\
        -DLIBGNOMEUI_SERVERDIR=\""$(libexecdir)"\" 	\
	-DG_LOG_DOMAIN=\"GnomeUI\"			\
	-DGNOMEUILOCALEDIR="\"$(datadir)/locale\""	\
	-DGNOME_EXPLICIT_TRANSLATION_DOMAIN="GETTEXT_PACKAGE"	\
	$(WARN_CFLAGS)					\
	$(LIBGNOMEUI_CFLAGS)

EXTRA_HEADERS = 

if OS_WIN32
platform_dep_sources = \
	gnome-ui-win32.c
else
platform_dep_sources = \
	gnome-authentication-manager.c  \
	gnome-authentication-manager.h  \
	gnome-authentication-manager-private.h
endif


libgnomeui_2_la_SOURCES = \
	libgnomeui-access.c		\
	libgnomeui-access.h		\
	gnometypebuiltins.h		\
	gnometypebuiltins.c		\
	gnome-about.c			\
	gnome-app.c			\
	gnome-app-helper.c		\
	gnome-app-util.c		\
	gnome-appbar.c			\
	gnome-stock-icons.c		\
	gnome-client.c			\
	gnome-color-picker.c		\
	gnome-dateedit.c		\
	gnome-dialog.c			\
	gnome-dialog-util.c		\
	gnome-druid.c			\
	gnome-druid-page.c		\
	gnome-druid-page-edge.c		\
	gnome-druid-page-standard.c	\
	gnome-entry.c			\
	gnome-file-entry.c		\
	gnome-font-picker.c		\
	gnome-gconf-ui.c		\
	gnome-href.c			\
	gnome-ice.c			\
	gnome-marshal-main.c		\
	gnome-messagebox.c		\
	gnome-mdi.c			\
	gnome-mdi-child.c		\
	gnome-mdi-generic-child.c	\
	gnome-mdi-session.c		\
	gnome-multiscreen.c		\
	gnome-pixmap.c			\
	gnome-pixmap-entry.c		\
	gnome-popup-menu.c		\
	gnome-propertybox.c		\
	gnome-scores.c			\
	gnome-theme-parser.c		\
	gnome-thumbnail.c		\
	gnome-thumbnail-pixbuf-utils.c	\
	gnome-ui-init.c			\
	gnometypes.c			\
	gnome-icon-entry.c		\
	gnome-icon-item.c		\
	gnome-icon-list.c		\
	gnome-icon-lookup.c		\
	gnome-icon-sel.c		\
	gnome-icon-theme.c		\
	gnome-vfs-util.c		\
	gnome-window.c			\
	gnome-window-icon.c		\
	gnome-gconf-ui.h		\
	libgnomeuiP.h			\
	gnome-password-dialog.c         \
	gnome-password-dialog.h         \
	$(platform_dep_sources)

## this lists all the non-generated headers
gnome_headers = \
	gnome-about.h			\
	gnome-app.h			\
	gnome-app-helper.h		\
	gnome-app-util.h		\
	gnome-appbar.h			\
	gnome-authentication-manager.h  \
	gnome-client.h			\
	gnome-color-picker.h		\
	gnome-dateedit.h		\
	gnome-dialog.h			\
	gnome-dialog-util.h		\
	gnome-druid.h			\
	gnome-druid-page.h		\
	gnome-druid-page-edge.h		\
	gnome-druid-page-standard.h	\
	gnome-entry.h			\
	gnome-file-entry.h		\
	gnome-font-picker.h		\
	gnome-help.h			\
	gnome-href.h			\
	gnome-ice.h			\
	gnome-icon-entry.h		\
	gnome-icon-item.h		\
	gnome-icon-list.h		\
	gnome-icon-lookup.h		\
	gnome-icon-sel.h		\
	gnome-icon-theme.h		\
	gnome-messagebox.h		\
	gnome-mdi.h			\
	gnome-mdi-child.h		\
	gnome-mdi-generic-child.h	\
	gnome-mdi-session.h		\
	gnome-password-dialog.h         \
	gnome-pixmap.h			\
	gnome-pixmap-entry.h		\
	gnome-popup-menu.h		\
	gnome-propertybox.h		\
	gnome-scores.h			\
	gnome-ui-init.h			\
	gnome-stock-icons.h		\
	gnome-thumbnail.h		\
	gnome-types.h			\
	gnome-uidefs.h			\
	gnome-url.h			\
	gnome-vfs-util.h		\
	gnome-window.h			\
	gnome-window-icon.h		\
	libgnomeui.h

gnome_private_headers = \
	gnome-theme-parser.h


libgnomeuiinclude_HEADERS =		\
	$(gnome_headers)		\
	$(gnome_private_headers)	\
	gnometypebuiltins.h

BUILT_SOURCES = gnometypebuiltins.h gnometypebuiltins.c gnometype_inits.c \
  gnome-marshal.h gnome-marshal.c

gnometypebuiltins.h: stamp-gnometypebuiltins.h
	@true
stamp-gnometypebuiltins.h: @REBUILD@ $(gnome_headers)
	$(AM_V_GEN) (cd $(srcdir) \
	&& glib-mkenums \
			--fhead "#ifndef __GNOMETYPEBUILTINS_H__\n" \
			--fhead "#define __GNOMETYPEBUILTINS_H__ 1\n\n" \
			--fhead "#include <glib-object.h>\n\n" \
			--fhead "G_BEGIN_DECLS\n\n" \
			--ftail "G_END_DECLS\n\n" \
			--ftail "#endif /* __GNOMETYPEBUILTINS_H__ */\n" \
			--fprod "\n/* --- @filename@ --- */" \
			--eprod "#define GNOME_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
			--eprod "GType @enum_name@_get_type (void);\n" \
		$(gnome_headers) ) > xgen-gth \
	&& (cmp -s xgen-gth gnometypebuiltins.h || cp xgen-gth gnometypebuiltins.h) \
	&& rm -f xgen-gth \
	&& echo timestamp > $(@F)

gnometypebuiltins.c: @REBUILD@ $(gnome_headers) gnometypebuiltins.h
	$(AM_V_GEN) (cd $(srcdir) \
	&& glib-mkenums \
			--fhead "#include \"libgnomeui.h\"\n\n" \
			--fhead "#include \"gnometypebuiltins.h\"\n\n" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n" \
			--vhead "  static GType type = 0;\n\n" \
			--vhead "  if (G_UNLIKELY (type == 0))\n  {\n" \
			--vhead "    static const G@Type@Value _@enum_name@_values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n\n" \
			--vtail "    type = g_@type@_register_static (\"@EnumName@\", _@enum_name@_values);\n  }\n\n" \
			--vtail "  return type;\n}\n\n" \
		$(gnome_headers) ) > xgen-gtc \
	&& cp xgen-gtc $(@F) \
	&& rm -f xgen-gtc

gnometype_inits.c: @REBUILD@ $(gnome_headers)
	$(AM_V_GEN) (cd $(srcdir) \
	&& glib-mkenums \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vtail "    (void) @enum_name@_get_type ();\n" \
		$(gnome_headers) ) > xgen-gtic \
	&& cp xgen-gtic $(@F) \
	&& rm -f xgen-gtic

gnome-marshal.h: stamp-gnome-marshal.h
	$(AM_V_GEN) true
stamp-gnome-marshal.h: @REBUILD@ gnome-marshal.list
	$(AM_V_GEN) glib-genmarshal --prefix=_gnome_marshal $(srcdir)/gnome-marshal.list --header >> xgen-gmh \
	&& (cmp -s xgen-gmh gnome-marshal.h || cp xgen-gmh gnome-marshal.h) \
	&& rm -f xgen-gmh xgen-gmh~ \
	&& echo timestamp > $(@F)

gnome-marshal.c: @REBUILD@ gnome-marshal.list
	$(AM_V_GEN) glib-genmarshal --prefix=_gnome_marshal $(srcdir)/gnome-marshal.list --body >> xgen-gmc \
	&& cp xgen-gmc gnome-marshal.c \
	&& rm -f xgen-gmc xgen-gmc~

MAINTAINERCLEANFILES = stamp-gnometypebuiltins.h stamp-gnome-marshal.h \
  $(BUILT_SOURCES)
CLEANFILES = xgen-gmc xgen-gmh xgen-gth xgen-gtc xgen-gtic

# if srcdir!=builddir, clean out maintainer-clean files from builddir
# this allows dist to pass.
distclean-local:
	if test $(srcdir) != .; then \
	  rm -f $(MAINTAINERCLEANFILES); \
	fi

#######################

EXTRA_DIST =					\
	AUTHORS					\
	gnome-marshal.list			\
	gnome-boxed.defs			\
	gnometype_inits.c			\
	libgnomeui-2.0.pc.in			\
	$(BUILT_SOURCES)

libgnomeui_2_la_LDFLAGS = \
	-version-info $(LIBGNOMEUI_CURRENT):$(LIBGNOMEUI_REVISION):$(LIBGNOMEUI_AGE) \
	-no-undefined \
	-export-symbols-regex "^[^_].*"

libgnomeui_2_la_LIBADD = \
	$(LIBGNOMEUI_LIBS)			\
	$(SM_LIBS)				\
	$(X11_LIBS)

LDADD = \
	libgnomeui-2.la $(LIBGNOMEUI_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgnomeui-2.0.pc

