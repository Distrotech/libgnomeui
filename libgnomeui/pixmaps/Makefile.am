pixmapdir = $(datadir)/pixmaps

BITMAPS = calculator-font.xpm

pixmap_DATA = $(BITMAPS)

EXTRA_DIST  = $(BITMAPS)

noinst_HEADERS = gnome-stock-imlib.h

CLEANFILES = gnome-stock-imlib.h tmp.h

gnome-stock-imlib.h: Makefile $(srcdir)/stock_*.png
	@echo "/* gnome stock icons in Imlib usable rgb data" >tmp.h
	@echo " *" >>tmp.h
	@echo " * Copyright (C) 1997 the Free Software Foundation" >>tmp.h
	@echo " *" >>tmp.h
	@echo " * the pixmaps for button_cancel, save_as, trash, mail_receive, stop" >>tmp.h
	@echo " * right_arrow, home, save, book_blue, trash_full, undo, search, mail_forward" >>tmp.h
	@echo " * revert, mail_compose, mic, copy, jump_to, convert, button_apply, timer" >>tmp.h
	@echo " * mail, book_red, book_yellow, help, redo, new, mail_send, button_yes, cut" >>tmp.h
	@echo " * properties, print, mail_reply, refresh, search_replace, open, preferences" >>tmp.h
	@echo " * left_arrow, undelete, close, volume, button_no, exit, book_green" >>tmp.h
	@echo " * book_open, paste, line_in, spellcheck, timer_stopped" >>tmp.h
	@echo " * are Copyright (C) 1998 Tuomas Kuosmanen" >>tmp.h
	@echo " */" >>tmp.h
	@echo >>tmp.h
	../../tools/convertrgb/convertrgb -n -a=tmp.h $(srcdir)/stock_*.png
	sed -e 's/stock_\(.*\)_rgb/imlib_\1/' <tmp.h >gnome-stock-imlib.h ; \
		rm -f tmp.h

