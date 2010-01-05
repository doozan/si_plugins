/*
Copyright (c) 2001-2009 Jeff Doozan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rightsfN
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "Plugin.h"
#include "StrLib.h"
#include "resource.h"
#include <commctrl.h>

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


//#define RECORD 0

BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
char *ConvertKeysToSequence(char *keys);
char GetVKey(char *key);

#ifdef RECORD
s_data *map;

HANDLE ghFileShared  = NULL;
#endif

// handle to user32.dll, used for SendInput 
HMODULE ghDll         = NULL;
typedef UINT (WINAPI *SendInput__)(UINT,LPINPUT,int);
static SendInput__ SendInput_ = NULL;


HHOOK  ghHook        = NULL;
HWND   ghWndCapture  = NULL;

BOOL gbShift = FALSE;
BOOL gbCtrl  = FALSE;
BOOL gbWin   = FALSE;
BOOL gbAlt   = FALSE;
BOOL gbSpecial = FALSE;
char gKey[32];             // Used by HotKeyHook

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

// declare our commands
DECLARE_COMMAND(keys);
DECLARE_COMMAND(hotkey);
DECLARE_COMMAND(passwd);


/*************************************************************\
 *                                                           *
 *  INIT() is called when the plugin is loaded               * 
 *  Initiliaze all of the plugin commands and do any         *
 *  additional initializations your plugin may require       *
 *                                                           *
 *  Return the name of the plugin using INIT_RETURN          *
 *                                                           *
\*************************************************************/

INIT()
{
   INIT_COMMAND_CUSTOM   (keys,     STR(KEYS),       STR(KEYS_DESC),       ICON_KEYS,      keys,      keys,      keys,      keys);
   INIT_COMMAND_CUSTOM   (hotkey,   STR(HOTKEY),     STR(HOTKEY_DESC),     ICON_HOTKEY,    keys,      hotkey,    hotkey,    hotkey);
   INIT_COMMAND_CUSTOM   (passwd,   STR(PASS),       STR(PASS_DESC),       ICON_HOTKEY,    keys,      passwd,    passwd,    passwd);

   ghDll = LoadLibrary("user32");
   if (ghDll)
     SendInput_ = (SendInput__) GetProcAddress(ghDll, "SendInput");

   INIT_RETURN("Send Keystrokes");
}




/**************************************************************************\
 *                                                                        *
 *  EXEC functions are called when the command needs to be performed      *
 *                                                                        *
 *  Any return value will be saved, and passed back in lpAction->lpData   *
 *  to any subsequent calls to the EXEC function.                         *
 *                                                                        *
\**************************************************************************/

void BinToHex(char *dest, char *bin) {
   while (*bin) {
      *dest++ =  (*bin   & 0x0F) + 'A';
      *dest++ = ((*bin++ & 0xF0) >> 4) + 'A';
   }
   *dest = 0;
}

void HexToBin(char *dest, char *hex) {

   BOOL bLow = 1;
   while (*hex) {
      if (bLow)
         *dest    = (*hex++ - 'A');
      else
         *dest++ += (*hex++ - 'A') << 4;
      bLow ^= 1;
   }
   if (!bLow) dest++;
   *dest = 0;
}


void ForceForegroundWindow(HWND hWnd) {

#ifndef SPI_GETFOREGROUNDLOCKTIMEOUT

   #define SPI_GETFOREGROUNDLOCKTIMEOUT 2000
   #define SPI_SETFOREGROUNDLOCKTIMEOUT 2001

#endif

   HWND hWndActive = GetForegroundWindow();
   BOOL bAttached = AttachThreadInput( GetWindowThreadProcessId(hWndActive, NULL), GetCurrentThreadId(), TRUE);
   
   BringWindowToTop(hWnd);
   SetForegroundWindow(hWnd);
   
   BOOL bDone = (GetForegroundWindow() == hWnd);
   if (!bDone) {
      DWORD dwTimeout;
      DWORD dwZero = 0;
      
      SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &dwTimeout, 0);
      SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, &dwZero, SPIF_SENDCHANGE);
      BringWindowToTop(hWnd); // IE 5.5 related hack
      SetForegroundWindow(hWnd);
      SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, &dwTimeout, SPIF_SENDCHANGE);
   }

   if (bAttached)
      AttachThreadInput( GetWindowThreadProcessId(hWndActive, NULL), GetCurrentThreadId(), FALSE);

}


void SendKey (WORD wKy, DWORD dEvent)
{
  if (SendInput_)
  {
    INPUT input;
    memset(&input, 0, sizeof(input));
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = wKy;
    input.ki.dwFlags = dEvent;

    INT ret = SendInput_(1, &input, sizeof(INPUT));
/*
    char buf[128];
    wsprintf(buf, "Sent %d keys", ret);
    OutputDebugString(buf);
*/
  }
  else
  {
    keybd_event((BYTE)wKy, NULL, dEvent, NULL);
  }
}

EXEC(keys)
{
  s_execParams *params = _lpAction;

   // if the window the gesture was drawn over is not actually
   // the active window, then we need to activate it so that the
   // keystrokes will be sent to it

   HWND hWndFG = GetForegroundWindow();
   HWND hWndFGParent = hWndFG;
   while (hWndFGParent) {
      hWndFG = hWndFGParent;
      hWndFGParent = GetParent(hWndFG);
   }
   if (hWndFG != params->hWnd)
      ForceForegroundWindow(params->hWnd);

/*
    char buf[1024];
    wsprintf(buf, "Target window: %x, foreground %x", params->hWnd, hWndFG);
    OutputDebugString(buf);
*/

   if (!params->lpData)
   {
      char *inStr = RestoreStr();

      if (params->id == _pCmd_passwd->id)
      {
         char buf[256];

         int slen = (inStr[0] - 32) % 255;        // add 23 to the length, and rotate it via 255
         
         HexToBin(buf, inStr+1);
         
         char *ch = buf;
         while (slen) {
            if (*ch != 0x45)
               *ch ^= 0x45;
            ch++;
            slen--;
         }
         *ch = 0;

         params->lpData = ConvertKeysToSequence(buf);
      }
      else
         params->lpData = ConvertKeysToSequence(inStr);
   }

   if (params->lpData) {

      char *c = (char *) params->lpData;

      while (*c) {
         // flag - next key is "keydown"
         if (*c == KEY_DOWN) {
            c++;
            SendKey(*c, NULL);
         }
         // flag - next key is "keyup"
         else if (*c == KEY_UP) {
            c++;
            SendKey(*c, KEYEVENTF_KEYUP);
         }
         // key is normal
         else {
           SendKey(*c, NULL);
           Sleep(10);
           SendKey(*c, KEYEVENTF_KEYUP);
         }

         Sleep(10);

         c++;
      }
   }

  EXEC_RETURN(NULL);
}





/**************************************************************\
 *                                                            *
 *  SHOW is called when a command is going to be edited.      *
 *  Create and initialize the appropriate dialog box.         *
 *                                                            *
\**************************************************************/

SHOW(keys)
{
   HWND hWnd = SHOW_DIALOG(DLG_KEYS, DlgProc);
   SET(IDC_HOWTO, STR(KEYS_HELP));

  // lpszSavedParams is a 'hidden' parameter
   if (lpszSavedParams)
   {
     char *paramStart = SkipWhiteSpace(lpszSavedParams);
     // new style config, params are quoted
     if (*paramStart == '"')
     {
       RestoreItem(IDC_KEYS);
     }
     // old style config (no quotes, everything is one parameter, including whitespace)
     else
     {
       SetDlgItemText(hWnd, IDC_KEYS, paramStart);
     }
   }
   SHOW_RETURN();
}


SHOW(hotkey)
{
   HWND hWnd = SHOW_DIALOG(DLG_HOTKEY, DlgProc);
   SET(IDC_HOTKEY_DESC, STR(HOTKEY_LABEL));

   ghWndCapture = GetDlgItem(hWnd, IDC_HOTKEY);
   ghHook = SetWindowsHookEx(WH_KEYBOARD, HotKeyProc, ghInstance, GetCurrentThreadId());


   gbShift = FALSE;
   gbCtrl  = FALSE;
   gbWin   = FALSE;
   gbAlt   = FALSE;
   gbSpecial = FALSE;
   *gKey = 0;
  
   char *inStr = RestoreStr();

   char *keyStart = inStr;
   char *keyEnd = inStr;
   char *key;
   BOOL bFound = FALSE;
   while (keyStart && !bFound) {
      
      keyStart = SkipWhiteSpace(keyStart);
      if ( *keyStart != '[' ||
           ( keyStart[0] == '[' && keyStart[1] == '[' ) // support for Ctrl + [ which would be [CTRL_DOWN][[CTRL_UP]
         ) {
         char *end = mstrchr(keyStart+1, '[');
         if (end) *end = 0;
         lstrcpy(gKey, keyStart);
         if (end) *end = '[';
         bFound = TRUE;
      }
      else {
         
         keyStart = mstrchr(keyStart, '[');
         if (keyStart) {
            keyEnd = mstrchr(keyStart, ']');
            if (keyEnd)
               *keyEnd = 0;
            
            key = keyStart+1;
            if (!lstrcmpi(key, "WIN_DOWN"))
               gbWin = TRUE;
            else if (!lstrcmpi(key, "CTRL_DOWN"))
               gbCtrl = TRUE;
            else if (!lstrcmpi(key, "ALT_DOWN"))
               gbAlt = TRUE;
            else if (!lstrcmpi(key, "SHIFT_DOWN"))
               gbShift = TRUE;

            // if we've come to the modifier_up commands,
            // then this is a modifier-only keystroke
            else if (    !lstrcmpi(key, "WIN_UP")
                 || !lstrcmpi(key, "CTRL_UP")
                 || !lstrcmpi(key, "ALT_UP")
                 || !lstrcmpi(key, "SHIFT_UP")
               )
            {
               bFound = TRUE;
            }

            // it's not a modifier, so it must be the actual key
            else {
               TrimWhiteSpace(key);
               char *end = mstrchr(key, '[');
               
               if (end < keyEnd)  // ex: X[SHIFT_UP]
                  gbSpecial = TRUE;
               
               if (end)
                  *end = 0;
               lstrcpy(gKey, key);
               if (end)
                  *end = '[';
               bFound = TRUE;
            }
            
            if (keyEnd) {
               *keyEnd = ']';
               keyStart = keyEnd+1;
            }
            else
               keyStart = NULL;
         }
      }
   }


   char buf[64] = "";
   BOOL bInit=FALSE;
   
   if (gbWin) {
      if (bInit) lstrcat(buf, " + ");
      lstrcat(buf, "WIN");
      bInit = TRUE;
   }
   
   if (gbCtrl) {
      if (bInit) lstrcat(buf, " + ");
      lstrcat(buf, "CTRL");
      bInit = TRUE;
   }
   
   if (gbAlt) {
      if (bInit) lstrcat(buf, " + ");
      lstrcat(buf, "ALT");
      bInit = TRUE;
   }
   
   if (gbShift) {
      if (bInit) lstrcat(buf, " + ");
      lstrcat(buf, "SHIFT");
      bInit = TRUE;
   }

   if (bInit && *gKey) lstrcat(buf, " + ");
   if (*gKey) lstrcat(buf, gKey);

   SET(IDC_HOTKEY, buf);

   SHOW_RETURN();
}



WNDPROC EditWndProc;
LRESULT CALLBACK NewEditWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   if (uMsg != EM_SETPASSWORDCHAR)
      return CallWindowProc(EditWndProc, hWnd, uMsg, wParam, lParam);
   return FALSE;
}

SHOW(passwd)
{
   HWND hWnd = SHOW_DIALOG(DLG_HOTKEY, DlgProc);
   SET(IDC_HOTKEY_DESC, STR(PASS_LABEL));

   SendDlgItemMessage(hWnd, IDC_HOTKEY, EM_SETPASSWORDCHAR, (WPARAM) '*', 0);

   // subclass the edit window proc, so we can discard any subversive SETPASSWORDCHAR messages
   EditWndProc = (WNDPROC) GetWindowLong(GetDlgItem(hWnd, IDC_HOTKEY), GWL_WNDPROC);
   SetWindowLong(GetDlgItem(hWnd, IDC_HOTKEY), GWL_WNDPROC, (LONG) NewEditWndProc);


   char *inStr = mstrdup(RestoreStr());
   if (inStr && *inStr) {
      
      int slen = (inStr[0] - 32) % 255;        // add 23 to the length, and rotate it via 255
      
      HexToBin(inStr+1, inStr+1);
      
      char *ch = inStr+1;
      while (slen) {
         if (*ch != 0x45)
            *ch ^= 0x45;
         ch++;
         slen--;
      }
      *ch = 0;
      
      slen = lstrlen(inStr+1);
      if (slen < 32) {
         unsigned char buf[33];
         mmemcpy(buf, inStr+1, slen);
         while (slen<32)
            buf[slen++] = 0x01;
         buf[32] = 0;
         SET(IDC_HOTKEY, (char *) buf);
      }
      else
         SET(IDC_HOTKEY, inStr+1);   
   }

   delete inStr;
   SHOW_RETURN();
}


/*****************************************************************\
 *                                                               *
 *  SAVE is called when the config dialog is about to be closed  *
 *                                                               *
 *     HWND  hWnd    - handle to the dialog box created in SHOW  *
 *     int   id      - id of the function to save                *
 *     int  *lenght  - the length of the returned string         *
 *     int  *lpszBuf - the buffer to store the returned string   *
 *     void *data    - pointer to any data previously            *
 *                     associated with this command              *
 *                                                               *
 *  If lpszBuf is not null, copy the text parameters into it.    *
 *  Always set lenght to the lenght of the text parameters.      *
 *  Always return a pointer to the data associated with this     *
 *  command.  If you are not associating data, return NULL       *
 *                                                               *
\*****************************************************************/

SAVE(keys)
{
  char *params = new char[4096];
  GetDlgItemText(hWnd, IDC_KEYS, params, 4096);
  char *newline = mstrstr(params, "\r\n");
  while (newline) {
    lstrcpy(newline, newline+2);
    newline = mstrstr(newline, "\r\n");
  }

  SaveStr(params);
  delete params;

  SAVE_RETURN();
}

SAVE(hotkey)
{
   if (ghHook) {
      UnhookWindowsHookEx(ghHook);
      ghHook = NULL;
   }  

   char szHotKey[128];  // longest possible hotkey is "[WIN_DOWN][CTRL_DOWN][ALT_DOWN][SHIFT_DOWN][VK_LAUNCH_MEDIA_SELECT][SHIFT_UP][ALT_UP][CTRL_UP][WIN_UP]"
   *szHotKey = NULL;

   if (gbWin)
      lstrcat(szHotKey, "[WIN_DOWN]");
   if (gbCtrl)
      lstrcat(szHotKey, "[CTRL_DOWN]");
   if (gbAlt)
      lstrcat(szHotKey, "[ALT_DOWN]");
   if (gbShift)
      lstrcat(szHotKey, "[SHIFT_DOWN]");

   if (gbSpecial)
      lstrcat(szHotKey, "[");

   lstrcat(szHotKey, gKey);

   if (gbSpecial)
      lstrcat(szHotKey, "]");

   if (gbShift)
      lstrcat(szHotKey, "[SHIFT_UP]");
   if (gbAlt)
      lstrcat(szHotKey, "[ALT_UP]");
   if (gbCtrl)
      lstrcat(szHotKey, "[CTRL_UP]");
   if (gbWin)
      lstrcat(szHotKey, "[WIN_UP]");

   SaveStr(szHotKey);

   SAVE_RETURN();
}

SAVE(passwd)
{
   char buf[120];
   unsigned char slen = GetDlgItemText(hWnd, IDC_HOTKEY, buf, 120);

   int x=0;
   while (x<slen) {
      if ( (unsigned char) buf[x] == 0x01) {
         buf[x] = 0;
         break;
      }
      x++;
   }
   slen = x;

   int padding = 16 - (slen % 16);  // padding will always be a multiple of 16
   if (padding) padding--;          // subtract one for the byte representing the length
   
   char *ch = buf;
   while (*ch) {
      if (*ch != 0x45)
         *ch ^= 0x45;   // toggle the high bit
      ch++;
   }
   while (padding) {
      *ch = (*(ch-1) + 44 * 33) ^ 149 %255;   // rand();
      ch++;
      padding--;
   }
   *ch = 0;

  char szOut[255];
  char *szHex = szOut+1;
  BinToHex(szHex, buf);

  slen = (slen + 32) % 255;        // add 32 to the length, and rotate it via 255
 
  szOut[0] = (char) slen;
  SaveStr(szOut);

  SAVE_RETURN();
}


/***********************************************************\
 *                                                         *
 *  CLEAR is called when a command associated with this    *
 *  plugin is about to be destroyed.  This is your chance  *
 *  to delete any data you have stored in the command      *
 *                                                         *
 *  CLEAR functions recieve only one parameter, data,      *
 *  which contains a pointer to any data associated with   *
 *  this command.                                          *
 *                                                         *
\***********************************************************/

CLEAR(keys)
{
  delete CLEAR_GET_DATA();
  CLEAR_RETURN();
}

CLEAR(hotkey)
{
  delete CLEAR_GET_DATA();
  CLEAR_RETURN();
}

CLEAR(passwd)
{
  delete CLEAR_GET_DATA();
  CLEAR_RETURN();
}

/**************************************************\
 *                                                *
 *  Quit is called before the plugin is unloaded  *
 *  This is the place to do you code cleanup      *
 *                                                *
\**************************************************/


QUIT()
{
  if (ghDll)
    FreeLibrary(ghDll);

  if (ghHook) {     // we need to refcount, this won't really work
    UnhookWindowsHookEx(ghHook);
    ghHook = NULL;
  }

  QUIT_RETURN();
}



extern HANDLE ghFileShared;

BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   switch (uMsg) {

#ifdef RECORD
      case WM_COMMAND:
         switch (LOWORD(wParam)) {
            case IDC_RECORD:
               if (ghHook) {
                  UnhookWindowsHookEx(ghHook);
                  ghHook = NULL;
                  UnmapViewOfFile(ghFileShared);
                  SET(IDC_RECORD, "Record");
               }
               else {
                  if (!ghFileShared) {
                     ghFileShared = CreateFileMapping( (HANDLE) 0xFFFFFFFF, NULL, PAGE_READWRITE, 0, sizeof(s_data), "SIKEYHOOK" );
                     map = (s_data *) MapViewOfFile( ghFileShared, FILE_MAP_WRITE, 0, 0, 0 );
                  }

                  map->sBuf[0] = 0;
                  map->hWndTarget = hWnd; //GetDlgItem(hWnd, IDC_KEYS);
                  map->iKeyDownCount = 0;
                  map->sPrevChar[0] = 0;
                  map->bPrevCharSpecial = FALSE;
                  map->bKeyIsDown = FALSE;
                  map->bShifted = FALSE;
                  map->bKeyCombo = FALSE;
                  map->bWinKey = FALSE;
                  map->bModKeyIsDown = FALSE;

                  ghHook = SetWindowsHookEx(WH_KEYBOARD, KeyProc, ghInstance, 0);
                  SET(IDC_RECORD, "StopRecording");
               }

               break;
         }
         break;
#endif
      case WM_CLOSE:
			DestroyWindow(hWnd);
			break;

#ifdef RECORD
      case UWM_UPDATE:  // updated edit box
         SetDlgItemText(hWnd, IDC_KEYS, map->sBuf);
         SendDlgItemMessage(hWnd, IDC_KEYS, EM_SETSEL, -2, -2);      // -2 actually becomes the greatest possible positve value
         SendDlgItemMessage(hWnd, IDC_KEYS, EM_SCROLLCARET, 0, 0);
         break;
#endif
		default:
			return FALSE;
   }
   return TRUE;
}



#ifdef RECORD

int ProcessKey(WPARAM wParam, LPARAM lParam);


LRESULT CALLBACK KeyProc(int code, WPARAM wParam, LPARAM lParam) {

   if ( !(HIWORD(lParam) & KF_REPEAT) || (HIWORD(lParam) & KF_UP) ) {
      if ( ProcessKey(wParam, lParam) )
         return 1;
   }
   else if ( HIWORD(lParam) & KF_REPEAT )
      if (IsChild(GetActiveWindow(), map->hWndTarget))
         return 1;


   CallNextHookEx(ghHook, code, wParam, lParam);
   return 0;
}

int ProcessKey(WPARAM wParam, LPARAM lParam) {

   char tempBuf[32] = "";  
   BOOL bKeyUp = HIWORD(lParam) & KF_UP;
   BYTE kbState[256] = {0};

   
   // Get the appropriate key name
   
   BOOL bSpecial = GetSpecialKey(wParam, tempBuf);

   // any normal character, or something we don't know about
   // let windows handle it through ToAscii
   if (!bSpecial) {
      WORD tempWord;
      if ( (GetAsyncKeyState(VK_SHIFT) & 0x8000) ) {
         if (
            ( !bKeyUp && (map->iKeyDownCount <= 1) ) ||
            (  bKeyUp && (map->iKeyDownCount <= 2) ) ) {
            map->bShifted = TRUE;
            kbState[VK_SHIFT] = 0x80;
         }
      }
      ToAscii( (wParam | 0x100), NULL, (unsigned char *) kbState, &tempWord, NULL);
      
      char ch = LOBYTE(tempWord);
      
      if (ch == '[' || ch == ']' || ch == '\\') {
         tempBuf[0] = '\\';
         tempBuf[1] = ch;
         tempBuf[2] = 0;
      }
      else {
         tempBuf[0] = ch;
         tempBuf[1] = 0;
      }
   }
   else if ( (wParam == VK_SHIFT) && (!map->bKeyIsDown) && !bKeyUp )
      map->bShifted = TRUE;
   
   
  
//#if 0
   if (bKeyUp) {
      lstrcat(map->sBuf, "[");
      lstrcat(map->sBuf, tempBuf);
      lstrcat(map->sBuf, "_UP]");
   }
   else {
      lstrcat(map->sBuf, "[");
      lstrcat(map->sBuf, tempBuf);
      lstrcat(map->sBuf, "_DOWN]");
   }
   
//#else

   // [CTRL_DOWN] [ALT_DOWN]  [S_DOWN] [S_UP]         [ALT_UP]   [CTRL_UP]
   // [CTRL_DOWN] [ALT_DOWN]           [A_UP]         [ALT_UP]   [CTRL_UP]
   //             [ALT_DOWN]           [TAB_UP]       [ALT_UP]
   //             [LWIN_DOWN]          [M_UP]         [LWIN_UP]
   //             [LWIN_DOWN]
   //                                  [SNAPSHOT_UP]
   


   if (bKeyUp) {
      
      map->iKeyDownCount--;
      if (map->iKeyDownCount < 0) {
         map->iKeyDownCount = 0;
         map->bKeyCombo = TRUE;
      }
      
      
      // shift key, special case
      if (map->bShifted && wParam == VK_SHIFT) {
         // special case, shift was just pressed once by itself
         if (lstrcmpi(map->sPrevChar, tempBuf) == 0) {
            lstrcat(map->sBuf, "[SHIFT]");
            map->bKeyIsDown = FALSE;
         }
         map->bShifted = FALSE;
      }

      // windows key, special case
      else if (map->bWinKey) {
         map->bKeyCombo = TRUE;         
      }

      // any other key up event
      else {
         BOOL bExplicitUp = FALSE;


         // if it's a "special" key, and it's not the same as the
         // previous char (the key that most recently went down)
         // then it's a "combination" or something, so we need to
         // explicitly specify it as a keyup
         if (bSpecial && lstrcmpi(map->sPrevChar, tempBuf) != 0)
            bExplicitUp = TRUE;
         else
            map->bKeyIsDown = FALSE;



         // if it's a key combo, we're printing the "simple" characters
         // here instead of on keydown because windows traps the keydown
         // messages for associated hotkeys
         if (map->bKeyCombo) { // && map->bKeyIsDown) {
            
            if (map->bPrevCharSpecial) {
               lstrcat(map->sBuf, "[");
               lstrcat(map->sBuf, map->sPrevChar);
               lstrcat(map->sBuf, "_DOWN]");
            }
            else
               lstrcat(map->sBuf, map->sPrevChar);
            
            if (!bSpecial) {            
               lstrcat(map->sBuf, tempBuf);
            }

            map->bKeyCombo = (map->iKeyDownCount == 0);
         }
         
         else if (bExplicitUp) {
            lstrcat(map->sBuf, "[");
            if (!bSpecial)
               GetKeyNameText(lParam, tempBuf, 32);
            lstrcat(map->sBuf, tempBuf);
            lstrcat(map->sBuf, "_UP]");
         }
         else if (bSpecial) {
            lstrcat(map->sBuf, "[");
            lstrcat(map->sBuf, tempBuf);
            lstrcat(map->sBuf, "]");
         }
         
         lstrcpy(map->sPrevChar, tempBuf);
         map->bPrevCharSpecial = bSpecial;
      }
      
   }
   
   // key down
   
   else {
      
      // the windows key handling is a real mess...basically, windows traps
      // random keyboard messages when it recognizes a hotkey.  For example,
      // Ctrl+Alt+A would send ctrl_down, alt_down, a_UP, alt_up, ctrl_up
      // but it would trap the a_down.   The windows key is even trickier,
      // because it can be a hotkey all by itself.  When the win key is pressed
      // and released, it only sends  win_down, but never win_up.  However,
      // when it's pressed in combination with "X" (which is not a windows
      // hotkey), it sends win_down, x_down, x_up, win_up.  But when it's
      // used with "M", which IS a hotkey, it only sends win_down, m_up, win_up
      
      // the case we're looking for right here is the single press.  bWinKey
      // is flagged when the windows key is pressed.  If it is released, we'll
      // never know until another key is pressed, but if this flag is still
      // set, and the explicit check shows that the key isn't really down
      // anmore, we'll assume that it's been released and was just a single
      // key press
      
      // normal combo key handling is a bit messy, too, for the same reasons
      
      
      if (map->bWinKey) {
         // if they key isn't down anymore, then it was just pressed once
         if ( !(GetAsyncKeyState(VK_LWIN) & 0x8000) && !(GetAsyncKeyState(VK_RWIN) & 0x8000)) {
            lstrcat(map->sBuf, "[");
            lstrcat(map->sBuf, map->sPrevChar);
            lstrcat(map->sBuf, "]");
            map->bKeyCombo = FALSE;
            map->iKeyDownCount--;
            map->bKeyIsDown = FALSE;   // the windows key is no longer down
         }
         
         // the key is still down, so it's a combo
         else {
            lstrcat(map->sBuf, "[");
            lstrcat(map->sBuf, map->sPrevChar);
            lstrcat(map->sBuf, "_DOWN]");
            map->bKeyCombo = TRUE;
         }
         map->bWinKey = FALSE;
      }         
      else {
         
         if (wParam == VK_LWIN || wParam == VK_RWIN)
            map->bWinKey = TRUE;
         
         // we've got a key combo goin' on here
         if (map->bKeyIsDown && !map->bShifted) {  // if a key is being held down, and it's not "SHIFT"
            if (map->bPrevCharSpecial) {           //  if the key down is a special character
               if (lstrcmpi(map->sPrevChar, tempBuf) != 0) {
                  lstrcat(map->sBuf, "[");
                  lstrcat(map->sBuf, map->sPrevChar);
                  lstrcat(map->sBuf, "_DOWN]");
                  map->bKeyCombo = TRUE;

               }
            }
         }

         if (wParam == VK_ALT || wParam == VK_CTRL)
            map->bModKeyIsDown = TRUE;

      }

      if (!bSpecial && !map->bKeyCombo) {
         lstrcat(map->sBuf, tempBuf);
      }
      map->iKeyDownCount++;
      map->bKeyIsDown = TRUE;

      lstrcpy(map->sPrevChar, tempBuf);
      map->bPrevCharSpecial = bSpecial;
   }
//#endif
   
   SendMessage(map->hWndTarget, UWM_UPDATE, 0, 0);
  
   return IsChild(GetActiveWindow(), map->hWndTarget);
}

#endif



LRESULT CALLBACK HotKeyProc(int code, WPARAM wParam, LPARAM lParam) {
  
   BOOL bTrap = (ghWndCapture == GetFocus());
   if (bTrap && (code == HC_ACTION)  && !(HIWORD(lParam) & KF_REPEAT) ) {

      if (
         ( (wParam == VK_ALT)   && (gbAlt)   ) ||
         ( (wParam == VK_SHIFT) && (gbShift) ) ||
         ( (wParam == VK_RWIN)  && (gbWin)   ) ||
         ( (wParam == VK_LWIN)  && (gbWin)   ) ||
         ( (wParam == VK_CTRL)  && (gbCtrl)  )
         )
      {
         return 1;
      }

      // reset gKey and gbSpecial
      gbSpecial = FALSE;
      *gKey = 0;

      gbWin =   ( (GetAsyncKeyState(VK_LWIN)  & 0x8000) != 0);
      if (!gbWin)
         gbWin = ( (GetAsyncKeyState(VK_RWIN)  & 0x8000) != 0);

      gbCtrl =  ( (GetAsyncKeyState(VK_CTRL)  & 0x8000) != 0);
      gbAlt =   ( (GetAsyncKeyState(VK_ALT)   & 0x8000) != 0);
      gbShift = ( (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
          
      
      char buf[32] = "";
      BOOL bInit = FALSE;

      if (gbWin) {
         if (bInit) lstrcat(buf, " + ");
         lstrcat(buf, "WIN");
         bInit = TRUE;
      }

      if (gbCtrl) {
         if (bInit) lstrcat(buf, " + ");
         lstrcat(buf, "CTRL");
         bInit = TRUE;
      }

      if (gbAlt) {
         if (bInit) lstrcat(buf, " + ");
         lstrcat(buf, "ALT");
         bInit = TRUE;
      }

      if (gbShift) {
         if (bInit) lstrcat(buf, " + ");
         lstrcat(buf, "SHIFT");
         bInit = TRUE;
      }

      if ( !(
              (wParam == VK_ALT)   ||
              (wParam == VK_SHIFT) ||
              (wParam == VK_LWIN)   ||
              (wParam == VK_RWIN)   ||
              (wParam == VK_CTRL))
            )
      {
         // we use GetKeyName to get the "correct" name
         gbSpecial = GetSpecialKey(wParam, gKey);

         // any normal character, or something we don't know about
         // let windows handle it through GetKeyNameText
         if (!gbSpecial) {
           GetKeyNameText(lParam, gKey, 32);
         }

         // if it's an alphabetic key, make it lowercase
         if (lstrlen(gKey) == 1 && *gKey >= 'A' && *gKey <= 'Z')
           *gKey += 'a'-'A';

         if (bInit)
            lstrcat(buf, " + ");
         lstrcat(buf, gKey);
      }

      SetWindowText(ghWndCapture, buf);

      // don't allow any further processing
      return 1;
   }

   CallNextHookEx(ghHook, code, wParam, lParam);
   return 0;
}










char *ConvertKeysToSequence(char *keys) {

   if (!keys)
      return NULL;
/*
   char *unescaped = mstrdup(keys);
   Unescape(unescaped);

   char *data = unescaped;
*/
   char *data = keys;

   char *buf = new char[lstrlen(data) + 1024];
   *buf = 0;
   int count=0;

   // look for constants
   // [KEYNAME] [KEYNAME_DOWN] [KEYNAME_UP]
   while (data && *data) {

     if (*data == '[') { 

         char *end = FindSeparator(data,']');
  
         if (!end)
           // no closing ] brace found, just send it as a keystroke
           buf[count++] = VkKeyScan('[');

         else
         {

           // we've found a set of braces []
           // temporarily null terminate the ]
           // so we can do string comparisons on the data inside
           *end=0;         
   
           char *s = NULL;
           char state = 0;
           if (s = mstrstri(data, "_DOWN")) {
              *s=0;
              state = KEY_DOWN;
           }
           else if (s = mstrstri(data, "_UP")) {
              *s=0;
              state = KEY_UP;
           }

           char key = GetVKey(data);
           // we've found a constant
           if (key) {
              if (state != 0)
                 buf[count++] = state;
              buf[count++] = key;
              data=end;
           }
           // the [ wasn't the start of a constant, just send it as a keystroke
           else
             buf[count++] = VkKeyScan('[');
   
           if (s)   *s = '_';      // restore the _
           if (end) *end = ']';    // restore ]
         }
      }

      else {
         if (*data == '\\')
            if ( (*(data+1) == '[') || (*(data+1) == ']') )
               data++;

         short key = VkKeyScan(*data);

         // if it's some weird extended character, post ALT+0XXX
         if (key == 0xFFFF) {
            char alt[5];
            wsprintf(alt, "%04d", *data);

            buf[count++] = KEY_DOWN;
            buf[count++] = VK_MENU;

            buf[count++] = alt[0];
            buf[count++] = alt[1];
            buf[count++] = alt[2];
            buf[count++] = alt[3];

            buf[count++] = KEY_UP;
            buf[count++] = VK_MENU;
         }

         else if (key) {

            // if the previous command was [CTRL_DOWN] or [ALT_DOWN], then we want to ignore
            // any "special" properties of this char (namely the VK_SHIFT attribute, since that would
            // be the equivalent of Ctrl+Shift+Char instead of the intended Ctrl+C
            if ( (count > 1) && (buf[count-2] == KEY_DOWN) && ((buf[count-1] == VK_CONTROL) || (buf[count-1] == VK_MENU) || (buf[count-1] == VK_SHIFT) || (buf[count-1] == VK_WIN)) )
               buf[count++] = (char) key;

            else {
               if (key & 0x100) { buf[count++] = KEY_DOWN; buf[count++] = VK_SHIFT; }
               if (key & 0x200) { buf[count++] = KEY_DOWN; buf[count++] = VK_CONTROL; }
               if (key & 0x400) { buf[count++] = KEY_DOWN; buf[count++] = VK_MENU; }
               
               buf[count++] = (char) key;
               
               if (key & 0x100) { buf[count++] = KEY_UP; buf[count++] = VK_SHIFT; }
               if (key & 0x200) { buf[count++] = KEY_UP; buf[count++] = VK_CONTROL; }
               if (key & 0x400) { buf[count++] = KEY_UP; buf[count++] = VK_MENU; }
            }
         }
      }
      data++;
   }

   char *ret = NULL;
   if (count > 0) {
      buf[count] = 0;
      ret = mstrdup(buf);
   }

   delete buf;
//   delete unescaped;

   return ret;
}


// Count the number of virtual keys in vkeys.h

#define KEY(VAR) KEYDEF_##VAR,
enum {
   START=0,
   #include "vkeys.h"
   VK_COUNT
};  
#undef KEY

// Create an array of VKNames and VKValues.

#define KEY(VAR) { #VAR, (char) VK_##VAR },

struct key {
   char *szName;
   char lValue;
} keys[VK_COUNT] = {
   #include "vkeys.h"   
};
#undef KEYS


char GetVKey(char *key) {
   for (int x=0;x<VK_COUNT;x++) {
      if (!lstrcmpi(keys[x].szName, key))
         return keys[x].lValue;
   }
   return NULL;
}



BOOL GetSpecialKey(long ch, char *dest) {

   for (int x=0;x<VK_COUNT;x++) {
      if (keys[x].lValue == ch) {
         lstrcpy(dest, keys[x].szName);
         return TRUE;
      }
   }
   return FALSE;
}
