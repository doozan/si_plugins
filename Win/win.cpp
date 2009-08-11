/*
Copyright (c) 2001-2009 Jeff Doozan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
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


#include <windows.h>
#include "Plugin.h"
#include "StrLib.h"
#include "resource.h"
#include <commctrl.h>



BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HWND FindTaskBar();

int GetCurrentItem(HWND hWnd);
int GetItemCount(HWND hWnd);

BOOL CALLBACK EnumWindowProc(HWND hWnd, LPARAM lParam);
HWND FindWindowSpecial(char *className, char *caption, BOOL bPattern);
void ForceForegroundWindow(HWND hWnd);




// declare our commands
DECLARE_COMMAND(max_rest);
DECLARE_COMMAND(min);
DECLARE_COMMAND(move);
DECLARE_COMMAND(size);
DECLARE_COMMAND(next);
DECLARE_COMMAND(prev);
DECLARE_COMMAND(activate);
DECLARE_COMMAND(wait);

DECLARE_SHOW(none);
DECLARE_SAVE(none);
DECLARE_CLEAR(none);


BOOL gbWinOld     = FALSE;   // Win95/NT4 detection needed for Next/Prev Task
BOOL gbWinTooNew  = FALSE;   // WinXP or newer detection needed for Next/Prev Task



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

  // check for winXP or newer; task next/prev won't work with newer taskbars
  OSVERSIONINFO osvi;

  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

  if (GetVersionEx ((OSVERSIONINFO *) &osvi))
  {

    if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
      if ( (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1) || ( osvi.dwMajorVersion > 5) )
        gbWinTooNew = TRUE;
      // NT3 and NT4
      else
        gbWinOld = TRUE;
    }

    else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    {
      // less than win98 (4.10)
      if ( ( ((osvi.dwMajorVersion == 4) && (osvi.dwMinorVersion < 10) ) || (osvi.dwMajorVersion < 4) ) )
        gbWinOld = TRUE;
    }
  }  


  // initialize the commands (command, command_name, command_description, command_icon)
  INIT_COMMAND_CUSTOM   (max_rest, STR(MAXRESTORE), STR(MAXRESTORE_DESC), ICON_MAXRESTORE,    max_rest,      none,      none,      none);
  INIT_COMMAND_CUSTOM   (min,      STR(MINIMIZE),   STR(MINIMIZE_DESC),   ICON_MINIMIZE,      min,           none,      none,      none);
  INIT_COMMAND_CUSTOM   (move,     STR(MOVE),       STR(MOVE_DESC),       ICON_MOVE,          move,          move,      move,      none);
  INIT_COMMAND_CUSTOM   (size,     STR(SIZE),       STR(SIZE_DESC),       ICON_SIZE,          size,          size,      size,      none);
  INIT_COMMAND_CUSTOM   (activate, STR(ACTIVATE),   STR(ACTIVATE_DESC),   ICON_ACTIVATE,      activate,      activate,  activate,  none);

  if (!gbWinTooNew)
  {
    INIT_COMMAND_CUSTOM (next,     STR(TASKNEXT),   STR(TASKNEXT_DESC),   ICON_TASKNEXT,      next,          none,      none,      none);
    INIT_COMMAND_CUSTOM (prev,     STR(TASKPREV),   STR(TASKPREV_DESC),   ICON_TASKPREV,      prev,          none,      none,      none);
  }
    
  INIT_RETURN("Window management");
}




/**************************************************************************\
 *                                                                        *
 *  EXEC functions are called when the command needs to be performed      *
 *                                                                        *
 *  Any return value will be saved, and passed back in lpAction->lpData   *
 *  to any subsequent calls to the EXEC function.                         *
 *                                                                        *
\**************************************************************************/

EXEC(max_rest)
{
  // execute the command
  WINDOWPLACEMENT wp;
  wp.length = sizeof (WINDOWPLACEMENT);

  HWND hWnd = EXEC_GET_HWND();

  GetWindowPlacement(hWnd, &wp);
  if (wp.showCmd == SW_SHOWMAXIMIZED)
    ShowWindow(hWnd, SW_RESTORE);
  else
    ShowWindow(hWnd, SW_MAXIMIZE);


  // if we had created a data structure or string or any other information that we wanted to save
  // and have available for the next time this command runs, we could return a pointer to the data
  // instead, just return NULL
  EXEC_RETURN(NULL);
}

EXEC(min)
{
  // execute the command
  ShowWindow(EXEC_GET_HWND(), SW_MINIMIZE);

  // if we had created a data structure or string or any other information that we wanted to save
  // and have available for the next time this command runs, we could return a pointer to the data
  // instead, just return NULL
  EXEC_RETURN(NULL);
}

EXEC(move)
{
  INT x = RestoreInt();
  INT y = RestoreInt();

  // execute the command
  SetWindowPos(EXEC_GET_HWND(), 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

  // if we had created a data structure or string or any other information that we wanted to save
  // and have available for the next time this command runs, we could return a pointer to the data
  // instead, just return NULL
  EXEC_RETURN(NULL);
}

EXEC(size)
{      
  INT iWidth = RestoreInt();
  INT iHeight = RestoreInt();

  // execute the command  
  SetWindowPos(EXEC_GET_HWND(), 0, 0, 0, iWidth, iHeight, SWP_NOMOVE | SWP_NOZORDER);

  // if we had created a data structure or string or any other information that we wanted to save
  // and have available for the next time this command runs, we could return a pointer to the data
  // instead, just return NULL
  EXEC_RETURN(NULL);
}


#define KEYDOWN(KEY) \
   keybd_event(VK_ ##KEY, NULL, NULL, NULL); \
   Sleep(1);

#define KEYUP(KEY) \
   keybd_event(VK_ ##KEY, NULL, KEYEVENTF_KEYUP, NULL); \
   Sleep(1);

#define KEYPRESS(KEY) \
   keybd_event(VK_ ##KEY, NULL, NULL, NULL); \
   Sleep(1); \
   keybd_event(VK_ ##KEY, NULL, KEYEVENTF_KEYUP, NULL); \
   Sleep(1);

EXEC(next)
{
  HWND hwndTask = FindTaskBar();
  
  if (hwndTask)
  {    
    int tabCur  = GetCurrentItem(hwndTask);
    
    KEYDOWN(LWIN);
    for (int x = 0; x <= tabCur+1; x++)
    {
      KEYPRESS(TAB);
    }

    KEYUP(LWIN);
    
    Sleep(10);

    KEYPRESS(SPACE); 
  }
  
  EXEC_RETURN(NULL);
}



EXEC(prev)
{
  HWND hwndTask = FindTaskBar();

  if (hwndTask) {
    
    int tabCur = GetCurrentItem(hwndTask);
    
    if (tabCur == -1 || tabCur == 0)
      tabCur = GetItemCount(hwndTask);
    
    KEYDOWN(LWIN);
    for (int x = 0; x <= tabCur-1; x++) {
      KEYPRESS(TAB);
    }
    KEYUP(LWIN);
    
    Sleep(10);

    KEYPRESS(SPACE);
    
  }

  EXEC_RETURN(NULL);
}



EXEC(activate)
{

  char *szClass     = mstrdup(RestoreStr());
  char *szCaption   = mstrdup(RestoreStr());
  char *szPattern   = RestoreStr();
  BOOL bPattern     = !lstrcmpi(szPattern, "PATTERN");
  int  iWait        = RestoreInt();

  if (!*szCaption) {
    delete szCaption;
    szCaption = NULL;
  }

  if (!*szClass) {
    delete szClass;
    szClass = NULL;
  }
  
  if (iWait)
  {
    int loop = iWait/10;  // each loop iteration takes 10ms
    
    if (loop > 12000)
      loop = 12000;    // 120 seconds should be much longer than anyone is willing to wait
    
    while (loop && !FindWindowSpecial(szClass, szCaption, bPattern))
    {
      Sleep(10);
      loop--;
    }
  }
  
  

  HWND hWndActivate = FindWindowSpecial(szClass, szCaption, bPattern);
  if (hWndActivate)
  {    
    ForceForegroundWindow(hWndActivate);
    siFuncs->SetTarget(hWndActivate);
  }

  delete(szCaption);
  delete(szClass);

  EXEC_RETURN(NULL);
}









/**************************************************************\
 *                                                            *
 *  SHOW is called when a command is going to be edited.      *
 *  Create and initialize the appropriate dialog box.         *
 *                                                            *
\**************************************************************/

SHOW(none)
{
  SHOW_RETURN();
}


SHOW(move)
{
  // Show the dialog box
  SHOW_DIALOG(DLG_PARAMS, DlgProc);

  // Set the text for all of the dialog labels
  SET(IDC_COMMAND,   STR(MOVE_DESC));
  SET(IDC_VAL1_DESC, STR(MOVE_XPOS));
  SET(IDC_VAL2_DESC, STR(MOVE_YPOS));

  // Restore the save parameters to the dialog
  RestoreItem(IDC_VAL1);  // x
  RestoreItem(IDC_VAL2);  // y

  SHOW_RETURN();
}

SHOW(size)
{
  // Show the dialog box
  SHOW_DIALOG(DLG_PARAMS, DlgProc);

  // Set the text for all of the dialog labels
  SET(IDC_COMMAND,   STR(SIZE_DESC));
  SET(IDC_VAL1_DESC, STR(SIZE_WIDTH));
  SET(IDC_VAL2_DESC, STR(SIZE_HEIGHT));

  RestoreItem(IDC_VAL1);  // width
  RestoreItem(IDC_VAL2);  // height

  SHOW_RETURN();
}



SHOW(activate)
{
  // Create the dialog box inside the containing window
  HWND hWnd = SHOW_DIALOG(DLG_ACTIVATE, DlgProc);

  SET(IDC_CLASS_DESC, STR(CLASS));
  SET(IDC_CAPTION_DESC, STR(CAPTION));
  SET(IDC_CHECK_PATTERN, STR(PATTERN));
  SET(IDC_CHECK_WAIT, STR(CHECK_WAIT));
  SET(IDC_TIMEOUT_DESC, STR(TIME));

  RestoreItem(IDC_CLASS);
  RestoreItem(IDC_CAPTION);

  RestoreCheck(IDC_CHECK_PATTERN, "PATTERN");
  
  char *time = RestoreStr();
  if (time && *time) {
    SetDlgItemText(hWnd, IDC_TIME, time);
    CheckDlgButton(hWnd, IDC_CHECK_WAIT, BST_CHECKED);
  }
  else
    EnableWindow(GetDlgItem(hWnd, IDC_TIME), FALSE);

  SHOW_RETURN();
}


/*****************************************************************\
 *                                                               *
 *  SAVE is called when the config dialog is about to be closed  *
 *  Save the command parameters using save commands              *
 *  return with SAVE_RETURN()                                    *
 *                                                               *
\*****************************************************************/

SAVE(none)
{
  // nothing to save
  SAVE_RETURN();
}

SAVE(move)
{
  SaveItem(IDC_VAL1);  // X
  SaveItem(IDC_VAL2);  // Y

  SAVE_RETURN();
}

SAVE(size)
{
  SaveItem(IDC_VAL1); // Width
  SaveItem(IDC_VAL2); // Height

  SAVE_RETURN();
}


SAVE(activate)
{
  SaveItem(IDC_CLASS);
  SaveItem(IDC_CAPTION);
  SaveCheck(IDC_CHECK_PATTERN, "PATTERN");

  if (IsDlgButtonChecked(SAVE_GET_HWND(), IDC_CHECK_WAIT))
    SaveItem(IDC_TIME);

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

CLEAR(none)
{
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
  QUIT_RETURN();
}






/*

   === Win95/NT4 ===
   Shell_TrayWnd
      - MSTaskSwWClass
         - SysTabControl32

   === Win2k ===
   Shell_TrayWnd (multiple instances)
      - ReBarWindow32
         - MSTaskSwWClass
            - SysTabControl32

   === WinXP ===
   Shell_TrayWnd
      - ReBarWindow32
         - MSTaskSwWClass
            -ToolbarWindow32

*/





HWND FindTaskBar() {

  HWND hWndTray = NULL;
    
  do
  {
    hWndTray = FindWindowEx(NULL, hWndTray, "Shell_TrayWnd", NULL);
    
    if (hWndTray)
    {
      
      // Win95/NT4      
      if (gbWinOld)
      {
        HWND task = FindWindowEx(hWndTray, NULL, "MSTaskSwWClass", NULL); 
        if (task)
          return FindWindowEx(task, NULL, "SysTabControl32", NULL);
      }

      // later OSs
      else
      {
        HWND task = FindWindowEx(hWndTray, NULL, "ReBarWindow32", NULL);
        
        if (task)
        {
          task = FindWindowEx(task, NULL, "MSTaskSwWClass", NULL); 
          
          if (task)
          {
            // Win XP
            if (gbWinTooNew)
              return FindWindowEx(task, NULL, "ToolbarWindow32", NULL);
            
            // Win2k
            else
              return FindWindowEx(task, NULL, "SysTabControl32", NULL);
          }                   
        }
      }                  
    }    
  } while (hWndTray);

   return NULL;
}




int GetCurrentItem(HWND hWnd) {
   return SendMessage(hWnd, TCM_GETCURSEL, 0, 0);
}

int GetItemCount(HWND hWnd) {
   return SendMessage(hWnd, TCM_GETITEMCOUNT, 0, 0);
}


struct s_winstrings {
   char *className;
   char *caption;
};

HWND ghWndFound;

BOOL CALLBACK EnumWindowProc(HWND hWnd, LPARAM lParam) {

   s_winstrings *params = (s_winstrings *)lParam;

   char buf[512];
   
   if (params->className && *params->className) {  
      GetClassName(hWnd, buf, 512);
      if (!siFuncs->WildMatch(buf, params->className))
         return TRUE;   // not a match
   }

   if (params->caption && *params->caption) {  
      GetWindowText(hWnd, buf, 512);
      if (!siFuncs->WildMatch(buf, params->caption))
         return TRUE;   // not a match
   }

   ghWndFound = hWnd;
   return FALSE;
}

HWND FindWindowSpecial(char *className, char *caption, BOOL bPattern) {
   
   if (!bPattern)
      return FindWindow(className, caption);

   else {
      ghWndFound = NULL;
      s_winstrings params;
      params.caption = caption;
      params.className = className;
      EnumWindows(EnumWindowProc, (LPARAM) &params);
      return ghWndFound;
   }
}


void ForceForegroundWindow(HWND hWnd) {

#ifndef SPI_GETFOREGROUNDLOCKTIMEOUT

   #define SPI_GETFOREGROUNDLOCKTIMEOUT 2000
   #define SPI_SETFOREGROUNDLOCKTIMEOUT 2001

#endif

   HWND hWndActive = GetForegroundWindow();
   BOOL bAttached = AttachThreadInput( GetWindowThreadProcessId(hWndActive, NULL), GetCurrentThreadId(), TRUE);

   

   // Restore the window if it is minimized
   WINDOWPLACEMENT wp;
   wp.length = sizeof (WINDOWPLACEMENT);
   
   GetWindowPlacement(hWnd, &wp);
   if (wp.showCmd == SW_SHOWMINIMIZED)
      ShowWindow(hWnd, SW_RESTORE);


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







BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   switch (uMsg) {
      
      case WM_COMMAND:
         if (HIWORD(wParam) == BN_CLICKED) {
            if (LOWORD(wParam) == IDC_CHECK_WAIT)
               EnableWindow(GetDlgItem(hWnd, IDC_TIME), IsDlgButtonChecked(hWnd, IDC_CHECK_WAIT));
         }
         break;
      
      case WM_CLOSE:
			DestroyWindow(hWnd);
			break;
		default:
			return FALSE;
   }
   return TRUE;
}
