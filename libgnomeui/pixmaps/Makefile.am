pixmapdir = $(datadir)/pixmaps
CONVERTRGB = ../../tools/convertrgb/convertrgb
BITMAPS    = calculator-font.png

# 
# CONVERTRGB is required to create generated sopurces.
# If we are just doing "make dist", then this must be built by hand
#
$(CONVERTRGB):
	cd ../../tools/convertrgb && \
	$(MAKE) $(AM_MAKEFLAGS) convertrgb

pixmap_DATA = $(BITMAPS)






# Please update this when you add more `stock_*.png' files
stock_images = \
stock_add.png \
stock_align_center.png \
stock_align_justify.png \
stock_align_left.png \
stock_align_right.png \
stock_attach.png \
stock_book_blue.png \
stock_book_green.png \
stock_book_open.png \
stock_book_red.png \
stock_book_yellow.png \
stock_bottom.png \
stock_button_apply.png \
stock_button_cancel.png \
stock_button_close.png \
stock_button_no.png \
stock_button_ok.png \
stock_button_yes.png \
stock_cdrom.png \
stock_clear.png \
stock_close.png \
stock_colorselector.png \
stock_convert.png \
stock_copy.png \
stock_cut.png \
stock_down_arrow.png \
stock_exec.png \
stock_exit.png \
stock_font.png \
stock_first.png \
stock_help.png \
stock_home.png \
stock_index.png \
stock_jump_to.png \
stock_last.png \
stock_left_arrow.png \
stock_line_in.png \
stock_mail.png \
stock_mail_compose.png \
stock_mail_forward.png \
stock_mail_receive.png \
stock_mail_reply.png \
stock_mail_send.png \
stock_menu_about.png \
stock_menu_blank.png \
stock_menu_scores.png \
stock_mic.png \
stock_multiple_file.png \
stock_new.png \
stock_not.png \
stock_open.png \
stock_paste.png \
stock_preferences.png \
stock_print.png \
stock_properties.png \
stock_redo.png \
stock_refresh.png \
stock_remove.png \
stock_revert.png \
stock_right_arrow.png \
stock_save.png \
stock_save_as.png \
stock_scores.png \
stock_search.png \
stock_search_replace.png \
stock_spellcheck.png \
stock_stop.png \
stock_table_borders.png \
stock_table_fill.png \
stock_text_bold.png \
stock_text_bulleted_list.png \
stock_text_indent.png \
stock_text_italic.png \
stock_text_numbered_list.png \
stock_text_strikeout.png \
stock_text_underline.png \
stock_text_unindent.png \
stock_timer.png \
stock_timer_stopped.png \
stock_top.png \
stock_trash.png \
stock_trash_full.png \
stock_undelete.png \
stock_undo.png \
stock_up_arrow.png \
stock_volume.png \
stock_midi.png

EXTRA_DIST  = $(BITMAPS) $(stock_images)

noinst_HEADERS = gnome-stock-imlib.h
BUILT_SOURCES = gnome-stock-imlib.h

CLEANFILES = $(BUILT_SOURCES)

gnome-stock-imlib.h: $(stock_images) $(CONVERTRGB) Makefile.am
	@echo "creating gnome-stock-imlib.h"
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
	for i in $(stock_images); do \
	  vn=`echo $$i | sed -e 's,stock_\(.*\)\.png,imlib_\1,'`; \
	  $(CONVERTRGB) -n -v=$$vn -a=gnome-stock-imlib.h $(srcdir)/$$i;\
	done


