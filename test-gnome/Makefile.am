## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/libgnomeui			\
	$(WARN_CFLAGS)					\
	$(LIBGNOMEUI_CFLAGS)				\
	-DGNOMEUILIBDIR=\""$(libdir)"\"			\
	-DGNOMEUIDATADIR=\""$(datadir)"\"		\
	-DGNOMEUIUIDIR=\""$(datadir)/ui"\"		\
	-DGNOMEUIPIXMAPDIR=\""$(datadir)/pixmaps"\"	\
	-DGNOMEUIBINDIR=\""$(bindir)"\"			\
	-DGNOMEUILOCALSTATEDIR=\""$(localstatedir)"\"	\
	-DG_LOG_DOMAIN=\"GnomeUI\"			\
	-DVERSION=\"$(VERSION)\"			\
	$(GDK_PIXBUF_CFLAGS)

LDADD = \
	$(top_builddir)/libgnomeui/libgnomeui-2.la

noinst_HEADERS = \
	testgnome.h

noinst_PROGRAMS = \
	test-gnome

test_gnome_SOURCES =		\
	testgnome.c		\
	$(noinst_HEADERS)

EXTRA_DIST = 		\
	bomb.xpm

# the program has to be run from $srcdir to find its image files
test: test-gnome
	blddir=`pwd`; top_blddir=`cd $(top_builddir) && pwd`; cd $(srcdir); \
	$(SHELL) $$top_blddir/libtool --mode=execute $$blddir/test-gnome

test-debug: test-gnome
	blddir=`pwd`; top_blddir=`cd $(top_builddir) && pwd`; cd $(srcdir); \
	$(SHELL) $$top_blddir/libtool --mode=execute gdb $$blddir/test-gnome

