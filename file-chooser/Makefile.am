
NULL =

INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	$(GNOME_VFS_FILE_SYSTEM_CFLAGS)		\
	$(GIO_FILE_SYSTEM_CFLAGS)		\
	$(WARN_CFLAGS)				\
	-DGNOMEUILOCALEDIR="\"$(datadir)/locale\""	\
	-D_FILE_OFFSET_BITS=64			\
	-D_BSD_SOURCE				\
	-D_GNU_SOURCE				\
	-D_LARGEFILE64_SOURCE			\
	-D_POSIX_PTHREAD_SEMANTICS		\
	-D_REENTRANT				\
	-DG_DISABLE_DEPRECATED			\
	$(NULL)

module_flags = -export_dynamic -avoid-version -module -no-undefined
modulesdir = $(libdir)/gtk-2.0/$(GTK_BINARY_VERSION)/filesystems

modules_LTLIBRARIES =				\
	libgnome-vfs.la			\
	libgio.la			\
	$(NULL)

libgnome_vfs_la_SOURCES = gtkfilesystemgnomevfs.c gtkfilesystemgnomevfs.h
libgnome_vfs_la_LDFLAGS = $(module_flags)
libgnome_vfs_la_LIBADD = $(GNOME_VFS_FILE_SYSTEM_LIBS)	\
	$(top_builddir)/libgnomeui/libgnomeui-2.la

libgio_la_SOURCES = gtkfilesystemgio.c gtkfilesystemgio.h
libgio_la_LDFLAGS = $(module_flags)
libgio_la_LIBADD = $(GIO_FILE_SYSTEM_LIBS)

EXTRA_DIST = \
	testfilechooser.c

