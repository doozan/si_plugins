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

// function definitions
BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
char *GetRegKey(HKEY type, char *parentKey, char *keyName);
char *GetHandler(char *szKey, char *szDefaultViewer);
void StripArgs(char *szCmdLine);
char *StrReplace(char *szIn, char *szMatch, char *szReplace);
void Run(char *szCmd, char *szArgs, char *szDir, char *szShow);

// declare our commands
DECLARE_COMMAND(run);
DECLARE_COMMAND(browser);
DECLARE_COMMAND(mail);
DECLARE_COMMAND(newmail);
DECLARE_COMMAND(web);

// global variable
BOOL bVistaOrNewer = FALSE;


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
  // initialize the commands (command, command_name, command_description, command_icon)

  INIT_COMMAND(run,     STR(RUN),     STR(RUN_DESC),     ICON_RUN);
  INIT_COMMAND(web,     STR(WEB),     STR(WEB_DESC),     ICON_WEB);
  INIT_COMMAND(browser, STR(BROWSER), STR(BROWSER_DESC), ICON_WEB);
  INIT_COMMAND(mail,    STR(MAIL),    STR(MAIL_DESC),    ICON_MAIL);
  INIT_COMMAND(newmail, STR(NEWMAIL), STR(NEWMAIL_DESC), ICON_MAIL);



  OSVERSIONINFO osvi;
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  if (GetVersionEx ((OSVERSIONINFO *) &osvi))
  {
    if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.dwMajorVersion >= 5)
    {
      bVistaOrNewer = TRUE;
    }
  }  

  // return the plugin description
  INIT_RETURN("Execute Programs");
}



/**************************************************************************\
 *                                                                        *
 *  EXEC functions are called when the command needs to be performed      *
 *                                                                        *
 *  Any return value will be saved and can be retrieved with              *
 *  EXEC_GET_DATA() on any subsequent calls to the EXEC function.         *
 *                                                                        *
\**************************************************************************/

EXEC(browser)
{
  char *szFile = GetHandler("http", "IE.HTTP");
  StripArgs(szFile);

  if (szFile)
  {
    Run(szFile, NULL, NULL, NULL);
    delete szFile;
  }

  EXEC_RETURN(NULL);
}

EXEC(mail)
{
  char *szFile = GetHandler("mailto", "WindowsMail.Url.mailto");
  StripArgs(szFile);

  if (szFile)
  {
    Run(szFile, NULL, NULL, NULL);
    delete szFile;
  }

  EXEC_RETURN(NULL);
}

EXEC(newmail)
{
  char *szFile = GetHandler("mailto", "WindowsMail.Url.mailto");

  if (szFile)
  {
    char *szTarget = RestoreStr();

    // find where the arguments start
    char *pArgs = NULL;
    if (szFile[0] == '"')
    {
      pArgs = mstrchr(szFile+1, '"');
      pArgs++;
    }
    else
      pArgs = mstrchr(szFile+1, ' ');

    if (pArgs){
      *pArgs = '\0';  // szFile now has no arguments
      pArgs++;
    }

    // replace %1 in the args with the target address
    char *szCmdLine = StrReplace(pArgs, "%1", szTarget);

    Run(szFile, szCmdLine, NULL, NULL);
    delete szFile;
    delete szCmdLine;
  }

  EXEC_RETURN(NULL);
}

EXEC(web)
{
  char *szFile = GetHandler("http", "IE.HTTP");

  if (szFile)
  {
    char *szTarget = RestoreStr();

    // find where the arguments start
    char *pArgs = NULL;
    if (szFile[0] == '"')
    {
      pArgs = mstrchr(szFile+1, '"');
      pArgs++;
    }
    else
      pArgs = mstrchr(szFile+1, ' ');

    if (pArgs){
      *pArgs = '\0';  // szFile now has no arguments
      pArgs++;
    }

    // replace %1 in the args with the target address
    char *szCmdLine = StrReplace(pArgs, "%1", szTarget);

    Run(szFile, szCmdLine, NULL, NULL);
    delete szFile;
    delete szCmdLine;
  }

  EXEC_RETURN(NULL);
}

EXEC(run)
{
  // strings returned by RestoreStr are only valid until the next
  // call to RestoreStr, so we have to make a copy of each string

  char *szFile  = mstrdup(RestoreStr());
  char *szDir   = mstrdup(RestoreStr());
  char *szArgs  = mstrdup(RestoreStr());
  char *szState = mstrdup(RestoreStr());

  Run(szFile, szArgs, szDir, szState);

  delete szFile;
  delete szDir;
  delete szArgs;
  delete szState;

  EXEC_RETURN(NULL);
}




/**************************************************************\
 *                                                            *
 *  SHOW is called when a command is going to be edited.      *
 *  Create and initialize the appropriate dialog box.         *
 *                                                            *
\**************************************************************/

SHOW(run) {

  // Show the dialog
  SHOW_DIALOG(DLG_RUN, DlgProc);

  // initialize the labels
  SET(IDC_FILE_DESC, STR(FILE));
  SET(IDC_START_DESC, STR(START));
  SET(IDC_ARGS_DESC, STR(ARGS));
  SET(IDC_BROWSE, STR(BROWSE));
  SET(IDC_RUN_DESC, STR(RUNDESC));

  COMBO(IDC_COMBO, STR(MAXIMIZED), "max");
  COMBO(IDC_COMBO, STR(MINIMIZED), "min");
  COMBO(IDC_COMBO, STR(NORMAL), "");


  // restore the data
  RestoreItem(IDC_FILE);
  RestoreItem(IDC_START);
  RestoreItem(IDC_ARGS);
  RestoreCombo(IDC_COMBO);

  SHOW_RETURN();
}

SHOW(browser)
{
  SHOW_RETURN();
}

SHOW(mail)
{
  SHOW_RETURN();
}

SHOW(newmail)
{
  // Show the dialog
  SHOW_DIALOG(DLG_RUN, DlgProc);

  // we're re-using the "run" dialog, so hide the controls we don't need
  HIDE_CONTROL(IDC_START_DESC);
  HIDE_CONTROL(IDC_START_DESC);
  HIDE_CONTROL(IDC_ARGS_DESC);
  HIDE_CONTROL(IDC_BROWSE);
  HIDE_CONTROL(IDC_START);
  HIDE_CONTROL(IDC_ARGS);
  HIDE_CONTROL(IDC_BORDER);
  HIDE_CONTROL(IDC_RUN_DESC);
  HIDE_CONTROL(IDC_COMBO);

  SET(IDC_FILE_DESC, STR(ADDRESS));
  
  char *szFile = RestoreStr();
  if (szFile && *szFile)
    szFile += lstrlen("mailto:");
  SET(IDC_FILE, szFile);

  SHOW_RETURN();
}

SHOW(web)
{
  // Show the dialog
  SHOW_DIALOG(DLG_RUN, DlgProc);

  // we're re-using the "run" dialog, so hide the controls we don't need
  HIDE_CONTROL(IDC_START_DESC);
  HIDE_CONTROL(IDC_START_DESC);
  HIDE_CONTROL(IDC_ARGS_DESC);
  HIDE_CONTROL(IDC_BROWSE);
  HIDE_CONTROL(IDC_START);
  HIDE_CONTROL(IDC_ARGS);
  HIDE_CONTROL(IDC_BORDER);
  HIDE_CONTROL(IDC_RUN_DESC);
  HIDE_CONTROL(IDC_COMBO);

  SET(IDC_FILE_DESC, STR(WEBSITE));
  RestoreItem(IDC_FILE);

  SHOW_RETURN();
}


/*****************************************************************\
 *                                                               *
 *  SAVE is called when the config dialog is about to be closed  *
 *  Save the command parameters using save commands              *
 *  return with SAVE_RETURN()                                    *
 *                                                               *
\*****************************************************************/


SAVE(newmail)
{
  char szMailTo[512];
  lstrcpy(szMailTo, "mailto:");
  GetDlgItemText(hWnd, IDC_FILE, szMailTo+lstrlen(szMailTo), 500);

  SaveStr(szMailTo);

  SAVE_RETURN();
}

SAVE(web)
{
  SaveItem(IDC_FILE);
  SAVE_RETURN()
}

SAVE(mail)
{
  SAVE_RETURN();
}

SAVE(browser)
{
  SAVE_RETURN();
}

SAVE(run)
{
  SaveItem(IDC_FILE);
  SaveItem(IDC_START);
  SaveItem(IDC_ARGS);
  SaveCombo(IDC_COMBO);

  SAVE_RETURN();
}




/***********************************************************\
 *                                                         *
 *  CLEAR is called when a command associated with this    *
 *  plugin is about to be destroyed.  This is your chance  *
 *  to delete any data you have stored in the command      *
 *                                                         *
\***********************************************************/

CLEAR(run)
{
  CLEAR_RETURN();
}

CLEAR(browser)
{
  CLEAR_RETURN();
}

CLEAR(web)
{
  CLEAR_RETURN();
}

CLEAR(mail)
{
  CLEAR_RETURN();
}

CLEAR(newmail)
{
  CLEAR_RETURN();
}




/**************************************************\
 *                                                *
 *  Quit is called before the plugin is unloaded  *
 *  This is the place to do your code cleanup     *
 *                                                *
\**************************************************/

QUIT() {
  QUIT_RETURN();
}










char *GetRegKey(HKEY type, char *parentKey, char *keyName)
{

	HKEY	regkey;
	int   size;
  char *value = NULL;
  DWORD dwType = REG_EXPAND_SZ;

  if( RegOpenKeyEx (type, parentKey, NULL, KEY_READ, &regkey) == ERROR_SUCCESS  )
  {
    if(RegQueryValueEx(regkey, keyName, NULL, NULL, NULL, (LPDWORD) &size) == ERROR_SUCCESS)
    {
      if (size > 0)
      {
        value = new char[size+1];
        RegQueryValueEx(regkey, keyName, NULL, NULL, (LPBYTE) value, (LPDWORD) &size);
      }
    }
    RegCloseKey(regkey);
  }
  return value;
}


// replace the first occurrance of szMatch with szReplace
char *StrReplace(char *szIn, char *szMatch, char *szReplace)
{
  if (!szIn || !*szIn || !szMatch || !*szMatch || !szReplace || !*szReplace)
    return NULL;

  char *szOut = new char[ lstrlen(szIn) + lstrlen(szReplace) ];
  char *pos = mstrstr(szIn, szMatch);

  // copy the part of the string before tha match
  mmemcpy(szOut, szIn, pos-szIn);
  szOut[pos-szIn] = '\0';

  // copy the replacement text
  lstrcat(szOut, szReplace);

  // copy the string after the match
  pos += lstrlen(szMatch);
  if (*pos)
    lstrcat(szOut, pos);


  return szOut;
}


void StripArgs(char *szCmdLine)
{
  if (!szCmdLine || !*szCmdLine)
    return;

  // if the command is quoted, find the closing quote
  // and remove everything afterwards
  if (szCmdLine[0] == '\"')
  {
    char *end = mstrchr(szCmdLine+1, '\"');
    if (end)
      *(end+1) = 0;
  }

  // otherwise just remove everything after the first space
  else
  {
    char *end = mstrchr(szCmdLine, ' ');
    if (end)
      *end = 0;
  }
}


char *GetHandler(char *szProtocol, char *szDefaultViewer)
{
  char szKey[MAX_PATH];
  char *szCmd = NULL;
  char *szViewer = NULL;

  if (bVistaOrNewer)
  {
    lstrcpy(szKey, "Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\");
    lstrcat(szKey, szProtocol);
    lstrcat(szKey, "\\UserChoice");

    szViewer = GetRegKey(HKEY_CURRENT_USER, szKey, "progid");

    if ( (!szViewer || !*szViewer) && (szDefaultViewer && *szDefaultViewer) )
      szViewer = mstrdup(szDefaultViewer);

    lstrcpy(szKey, szViewer);
    lstrcat(szKey, "\\shell\\open\\command");

    szCmd = GetRegKey(HKEY_CLASSES_ROOT, szKey, "");
    delete szViewer;
  }

  // older than vista, or no default viewer found
  if (!szCmd)
  {
    lstrcpy(szKey, szProtocol);
    lstrcat(szKey, "\\shell\\open\\command");

    szCmd = GetRegKey(HKEY_CLASSES_ROOT, szKey, "");
  }

  return szCmd;
}



void Run(char *szFile, char *szArgs, char *szDir, char *szState)
{

  if (!szFile || !*szFile)
    return;

  char szExpandedFile[MAX_PATH+1];
  *szExpandedFile = 0;

  char szExpandedDir[MAX_PATH+1];
  *szExpandedDir = 0;

  char szExpandedArgs[MAX_PATH+1];
  *szExpandedArgs = 0;

  // if szFile is quoted, remove the quotes
  if (*szFile == '"')
  {
    szFile = szFile + 1;
    *(szFile + lstrlen(szFile) - 1) = '\0';
  }


  // put the filename in quotes
  szExpandedFile[0] = '"';

  ExpandEnvironmentStrings(szFile, szExpandedFile+1, MAX_PATH);
  int len = lstrlen(szExpandedFile);
  szExpandedFile[ len ] = '"';
  szExpandedFile[ len+1 ] = '\0';


  if (szDir && *szDir)
    ExpandEnvironmentStrings(szDir, szExpandedDir, MAX_PATH);

  if (szArgs && *szArgs)
    ExpandEnvironmentStrings(szArgs, szExpandedArgs, MAX_PATH);

  INT iShow = SW_SHOWNORMAL;
  if (szState) {
    if (!lstrcmpi(szState, "max"))
      iShow = SW_MAXIMIZE;
    else if (!lstrcmpi(szState, "min"))
      iShow = SW_MINIMIZE;
  }
  

  STARTUPINFO si = {0};
  PROCESS_INFORMATION pi = {0};

  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = iShow;

  char *szCmdLine = new char[ lstrlen(szExpandedFile) + lstrlen(szExpandedArgs) + 4];
  lstrcpy(szCmdLine, szExpandedFile);
  lstrcat(szCmdLine, " ");
  lstrcat(szCmdLine, szExpandedArgs);


  char *szCmd = szExpandedFile;
  // if cmd is enclosed in quotes, strip them off
  if (szCmd[0] == '\"')
  {
    szCmd++;
    szCmd[lstrlen(szCmd)-1] = '\0';
  }

  if (*szExpandedDir)
    CreateProcess(szCmd, szCmdLine, NULL, NULL, FALSE, 0, NULL, szExpandedDir, &si, &pi);
  else
    CreateProcess(szCmd, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

  delete szCmdLine;

}







BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   switch (uMsg) {
   case WM_COMMAND:
      switch (LOWORD(wParam)) {
      case IDC_BROWSE:
         char *buf = new char[MAX_PATH+1];
         *buf = 0;
         
         OPENFILENAME ofn = {0};

         ofn.lStructSize = sizeof(OPENFILENAME);
         ofn.hInstance = ghInstance;
         ofn.hwndOwner = hWnd;
         
         // "Executable Files\0*.exe;*.com;*.bat\0All Files\0*.*\0";
         char *filter = new char[256];
         char *end = filter;

         lstrcpy(end, STR(EXE_FILES));
         end += lstrlen(end) + 1;
         lstrcpy(end, "*.exe;*.com;*.bat");
         end += lstrlen(end) + 1;

         lstrcpy(end, STR(ALL_FILES));
         end += lstrlen(end) + 1;
         lstrcpy(end, "*.*");
         end += lstrlen(end) + 1;

         *end = 0;

         
         ofn.lpstrFilter = filter;
         ofn.lpstrFile = buf;
         ofn.nMaxFile = MAX_PATH;
         ofn.Flags = OFN_PATHMUSTEXIST | 	OFN_LONGNAMES | OFN_EXPLORER;
         
         if (GetOpenFileName(&ofn))
           SetDlgItemText(hWnd, IDC_FILE, buf);

         delete filter;
         delete buf;
         break;
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

