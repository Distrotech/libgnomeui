# $(datadir)/gtkrc.hy
#
# This file defines the fontsets for Armenina language (hy) encoding (armscii-8)
# You can find links for armscii-8 True Type fonts following the links at my
# page on armenian localisation at http://www.ping.be/linux/armenian/
#
# 1999, Pablo Saratxaga <srtxg@chanae.alphanet.ch>
#

style "GnomeScores_CurrentPlayer_style"
{
  fg[NORMAL] = {1.0, 0.0, 0.0}
}

style "GnomeScores_Logo_style"
{
  fontset = "-*-armnet helvetica-medium-r-normal--30-*-*-*-*-*-armscii-8"
  fg[NORMAL] = {0.0, 0.0, 1.0}
}

style "GnomeAbout_DrawingArea_style"
{
  bg[NORMAL] = {1.0, 1.0, 1.0}
}

style "GnomeAbout_Title_style"
{
  fontset = "-*-armnet helvetica-medium-r-normal--20-*-*-*-*-*-armscii-8"
}

style "GnomeAbout_Copyright_style"
{
  fontset = "-*-armnet helvetica-medium-r-normal--14-*-*-*-*-*-armscii-8"
}

style "GnomeAbout_Author_style"
{
  fontset = "-*-armnet helvetica-medium-r-normal--12-*-*-*-*-*-armscii-8"
}

style "GnomeAbout_Names_style"
{
  fontset = "-*-armnet helvetica-medium-r-normal--12-*-*-*-*-*-armscii-8"
}

style "GnomeAbout_Comments_style"
{
  fontset = "-*-armnet helvetica-medium-r-normal--10-*-*-*-*-*-armscii-8"
}

style "GnomeHRef_Label_style"
{
  fg[NORMAL] = { 0.0, 0.0, 1.0 }
  fg[PRELIGHT] = { 0.0, 0.0, 1.0 }
  fg[INSENSITIVE] = { 0.5, 0.5, 1.0 }
  fg[ACTIVE] = { 1.0, 0.0, 0.0 }
}

style "GnomeGuru_PageTitle_style" 
{
 fontset = "-*-armnet helvetica-medium-r-normal--10-*-*-*-*-*-armscii-8"
}

widget "*GnomeScores*.CurrentPlayer" style "GnomeScores_CurrentPlayer_style"
widget "*GnomeScores*.Logo" style "GnomeScores_Logo_style"
widget "*GnomeAbout*.DrawingArea" style "GnomeAbout_DrawingArea_style"
widget "*GnomeAbout*.Author" style "GnomeAbout_Author_style"
widget "*GnomeAbout*.Comments" style "GnomeAbout_Comments_style"
widget "*GnomeAbout*.Copyright" style "GnomeAbout_Copyright_style"
widget "*GnomeAbout*.Names" style "GnomeAbout_Names_style"
widget "*GnomeAbout*.Title" style "GnomeAbout_Title_style"
widget "*GnomeHRef.GtkLabel" style "GnomeHRef_Label_style"
widget "*GnomeGuru*.PageTitle" style "GnomeGuru_PageTitle_style"
