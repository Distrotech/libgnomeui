INCLUDES = \
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/intl				\
	-I$(top_builddir)/intl				\
	-I$(top_srcdir)/libgnomeui			\
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

LDADD =	\
	$(top_builddir)/libgnomeui/libgnomeui-$(GNOME_INTERFACE_VERSION).la \
	$(top_builddir)/libgnome/libgnome-$(GNOME_INTERFACE_VERSION).la	\
        $(LIBGNOMEUI_LIBS)						\
	$(LIBART_LIBS)							\
	$(INTLLIBS)

noinst_PROGRAMS = \
	selector_demo		\
	stock_demo		\
	winhints_demo		\
	dock_demo 		\
	animator_demo		\
	mdi_demo

selector_demo_SOURCES = selector_demo.c

stock_demo_SOURCES = stock_demo.c

winhints_demo_SOURCES = winhints_demo.c

animator_demo_SOURCES = animator_demo.c

dock_demo_SOURCES = dock_demo.c

mdi_demo_SOURCES = mdi_demo.c

