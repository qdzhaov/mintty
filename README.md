# enhanced mintty #
branch from github.com/mintty/mintty 3.6.2  
Please report bugs or suggest enhancements via the 
[gitee  issue tracker](https://gitee.com/qdzhaov/mintty/issues).

## main enhancement: ##
+ surport multiple Tab followed fatty,  
+ Add control mode,So can release many hotkeys,it's helpfull for tmux+vim  
+ surport partline last line can be partline,usrful for tmux   
+ Control config dialog fontsize  
+ Surport Win+key as shortcuts key,all confict key with windows,will overide it.  
+ Surport WSL/CYGWIN/COMMAND/POWERSHELL session
+ optimize shortcuts key  
  easy use ,easy read,high effcient  
  user define shortcuts easy an high effcient  
+ surport colour & theme edit
+ TODO:
  Todo: user define shortcuts in config dialog  
  Todo: add user define shortcuts for every function  
  Todo: have many global vars,decrease it  
  Todo: add fast start surport  
  Todo: verify all global variable (include static,member of config ) is term independ.  
+ Important:   
  the golbal variable is term depend,should be in parameter,term is used many many times,  
  I can't modify it,I use global pointer cterm,be sure it pointer to correct term.  

## tab control shortcuts ## 
```
win+Left        prevtab  
win+right       nexttab  
win+Shift+Left  tabmovetoprev  
win+Shift+Right tabmovetoNext  
win+Q           Quit  
win+w           CloseTab  
win+T           newtab  
win+X           Enter Control mode
win+Z           minisize  
```
## enter control mode:  
### double ctrl A;  
### Ctrl>Ctrl>a 
### .     fast press 3 times  
### or Win+X 
```
S+LEFT  or S+H : tabmovetoprev  
  LEFT  or   H : prev tab  
S+RIGHT or S+L : tabmovetonext  
  RIGHT or   L : next tab  
'1' ... '9'    : tab_go(x)  
     SPACE     : selectmode  
'-' : zoomout  
'+' : zoomin  
'0' : zoomrest   
'A' : select_all  
'C' : copy  
'D' : DEFSIZE  
'F' : FULLSCREEN  
'G' : win_tab_show  
'I' : win_tab_indicator  
'K' : win_tog_partline  
'O' : win_tog_scrollbar  
'M' : popup_menu  
'N' : newwin   
'P' : change pointer  
'R' : RESET  
'S' : SEARCH  
'T' : new_tab  
'V' : paste  
'W' : close  
```
-----------------
## cfg.key_commands
### cfg.key_commands is a string for user define shortcuts;
### usage :
### mode is: 
####  S= Shift;  A= Alt;  C= Ctrl;  W= Win  
#### LS=LShift; LA=LAlt; LC=LCtrl; LW=LWin  
#### RS=RShift; RA=RAlt; RC=RCtrl; RW=RWin  
```
  A+C:win_close;A+RETURN:fullscreen;A+F4:;
  set : 
  Alt+C to close tab
  A+RETURN to fullscreen
  A+F4 to nothing; this will override orgin def

```

Mintty is the [Cygwin](http://cygwin.com) Terminal, 
as well as the 
[terminal emulator](http://en.wikipedia.org/wiki/Terminal_emulator) 
for MSYS 
([Msys2](https://github.com/msys2), 
[old MSYS](https://web.archive.org/web/20201223203407/http://www.mingw.org/)).

### Overview ###

For an introduction, features overview, and screenshots, see the 
[<img align=absmiddle src=icon/terminal.ico>Mintty homepage](http://mintty.github.io/).

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
