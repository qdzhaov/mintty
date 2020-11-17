clone from github.com/mintty/mintty 3.4.3
welcome Testing;
Please report bugs or suggest enhancements via the issue tracker.
remove orgin wintab,
Add new Tab followed fatty,
Add control mode,So can release many hotkeys,it's helpfull for tmux+vim
Optimize HotKey,not complete,fast hotkey,easyly [un]define by your self
Surport Global HotKey
Surport Win+hotkey use SetWindowsHookExW(WH_KEYBOARD_LL,...)
surport partline last line can be partline,usrful for tmux 

win+Left        prevtab
win+right       nexttab
win+Shift+Left  tabmovetoprev
win+Shift+Right tabmovetoNext

surport control mode:
  enter:double ctrl A;
        Ctrl>Ctrl>a 
        fast press 3 times
S+LEFT or S+H:tabmovetoprev
Left or H :prev tab
S+LEFT or S+H:tabmovetonext
Left or H :next tab
'1' ... '9': tab_go(x);
' ': selectmode
'A': select_all
'C': copy
'D': DEFSIZE
'F': FULLSCREEN
'G': win_tab_show
'I': win_tab_indicator
'K': win_tog_partline
'O': win_tog_scrollbar
'M': popup_menu
'N': newwin 
'P': change pointer 
'R': RESET
'S': SEARCH
'T': new_tab
'V': paste
'W': close
OEM_MINUS or SUBTRACT:  zoomout
OEM_PLUS  or ADD:       zoomin
'0'       or NUMPAD0:   zoomrest 



Mintty is the [Cygwin](http://cygwin.com) Terminal emulator, 
also available for [MSYS](http://mingw.org/wiki/MSYS) 
and [Msys2](https://github.com/msys2).

### Overview ###

For an introduction, features overview, and screenshots, see the 
[<img align=absmiddle src=icon/terminal.ico>Mintty homepage](http://mintty.github.io/),
with an opportunity to donate to appreciate mintty.

For detailed hints and specific issues, see the [Wiki](https://github.com/mintty/mintty/wiki).

For comprehensive general documentation, see the [manual page](http://mintty.github.io/mintty.1.html).

### Bugs and Enhancements ###

Please report bugs or suggest enhancements via the [issue tracker](https://github.com/mintty/mintty/issues).

Bugs that were reported to the previous repository at Google code before June, 2015, have been migrated here.

  * Mind! Before reporting an issue about character interaction with an application, please check the issue also with at least one other terminal (xterm, urxvt), and maybe the Cygwin Console. 
    It may also be useful to get a proper understanding of the rôle of a terminal as explained e.g. in [difference between a 'terminal', a 'shell', a 'tty' and a 'console'](http://unix.stackexchange.com/questions/4126/what-is-the-exact-difference-between-a-terminal-a-shell-a-tty-and-a-con).

### Contribution ###

If you consider to suggest a patch or contribute to mintty otherwise, discuss your proposal in an issue first, or on the Cygwin mailing list, or with the maintainer.

  * Repository policy: No unsolicited pull requests!
