## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/libgnomeui			\
	$(WARN_CFLAGS)					\
	$(LIBGNOMEUI_CFLAGS)				\
	-DGNOMEUIPIXMAPDIR=\""$(datadir)/pixmaps"\"	\
	-DGNOMEUISRCDIR=\""$(srcdir)"\"			\
	-DG_LOG_DOMAIN=\"GnomeUI\"

LDADD = \
	$(top_builddir)/libgnomeui/libgnomeui-2.la $(GNOME_TEST_LIBS)

noinst_PROGRAMS = \
	test-gnome test-druid test-entry test-iconlist test-password-dialog test-gnomegdkpixbuf

test_gnome_SOURCES =		\
	testgnome.c		\
	testgnome.h

test_druid_SOURCES =	\
	testdruid.c

test_password_dialog_SOURCES =	\
	prop-editor.c \
	prop-editor.h \
	test-password-dialog.c

test_entry_SOURCES = 	\
	testentry.c

test_iconlist_SOURCES = 	\
	testiconlist.c

test_gnomegdkpixbuf_SOURCES = 	\
	testgnomegdkpixbuf.c

EXTRA_DIST = 		\
	bomb.xpm	\
	testgnome.xml

# the program has to be run from $srcdir to find its image files
test: test-gnome
	blddir=`pwd`; top_blddir=`cd $(top_builddir) && pwd`; cd $(srcdir); \
	$(SHELL) $$top_blddir/libtool --mode=execute $$blddir/test-gnome

test-debug: test-gnome
	blddir=`pwd`; top_blddir=`cd $(top_builddir) && pwd`; cd $(srcdir); \
	$(SHELL) $$top_blddir/libtool --mode=execute gdb $$blddir/test-gnome

