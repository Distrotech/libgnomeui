pixmapdir = $(datadir)/pixmaps
BITMAPS    = calculator-font.png

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

noinst_HEADERS = gnome-stock-pixbufs.h
BUILT_SOURCES = gnome-stock-pixbufs.h

CLEANFILES = $(BUILT_SOURCES)

gnome-stock-pixbufs.h: @GDK_PIXBUF_CSOURCE@ $(stock_images)
	-rm -f gnome-stock-pixbufs.h
	var=`echo "$(stock_images)" | sed -e 's,stock_\([^\.]*\)\.png,stock_\1 $(srcdir)/stock_\1.png,g'` ; \
	   @GDK_PIXBUF_CSOURCE@ --raw --build-list $$var > pixbufs-tmp && \
	   cat $(srcdir)/copyright.txt pixbufs-tmp > gnome-stock-pixbufs.h
	-rm -f pixbufs-tmp
