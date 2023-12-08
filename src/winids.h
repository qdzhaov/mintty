#ifndef WINIDS_H
#define WINIDS_H

#define IDD_MAINBOX      100
#define IDI_MAINICON     200
#define IDI_CYGWIN       201
#define IDI_CMD          202
#define IDI_POWERSHELL   203
#define IDI_WSL          204
#define IDI_UBUNTU       205

/* From MSDN: In the WM_SYSCOMMAND message, the four low-order bits of
 * wParam are used by Windows, and should be masked off, so we shouldn't
 * attempt to store information in them. Hence all these identifiers have
 * the low 4 bits clear. Also, identifiers should be < 0xF000. */

#define IDM_OPEN            0x0010
#define IDM_NEW             0x0020
#define IDM_NEW_MONI        0x0030
#define IDM_OPTIONS         0x0040

#define IDM_SELALL          0x0100
#define IDM_SEARCH          0x0110
#define IDM_PASTE           0x0120
#define IDM_COPYTITLE       0x0130
#define IDM_COPY            0x0140
#define IDM_COPY_TEXT       0x0150
#define IDM_COPY_TABS       0x0160
#define IDM_COPY_TXT        0x0170
#define IDM_COPY_RTF        0x0180
#define IDM_COPY_HTXT       0x0190
#define IDM_COPY_HFMT       0x01A0
#define IDM_COPY_HTML       0x01B0
#define IDM_SAVEIMG         0x01C0
#define IDM_RESET_NOASK     0x01D0


#define IDM_UNUSED          0x0200
#define IDM_COPASTE         0x0210
#define IDM_CLRSCRLBCK      0x0220
#define IDM_RESET           0x0230
#define IDM_TEKRESET        0x0240
#define IDM_TEKPAGE         0x0250
#define IDM_TEKCOPY         0x0260

#define IDM_DEFSIZE         0x0300
#define IDM_DEFSIZE_ZOOM    0x0310
#define IDM_SCROLLBAR       0x0320
#define IDM_STATUSLINE      0x0330
#define IDM_FULLSCREEN      0x0340
#define IDM_FULLSCREEN_ZOOM 0x0350
#define IDM_BREAK           0x0360
#define IDM_FLIPSCREEN      0x0370
#define IDM_BORDERS         0x0380
#define IDM_TABBAR          0x0390
#define IDM_PARTLINE        0x03A0
#define IDM_INDICATOR       0x03B0

#define IDM_NEWTAB          0x0400
#define IDM_KILLTAB         0x0410
#define IDM_PREVTAB         0x0420
#define IDM_NEXTTAB         0x0430
#define IDM_MOVELEFT        0x0440
#define IDM_MOVERIGHT       0x0450

#define IDM_TOGLOG          0x0460
#define IDM_TOGCHARINFO     0x0470
#define IDM_TOGVT220KB      0x0480
#define IDM_HTML            0x0490
#define IDM_KEY_DOWN_UP     0x04A0

#define IDM_NEWTB           0x0500
#define IDM_NEWWSLT         0x0500
#define IDM_NEWCYGT         0x0510
#define IDM_NEWCMDT         0x0520
#define IDM_NEWPSHT         0x0530
#define IDM_NEWUSRT         0x0540
#define IDM_NEWTE           0x05FF

#define IDM_NEWWB           0x0600
#define IDM_NEWWSLW         0x0600
#define IDM_NEWCYGW         0x0610
#define IDM_NEWCMDW         0x0620
#define IDM_NEWPSHW         0x0630
#define IDM_NEWUSRW         0x0640
#define IDM_NEWWE           0x06FF
                                
#define IDM_USERCOMMAND     0x1000
#define IDM_SESSIONCMD      0x2000
#define IDM_SESSIONCOMMAND  0x4000
#define IDM_SYSMENUFUNCTION 0x7000
#define IDM_CTXMENUFUNCTION 0xA000
#define IDM_GOTAB           0xD000

#endif
