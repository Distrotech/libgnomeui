# $(datadir)/gtkrc

style "GnomeScores_CurrentPlayer_style"
{
  fg[NORMAL] = {1.0, 0.0, 0.0}
}

style "GnomeScores_Logo_style"
{
  fontset = "-freefont-garamond-*-*-*-*-30-170-*-*-*-*-iso8859-1,\
          -cronyx-times-*-r-*-*-34-*-*-*-*-*-koi8-r"
  fg[NORMAL] = {0.0, 0.0, 1.0}
}

style "GnomeAbout_DrawingArea_style"
{
  bg[NORMAL] = {1.0, 1.0, 1.0}
}

style "GnomeAbout_Title_style"
{
  fontset = "-*-helvetica-bold-r-normal-*-20-*-*-*-*-*-*-*"
}

style "GnomeAbout_Copyright_style"
{
  fontset = "-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*"
}

style "GnomeAbout_Author_style"
{
  fontset = "-*-helvetica-bold-r-normal-*-12-*-*-*-*-*-*-*"
}

style "GnomeAbout_Names_style"
{
  fontset = "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*"
}

style "GnomeAbout_Comments_style"
{
  fontset = "-adobe-helvetica-medium-r-normal-*-10-*-*-*-*-*-*-*,\
             -cronyx-helvetica-medium-r-normal-*-11-*-*-*-*-*-*-*"
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
 fontset = "-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*"	
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
