#define DKND(t,n) addknv(#n,VK_##n);vkname[VK_##n]=#n;vktype[VK_##n]=t;
#define DKNE(n  ) addknv(#n,VK_##n);
  DKND(0,LBUTTON            )
  DKND(0,RBUTTON            )
  DKND(2,BREAK              )
  DKND(0,MBUTTON            )
  DKND(0,XBUTTON1           )
  DKND(0,XBUTTON2           )
  DKND(2,BACK               )
  DKND(2,TAB                )
  DKND(2,BEGIN              )
  DKND(2,ENTER              )
  DKND(1,SHIFT              )
  DKND(1,CONTROL            )
  DKND(1,ALT                )
  DKND(6,PAUSE              )
  DKND(0,CAPSLOCK           )
  DKND(0,KANA               )
  DKND(0,IME_ON             )
  DKND(0,JUNJA              )
  DKND(0,FINAL              )
  DKND(0,HANJA              )
  DKND(0,IME_OFF            )
  DKND(2,ESC                )
  DKND(2,CONVERT            )
  DKND(2,NONCONVERT         )
  DKND(2,ACCEPT             )
  DKND(2,MODECHANGE         )
  DKND(2,SPACE              )
  DKND(6,PRIOR              )
  DKND(6,NEXT               )
  DKND(6,END                )
  DKND(6,HOME               )
  DKND(6,LEFT               )
  DKND(6,UP                 )
  DKND(6,RIGHT              )
  DKND(6,DOWN               )
  DKND(6,SELECT             )
  DKND(6,PRINT              )
  DKND(6,EXEC               )
  DKND(6,PRINTSCREEN)
  DKND(6,INSERT             )
  DKND(6,DELETE             )
  DKND(6,HELP               )
  DKND(1,LWIN               )
  DKND(1,RWIN               )
  DKND(1,Menu               )
  DKND(2,SLEEP              )
  DKND(2,NUMPAD0            )
  DKND(2,NUMPAD1            )
  DKND(2,NUMPAD2            )
  DKND(2,NUMPAD3            )
  DKND(2,NUMPAD4            )
  DKND(2,NUMPAD5            )
  DKND(2,NUMPAD6            )
  DKND(2,NUMPAD7            )
  DKND(2,NUMPAD8            )
  DKND(2,NUMPAD9            )
  DKND(2,MULTIPLY           )
  DKND(2,ADD                )
  DKND(2,SEPARATOR          )
  DKND(2,SUBTRACT           )
  DKND(2,DECIMAL            )
  DKND(2,DIVIDE             )
  DKND(6,F1                 )
  DKND(6,F2                 )
  DKND(6,F3                 )
  DKND(6,F4                 )
  DKND(6,F5                 )
  DKND(6,F6                 )
  DKND(6,F7                 )
  DKND(6,F8                 )
  DKND(6,F9                 )
  DKND(6,F10                )
  DKND(6,F11                )
  DKND(6,F12                )
  DKND(6,F13                )
  DKND(6,F14                )
  DKND(6,F15                )
  DKND(6,F16                )
  DKND(6,F17                )
  DKND(6,F18                )
  DKND(6,F19                )
  DKND(6,F20                )
  DKND(6,F21                )
  DKND(6,F22                )
  DKND(6,F23                )
  DKND(6,F24                )
  DKND(6,NUMLOCK            )
  DKND(6,SCROLLLOCK         )
  DKND(6,OEM_NEC_EQUAL      )
  DKND(6,OEM_FJ_MASSHOU     )
  DKND(6,OEM_FJ_TOUROKU     )
  DKND(6,OEM_FJ_LOYA        )
  DKND(6,OEM_FJ_ROYA        )
  DKND(1,LSHIFT             )
  DKND(1,RSHIFT             )
  DKND(1,LCONTROL           )
  DKND(1,RCONTROL           )
  DKND(1,LALT               )
  DKND(1,RALT               )
  DKND(6,BROWSER_BACK       )
  DKND(6,BROWSER_FORWARD    )
  DKND(6,BROWSER_REFRESH    )
  DKND(6,BROWSER_STOP       )
  DKND(6,BROWSER_SEARCH     )
  DKND(6,BROWSER_FAVORITES  )
  DKND(6,BROWSER_HOME       )
  DKND(6,VOLUME_MUTE        )
  DKND(6,VOLUME_DOWN        )
  DKND(6,VOLUME_UP          )
  DKND(6,MEDIA_NEXT_TRACK   )
  DKND(6,MEDIA_PREV_TRACK   )
  DKND(6,MEDIA_STOP         )
  DKND(6,MEDIA_PLAY_PAUSE   )
  DKND(6,LAUNCH_MAIL        )
  DKND(6,LAUNCH_MEDIA_SELECT)
  DKND(6,LAUNCH_APP1        )
  DKND(6,LAUNCH_APP2        )
  DKND(2,OEM_1              )
  DKND(2,OEM_PLUS           )
  DKND(2,OEM_COMMA          )
  DKND(2,OEM_MINUS          )
  DKND(2,OEM_PERIOD         )
  DKND(2,OEM_2              )
  DKND(2,OEM_3              )
  DKND(2,OEM_4              )
  DKND(2,OEM_5              )
  DKND(2,OEM_6              )
  DKND(2,OEM_7              )
  DKND(2,OEM_8              )
  DKND(2,OEM_AX             )
  DKND(2,OEM_102            )
  DKND(6,ICO_HELP           )
  DKND(6,ICO_00             )
  DKND(0,PROCESSKEY         )
  DKND(6,ICO_CLEAR          )
  DKND(6,PACKET             )
  DKND(6,OEM_RESET          )
  DKND(6,OEM_JUMP           )
  DKND(6,OEM_PA1            )
  DKND(6,OEM_PA2            )
  DKND(6,OEM_PA3            )
  DKND(6,OEM_WSCTRL         )
  DKND(6,OEM_CUSEL          )
  DKND(6,OEM_ATTN           )
  DKND(6,OEM_FINISH         )
  DKND(6,OEM_COPY           )
  DKND(6,OEM_AUTO           )
  DKND(6,OEM_ENLW           )
  DKND(6,OEM_BACKTAB        )
  DKND(6,ATTN               )
  DKND(6,CRSEL              )
  DKND(6,EXSEL              )
  DKND(6,EREOF              )
  DKND(6,PLAY               )
  DKND(6,ZOOM               )
  DKND(6,NONAME             )
  DKND(6,PA1                )
  DKND(6,OEM_CLEAR          )
#if _WIN32_WINNT >= 0x0604
  DKND(6,NAVIGATION_VIEW    )
  DKND(6,NAVIGATION_MENU    )
  DKND(6,NAVIGATION_UP      )
  DKND(6,NAVIGATION_DOWN    )
  DKND(6,NAVIGATION_LEFT    )
  DKND(6,NAVIGATION_RIGHT   )
  DKND(6,NAVIGATION_ACCEPT  )
  DKND(6,NAVIGATION_CANCEL  )
#endif
#if _WIN32_WINNT >= 0x0604
  DKND(6,GAMEPAD_A                         )
  DKND(6,GAMEPAD_B                         )
  DKND(6,GAMEPAD_X                         )
  DKND(6,GAMEPAD_Y                         )
  DKND(6,GAMEPAD_RIGHT_SHOULDER            )
  DKND(6,GAMEPAD_LEFT_SHOULDER             )
  DKND(6,GAMEPAD_LEFT_TRIGGER              )
  DKND(6,GAMEPAD_RIGHT_TRIGGER             )
  DKND(6,GAMEPAD_DPAD_UP                   )
  DKND(6,GAMEPAD_DPAD_DOWN                 )
  DKND(6,GAMEPAD_DPAD_LEFT                 )
  DKND(6,GAMEPAD_DPAD_RIGHT                )
  DKND(6,GAMEPAD_MENU                      )
  DKND(6,GAMEPAD_VIEW                      )
  DKND(6,GAMEPAD_LEFT_THUMBSTICK_BUTTON    )
  DKND(6,GAMEPAD_RIGHT_THUMBSTICK_BUTTON   )
  DKND(6,GAMEPAD_LEFT_THUMBSTICK_UP        )
  DKND(6,GAMEPAD_LEFT_THUMBSTICK_DOWN      )
  DKND(6,GAMEPAD_LEFT_THUMBSTICK_RIGHT     )
  DKND(6,GAMEPAD_LEFT_THUMBSTICK_LEFT      )
  DKND(6,GAMEPAD_RIGHT_THUMBSTICK_UP       )
  DKND(6,GAMEPAD_RIGHT_THUMBSTICK_DOWN     )
  DKND(6,GAMEPAD_RIGHT_THUMBSTICK_RIGHT    )
  DKND(6,GAMEPAD_RIGHT_THUMBSTICK_LEFT     )
#endif 
  DKNE(CANCEL             )
  DKNE(CLEAR              )
  DKNE(RETURN             )
//DKNE(MENU               )
  DKNE(CAPITAL            )
  DKNE(HANGEUL            )
  DKNE(HANGUL             )
  DKNE(KANJI              )
  DKNE(ESCAPE             )
  DKNE(EXECUTE            )
  DKNE(SNAPSHOT           )
  DKNE(APPS               )
  DKNE(SCROLL             )
  DKNE(OEM_FJ_JISHO       )
#undef DKNU
#undef DKNC
#undef DKND
#undef DKNE
