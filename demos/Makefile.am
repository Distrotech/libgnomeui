INCLUDES = \
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/intl				\
	-I$(top_builddir)/intl				\
	-I$(top_srcdir)/libgnomeui			\
	$(WARN_CFLAGS)					\
	$(LIBGNOMECANVAS_CFLAGS)			\
        -DGNOMEUILIBDIR=\""$(libdir)"\" 		\
        -DGNOMEUIDATADIR=\""$(datadir)"\" 		\
        -DGNOMEUIPIXMAPDIR=\""$(datadir)/pixmaps"\"	\
        -DGNOMEUIBINDIR=\""$(bindir)"\" 		\
        -DGNOMEUILOCALSTATEDIR=\""$(localstatedir)"\" 	\
        -DGNOMEUILOCALEDIR=\""$(gnomelocaledir)"\" 	\
	-DG_LOG_DOMAIN=\"GnomeUI\"			\
	-DVERSION=\"$(VERSION)\"

LDADD =	\
	$(top_builddir)/libgnomeui/libgnomeui-2.la \
	$(INTLLIBS)

noinst_PROGRAMS = \
	selector_demo		\
	winhints_demo		\
	dock_demo 		\
	animator_demo		\
	mdi_demo

selector_demo_SOURCES = selector_demo.c

winhints_demo_SOURCES = winhints_demo.c

animator_demo_SOURCES = animator_demo.c

dock_demo_SOURCES = dock_demo.c

mdi_demo_SOURCES = mdi_demo.c

