INCLUDES = \
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/libgnomeui			\
	$(WARN_CFLAGS)					\
	$(LIBGNOMEUI_DEMOS_CFLAGS)			\
	-DG_LOG_DOMAIN=\"GnomeUI\"

LDADD =	\
	$(top_builddir)/libgnomeui/libgnomeui-2.la \
	$(LIBGNOMEUI_DEMOS_LIBS)

EXTRA_PROGRAMS = \
	mdi_demo		\
	animator_demo

animator_demo_SOURCES = animator_demo.c

mdi_demo_SOURCES = mdi_demo.c

