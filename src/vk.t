#define DKND(t,n) addknv(#n,VK_##n);vkname[VK_##n]=#n;vktype[VK_##n]=t;
#define DKNE(n  ) addknv(#n,VK_##n);
  DKND(0,LBUTTON            )// 0x01  VK_LBUTTON	
  DKND(0,RBUTTON            )// 0x02  VK_RBUTTON	
  DKND(2,BREAK              )// 0x03  VK_CANCEL	
  DKND(0,MBUTTON            )// 0x04  VK_MBUTTON	
  DKND(0,XBUTTON1           )// 0x05  VK_XBUTTON1	
  DKND(0,XBUTTON2           )// 0x06  VK_XBUTTON2	
                             // 0x07  reserved
  DKND(2,BACK               )// 0x08  VK_BACK	
  DKND(2,TAB                )// 0x09  VK_TAB	
                             // 0x0A-0x0B reserved
  DKND(2,BEGIN              )// 0x0C  VK_CLEAR	
  DKND(2,ENTER              )// 0x0D  VK_RETURN	
                             // 0x0E-0x0F undefined
  DKND(1,SHIFT              )// 0x10  VK_SHIFT	
  DKND(1,CONTROL            )// 0x11  VK_CONTROL	
  DKND(1,ALT                )// 0x12  VK_MENU	
  DKND(6,PAUSE              )// 0x13  VK_PAUSE	
  DKND(0,CAPSLOCK           )// 0x14  VK_CAPITAL	
  DKND(0,KANA               )// 0x15  VK_KANA	
  DKND(0,IME_ON             )// 0x16  VK_IME_ON	
  DKND(0,JUNJA              )// 0x17  VK_JUNJA	
  DKND(0,FINAL              )// 0x18  VK_FINAL	
  DKND(0,HANJA              )// 0x19  VK_HANJA	
  DKND(0,IME_OFF            )// 0x1A  VK_IME_OFF	
  DKND(2,ESC                )// 0x1B  VK_ESCAPE	
  DKND(2,CONVERT            )// 0x1C  VK_CONVERT	
  DKND(2,NONCONVERT         )// 0x1D  VK_NONCONVERT	
  DKND(2,ACCEPT             )// 0x1E  VK_ACCEPT	
  DKND(2,MODECHANGE         )// 0x1F  VK_MODECHANGE	
  DKND(2,SPACE              )// 0x20  VK_SPACE	
  DKND(6,PRIOR              )// 0x21  VK_PRIOR	
  DKND(6,NEXT               )// 0x22  VK_NEXT	
  DKND(6,END                )// 0x23  VK_END	
  DKND(6,HOME               )// 0x24  VK_HOME	
  DKND(6,LEFT               )// 0x25  VK_LEFT	
  DKND(6,UP                 )// 0x26  VK_UP	
  DKND(6,RIGHT              )// 0x27  VK_RIGHT	
  DKND(6,DOWN               )// 0x28  VK_DOWN	
  DKND(6,SELECT             )// 0x29  VK_SELECT	
  DKND(6,PRINT              )// 0x2A  VK_PRINT	
  DKND(6,EXEC               )// 0x2B  VK_EXECUTE	
  DKND(6,PRINTSCREEN)        // 0x2C  VK_SNAPSHOT	
  DKND(6,INSERT             )// 0x2D  VK_INSERT	
  DKND(6,DELETE             )// 0x2E  VK_DELETE	
  DKND(6,HELP               )// 0x2F  VK_HELP	
                             //0x30-0x39 num
                             //0x3A-0x40 undefined
                             //0x41-0x5A A-Z
  DKND(1,LWIN               )// 0x5B  VK_LWIN	
  DKND(1,RWIN               )// 0x5C  VK_RWIN	
  DKND(1,APPS               )// 0x5D  VK_APPS	
                             // 0x5E  reserved
  DKND(2,SLEEP              )// 0x5F  VK_SLEEP	
  DKND(2,NUMPAD0            )// 0x60  VK_NUMPAD0	
  DKND(2,NUMPAD1            )// 0x61  VK_NUMPAD1	
  DKND(2,NUMPAD2            )// 0x62  VK_NUMPAD2	
  DKND(2,NUMPAD3            )// 0x63  VK_NUMPAD3	
  DKND(2,NUMPAD4            )// 0x64  VK_NUMPAD4	
  DKND(2,NUMPAD5            )// 0x65  VK_NUMPAD5	
  DKND(2,NUMPAD6            )// 0x66  VK_NUMPAD6	
  DKND(2,NUMPAD7            )// 0x67  VK_NUMPAD7	
  DKND(2,NUMPAD8            )// 0x68  VK_NUMPAD8	
  DKND(2,NUMPAD9            )// 0x69  VK_NUMPAD9	
  DKND(2,MULTIPLY           )// 0x6A  VK_MULTIPLY	
  DKND(2,ADD                )// 0x6B  VK_ADD	
  DKND(2,SEPARATOR          )// 0x6C  VK_SEPARATOR	
  DKND(2,SUBTRACT           )// 0x6D  VK_SUBTRACT	
  DKND(2,DECIMAL            )// 0x6E  VK_DECIMAL	
  DKND(2,DIVIDE             )// 0x6F  VK_DIVIDE	
  DKND(6,F1                 )// 0x70  VK_F1	
  DKND(6,F2                 )// 0x71  VK_F2	
  DKND(6,F3                 )// 0x72  VK_F3	
  DKND(6,F4                 )// 0x73  VK_F4	
  DKND(6,F5                 )// 0x74  VK_F5	
  DKND(6,F6                 )// 0x75  VK_F6	
  DKND(6,F7                 )// 0x76  VK_F7	
  DKND(6,F8                 )// 0x77  VK_F8	
  DKND(6,F9                 )// 0x78  VK_F9	
  DKND(6,F10                )// 0x79  VK_F10	
  DKND(6,F11                )// 0x7A  VK_F11	
  DKND(6,F12                )// 0x7B  VK_F12	
  DKND(6,F13                )// 0x7C  VK_F13	
  DKND(6,F14                )// 0x7D  VK_F14	
  DKND(6,F15                )// 0x7E  VK_F15	
  DKND(6,F16                )// 0x7F  VK_F16	
  DKND(6,F17                )// 0x80  VK_F17	
  DKND(6,F18                )// 0x81  VK_F18	
  DKND(6,F19                )// 0x82  VK_F19	
  DKND(6,F20                )// 0x83  VK_F20	
  DKND(6,F21                )// 0x84  VK_F21	
  DKND(6,F22                )// 0x85  VK_F22	
  DKND(6,F23                )// 0x86  VK_F23	
  DKND(6,F24                )// 0x87  VK_F24	
#if _WIN32_WINNT >= 0x0604
  DKND(6,NAVIGATION_VIEW    )// 0x88  VK_NAVIGATION_VIEW	
  DKND(6,NAVIGATION_MENU    )// 0x89  VK_NAVIGATION_MENU	
  DKND(6,NAVIGATION_UP      )// 0x8A  VK_NAVIGATION_UP	
  DKND(6,NAVIGATION_DOWN    )// 0x8B  VK_NAVIGATION_DOWN	
  DKND(6,NAVIGATION_LEFT    )// 0x8C  VK_NAVIGATION_LEFT	
  DKND(6,NAVIGATION_RIGHT   )// 0x8D  VK_NAVIGATION_RIGHT	
  DKND(6,NAVIGATION_ACCEPT  )// 0x8E  VK_NAVIGATION_ACCEPT	
  DKND(6,NAVIGATION_CANCEL  )// 0x8F  VK_NAVIGATION_CANCEL	
#endif
  DKND(6,NUMLOCK            )// 0x90  VK_NUMLOCK	
  DKND(6,SCROLLLOCK         )// 0x91  VK_SCROLL	
  DKND(6,OEM_NEC_EQUAL      )// 0x92  VK_OEM_NEC_EQUAL	
  DKND(6,OEM_FJ_MASSHOU     )// 0x93  VK_OEM_FJ_MASSHOU	
  DKND(6,OEM_FJ_TOUROKU     )// 0x94  VK_OEM_FJ_TOUROKU	
  DKND(6,OEM_FJ_LOYA        )// 0x95  VK_OEM_FJ_LOYA	
  DKND(6,OEM_FJ_ROYA        )// 0x96  VK_OEM_FJ_ROYA	
                             // 0x97-0x9F undefined
  DKND(1,LSHIFT             )// 0xA0  VK_LSHIFT	
  DKND(1,RSHIFT             )// 0xA1  VK_RSHIFT	
  DKND(1,LCONTROL           )// 0xA2  VK_LCONTROL	
  DKND(1,RCONTROL           )// 0xA3  VK_RCONTROL	
  DKND(1,LALT               )// 0xA4  VK_LMENU	
  DKND(1,RALT               )// 0xA5  VK_RMENU	
  DKND(6,BROWSER_BACK       )// 0xA6  VK_BROWSER_BACK	
  DKND(6,BROWSER_FORWARD    )// 0xA7  VK_BROWSER_FORWARD	
  DKND(6,BROWSER_REFRESH    )// 0xA8  VK_BROWSER_REFRESH	
  DKND(6,BROWSER_STOP       )// 0xA9  VK_BROWSER_STOP	
  DKND(6,BROWSER_SEARCH     )// 0xAA  VK_BROWSER_SEARCH	
  DKND(6,BROWSER_FAVORITES  )// 0xAB  VK_BROWSER_FAVORITES	
  DKND(6,BROWSER_HOME       )// 0xAC  VK_BROWSER_HOME	
  DKND(6,VOLUME_MUTE        )// 0xAD  VK_VOLUME_MUTE	
  DKND(6,VOLUME_DOWN        )// 0xAE  VK_VOLUME_DOWN	
  DKND(6,VOLUME_UP          )// 0xAF  VK_VOLUME_UP	
  DKND(6,MEDIA_NEXT_TRACK   )// 0xB0  VK_MEDIA_NEXT_TRACK	
  DKND(6,MEDIA_PREV_TRACK   )// 0xB1  VK_MEDIA_PREV_TRACK	
  DKND(6,MEDIA_STOP         )// 0xB2  VK_MEDIA_STOP	
  DKND(6,MEDIA_PLAY_PAUSE   )// 0xB3  VK_MEDIA_PLAY_PAUSE	
  DKND(6,LAUNCH_MAIL        )// 0xB4  VK_LAUNCH_MAIL	
  DKND(6,LAUNCH_MEDIA_SELECT)// 0xB5  VK_LAUNCH_MEDIA_SELECT	
  DKND(6,LAUNCH_APP1        )// 0xB6  VK_LAUNCH_APP1	
  DKND(6,LAUNCH_APP2        )// 0xB7  VK_LAUNCH_APP2	
                             // 0xB8-0xB9 reserved
  DKND(2,OEM_1              )// 0xBA  VK_OEM_1	
  DKND(2,OEM_PLUS           )// 0xBB  VK_OEM_PLUS	
  DKND(2,OEM_COMMA          )// 0xBC  VK_OEM_COMMA	
  DKND(2,OEM_MINUS          )// 0xBD  VK_OEM_MINUS	
  DKND(2,OEM_PERIOD         )// 0xBE  VK_OEM_PERIOD	
  DKND(2,OEM_2              )// 0xBF  VK_OEM_2	
  DKND(2,OEM_3              )// 0xC0  VK_OEM_3	
#if _WIN32_WINNT >= 0x0604
  DKND(6,GAMEPAD_A                         )// 0xC3  VK_GAMEPAD_A	
  DKND(6,GAMEPAD_B                         )// 0xC4  VK_GAMEPAD_B	
  DKND(6,GAMEPAD_X                         )// 0xC5  VK_GAMEPAD_X	
  DKND(6,GAMEPAD_Y                         )// 0xC6  VK_GAMEPAD_Y	
  DKND(6,GAMEPAD_RIGHT_SHOULDER            )// 0xC7  VK_GAMEPAD_RIGHT_SHOULDER	
  DKND(6,GAMEPAD_LEFT_SHOULDER             )// 0xC8  VK_GAMEPAD_LEFT_SHOULDER	
  DKND(6,GAMEPAD_LEFT_TRIGGER              )// 0xC9  VK_GAMEPAD_LEFT_TRIGGER	
  DKND(6,GAMEPAD_RIGHT_TRIGGER             )// 0xCA  VK_GAMEPAD_RIGHT_TRIGGER	
  DKND(6,GAMEPAD_DPAD_UP                   )// 0xCB  VK_GAMEPAD_DPAD_UP	
  DKND(6,GAMEPAD_DPAD_DOWN                 )// 0xCC  VK_GAMEPAD_DPAD_DOWN	
  DKND(6,GAMEPAD_DPAD_LEFT                 )// 0xCD  VK_GAMEPAD_DPAD_LEFT	
  DKND(6,GAMEPAD_DPAD_RIGHT                )// 0xCE  VK_GAMEPAD_DPAD_RIGHT	
  DKND(6,GAMEPAD_MENU                      )// 0xCF  VK_GAMEPAD_MENU	
  DKND(6,GAMEPAD_VIEW                      )// 0xD0  VK_GAMEPAD_VIEW	
  DKND(6,GAMEPAD_LEFT_THUMBSTICK_BUTTON    )// 0xD1  VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON	
  DKND(6,GAMEPAD_RIGHT_THUMBSTICK_BUTTON   )// 0xD2  VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON	
  DKND(6,GAMEPAD_LEFT_THUMBSTICK_UP        )// 0xD3  VK_GAMEPAD_LEFT_THUMBSTICK_UP	
  DKND(6,GAMEPAD_LEFT_THUMBSTICK_DOWN      )// 0xD4  VK_GAMEPAD_LEFT_THUMBSTICK_DOWN	
  DKND(6,GAMEPAD_LEFT_THUMBSTICK_RIGHT     )// 0xD5  VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT	
  DKND(6,GAMEPAD_LEFT_THUMBSTICK_LEFT      )// 0xD6  VK_GAMEPAD_LEFT_THUMBSTICK_LEFT	
  DKND(6,GAMEPAD_RIGHT_THUMBSTICK_UP       )// 0xD7  VK_GAMEPAD_RIGHT_THUMBSTICK_UP	
  DKND(6,GAMEPAD_RIGHT_THUMBSTICK_DOWN     )// 0xD8  VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN	
  DKND(6,GAMEPAD_RIGHT_THUMBSTICK_RIGHT    )// 0xD9  VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT	
  DKND(6,GAMEPAD_RIGHT_THUMBSTICK_LEFT     )// 0xDA  VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT	
#endif 
  DKND(2,OEM_4              )// 0xDB  VK_OEM_4	
  DKND(2,OEM_5              )// 0xDC  VK_OEM_5	
  DKND(2,OEM_6              )// 0xDD  VK_OEM_6	
  DKND(2,OEM_7              )// 0xDE  VK_OEM_7	
  DKND(2,OEM_8              )// 0xDF  VK_OEM_8	
                             // 0xE0  reserved
  DKND(2,OEM_AX             )// 0xE1  VK_OEM_AX	
  DKND(2,OEM_102            )// 0xE2  VK_OEM_102	
  DKND(6,ICO_HELP           )// 0xE3  VK_ICO_HELP	
  DKND(6,ICO_00             )// 0xE4  VK_ICO_00	
  DKND(0,PROCESSKEY         )// 0xE5  VK_PROCESSKEY	
  DKND(6,ICO_CLEAR          )// 0xE6  VK_ICO_CLEAR	
  DKND(6,PACKET             )// 0xE7  VK_PACKET	
                             // 0xE8  undefined
  DKND(6,OEM_RESET          )// 0xE9  VK_OEM_RESET	
  DKND(6,OEM_JUMP           )// 0xEA  VK_OEM_JUMP	
  DKND(6,OEM_PA1            )// 0xEB  VK_OEM_PA1	
  DKND(6,OEM_PA2            )// 0xEC  VK_OEM_PA2	
  DKND(6,OEM_PA3            )// 0xED  VK_OEM_PA3	
  DKND(6,OEM_WSCTRL         )// 0xEE  VK_OEM_WSCTRL	
  DKND(6,OEM_CUSEL          )// 0xEF  VK_OEM_CUSEL	
  DKND(6,OEM_ATTN           )// 0xF0  VK_OEM_ATTN	
  DKND(6,OEM_FINISH         )// 0xF1  VK_OEM_FINISH	
  DKND(6,OEM_COPY           )// 0xF2  VK_OEM_COPY	
  DKND(6,OEM_AUTO           )// 0xF3  VK_OEM_AUTO	
  DKND(6,OEM_ENLW           )// 0xF4  VK_OEM_ENLW	
  DKND(6,OEM_BACKTAB        )// 0xF5  VK_OEM_BACKTAB	
  DKND(6,ATTN               )// 0xF6  VK_ATTN	
  DKND(6,CRSEL              )// 0xF7  VK_CRSEL	
  DKND(6,EXSEL              )// 0xF8  VK_EXSEL	
  DKND(6,EREOF              )// 0xF9  VK_EREOF	
  DKND(6,PLAY               )// 0xFA  VK_PLAY	
  DKND(6,ZOOM               )// 0xFB  VK_ZOOM	
  DKND(6,NONAME             )// 0xFC  VK_NONAME	
  DKND(6,PA1                )// 0xFD  VK_PA1	
  DKND(6,OEM_CLEAR          )// 0xFE  VK_OEM_CLEAR	

  DKNE(CANCEL             )//0x03  VK_CANCEL    
  DKNE(CLEAR              )//0x0C  VK_CLEAR     
  DKNE(RETURN             )//0x0D  VK_RETURN    
//DKNE(MENU               )//0x12  VK_MENU      
  DKNE(CAPITAL            )//0x14  VK_CAPITAL   
  DKNE(HANGEUL            )//0x15  VK_HANGEUL   
  DKNE(HANGUL             )//0x15  VK_HANGUL    
  DKNE(KANJI              )//0x19  VK_KANJI     
  DKNE(ESCAPE             )//0x1B  VK_ESCAPE    
  DKNE(EXECUTE            )//0x2B  VK_EXECUTE   
  DKNE(SNAPSHOT           )//0x2C  VK_SNAPSHOT  
  DKNE(SCROLL             )//0x91  VK_SCROLL    
  DKNE(OEM_FJ_JISHO       )//0x92 VK_OEM_FJ_JISHO 


#undef DKNU
#undef DKNC
#undef DKND
#undef DKNE
