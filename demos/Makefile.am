INCLUDES = \
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/intl				\
	-I$(top_builddir)/intl				\
	-I$(top_srcdir)/libgnomeui			\
	$(WARN_CFLAGS)					\
	$(LIBGNOMEUI_DEMOS_CFLAGS)			\
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
	$(LIBGNOMEUI_DEMOS_LIBS)

EXTRA_PROGRAMS = \
	mdi_demo		\
	animator_demo

animator_demo_SOURCES = animator_demo.c

mdi_demo_SOURCES = mdi_demo.c

