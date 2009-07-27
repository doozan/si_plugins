#include "Plugin.h"
#include "StrLib.h"

#ifndef VK_BROWSER_BACK

#define VK_BROWSER_BACK        0xA6
#define VK_BROWSER_FORWARD     0xA7
#define VK_BROWSER_REFRESH     0xA8
#define VK_BROWSER_STOP        0xA9
#define VK_BROWSER_SEARCH      0xAA
#define VK_BROWSER_FAVORITES   0xAB
#define VK_BROWSER_HOME        0xAC

#define VK_VOLUME_MUTE         0xAD
#define VK_VOLUME_DOWN         0xAE
#define VK_VOLUME_UP           0xAF
#define VK_MEDIA_NEXT_TRACK    0xB0
#define VK_MEDIA_PREV_TRACK    0xB1
#define VK_MEDIA_STOP          0xB2
#define VK_MEDIA_PLAY_PAUSE    0xB3
#define VK_LAUNCH_MAIL         0xB4
#define VK_LAUNCH_MEDIA_SELECT 0xB5
#define VK_LAUNCH_APP1         0xB6
#define VK_LAUNCH_APP2         0xB7

#endif

struct s_data {
   HANDLE   hFileShared;
   HWND     hWndTarget;
   BOOL     bKeyIsDown;
   BOOL     bShifted;
   BOOL     bPrevCharSpecial;
   BOOL     bKeyCombo;
   BOOL     bWinKey;
   BOOL     bModKeyIsDown;
   int      iKeyDownCount;
   char     sPrevChar[32];
   char     sBuf[2048];
};

// experimental "record text" function
//#define RECORD 0

#ifdef RECORD
s_data *map;

HANDLE ghFileShared  = NULL;
#endif


BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
char *ConvertKeysToSequence(char *keys);
char GetVKey(char *key);

void ForceForegroundWindow(HWND hWnd);
void BringToFront(HWND hWnd);
void SendKey (WORD wKy, DWORD dEvent);
void SendKeys(char *szKeys, int iDelay);


void HexToBin(char *dest, char *hex);
void BinToHex(char *dest, char *bin);



LRESULT CALLBACK HotKeyProc(int code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK KeyProc(int code, WPARAM wParam, LPARAM lParam);


BOOL GetSpecialKey(LONG ch, char *dest);

#define UWM_UPDATE   WM_USER+42

#define  KEY_DOWN    (char) 254
#define  KEY_UP      (char) 255

#define VK_ALT    VK_MENU
#define VK_LALT   VK_LMENU
#define VK_RALT   VK_RMENU
#define VK_CTRL   VK_CONTROL
#define VK_LCTRL  VK_LCONTROL
#define VK_RCTRL  VK_RCONTROL
#define VK_WIN    VK_LWIN
#define VK_ENTER  VK_RETURN
#define VK_LBRACKET 0xDB  // value of '[' scan code
#define VK_RBRACKET 0xDD  // value of ']' scan code
