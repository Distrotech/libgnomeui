check_PROGRAMS = \
	test-libglade-gnome	\
	test-icon-theme	\
	test-libglade-gnome-noupgrade

check_DATA = \
	test-libglade-gnome.glade2	\
	test-libglade-gnome-noupgrade.glade2

TESTS = $(check_PROGRAMS)

TESTS_ENVIRONMENT = \
	LIBGLADE_MODULE_PATH="$(top_builddir)/glade/.libs:$$LIBGLADE_MODULE_PATH"	\
	GNOME_DISABLE_CRASH_DIALOG=1

INCLUDES = \
	-I$(top_srcdir)		\
	$(LIBGNOMEUI_GLADE_CFLAGS)

test-libglade-gnome.glade2: test-libglade-gnome.glade $(LIBGLADE_CONVERT)
	$(LIBGLADE_CONVERT) \
		$(srcdir)/test-libglade-gnome.glade \
		> test-libglade-gnome.glade2.tmp \
	&& mv test-libglade-gnome.glade2.tmp \
		test-libglade-gnome.glade2

test-libglade-gnome-noupgrade.glade2: test-libglade-gnome.glade $(LIBGLADE_CONVERT)
	$(LIBGLADE_CONVERT) --no-upgrade \
		$(srcdir)/test-libglade-gnome.glade \
		> test-libglade-gnome-noupgrade.glade2.tmp \
	&& mv test-libglade-gnome-noupgrade.glade2.tmp \
		test-libglade-gnome-noupgrade.glade2

LDADD = \
	$(top_builddir)/libgnomeui/libgnomeui-2.la \
	$(LIBGNOMEUI_GLADE_LIBS)

LDFLAGS = -export-dynamic

test_libglade_gnome_SOURCE = test-libglade-gnome.c
test_libglade_gnome_noupgrade_SOURCE = test-libglade-gnome-noupgrade.c
test_icon_theme_SOURCE = test-icon-theme.c

EXTRA_DIST = \
	test-libglade-gnome.glade

distclean-local:
	rm -f test-libglade-gnome.glade2 test-libglade-gnome-noupgrade.glade
