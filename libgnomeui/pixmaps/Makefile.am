pixmapdir = $(datadir)/pixmaps

BITMAPS = calculator-font.xpm

pixmap_DATA = $(BITMAPS)

EXTRA_DIST  = $(BITMAPS)

noinst_PROGRAMS = xpm2rgb

noinst_HEADERS = gnome-stock-imlib.h

xpm2rgb_SOURCES = xpm2rgb.c

xpm2rgb_LDADD = \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS) \
	$(INTLLIBS)

gnome-stock-imlib.h: Makefile.am stock_*.xpm xpm2rgb
	@echo "building gnome-stock-imlib.h"
	@echo "/* gnome stock icons in Imlib usable rgb data" >gnome-stock-imlib.h
	@echo " *" >>gnome-stock-imlib.h
	@echo " * Copyright (C) 1997 the Free Software Foundation" >>gnome-stock-imlib.h
	@echo " *" >>gnome-stock-imlib.h
	@echo " * the pixmaps for button_cancel, save_as, trash, mail_receive, stop" >>gnome-stock-imlib.h
	@echo " * right_arrow, home, save, book_blue, trash_full, undo, search, mail_forward" >>gnome-stock-imlib.h
	@echo " * revert, mail_compose, mic, copy, jump_to, convert, button_apply, timer" >>gnome-stock-imlib.h
	@echo " * mail, book_red, book_yellow, help, redo, new, mail_send, button_yes, cut" >>gnome-stock-imlib.h
	@echo " * properties, print, mail_reply, refresh, search_replace, open, preferences" >>gnome-stock-imlib.h
	@echo " * left_arrow, undelete, close, volume, button_no, exit, book_green" >>gnome-stock-imlib.h
	@echo " * book_open, paste, line_in, spellcheck, timer_stopped" >>gnome-stock-imlib.h
	@echo " * are Copyright (C) 1998 Tuomas Kuosmanen" >>gnome-stock-imlib.h
	@echo " */" >>gnome-stock-imlib.h
	@echo >>gnome-stock-imlib.h
	@for i in stock_*.xpm; do I="`echo $$i | sed -e 's/stock_//' -e 's/.xpm//'`" ; ./xpm2rgb $$i imlib_$$I ; done
	

