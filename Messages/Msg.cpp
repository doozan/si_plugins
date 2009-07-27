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

#include "msg.h"
#include "resource.h"


// declare our commands
DECLARE_COMMAND(send);
DECLARE_COMMAND(post);

// declare our custom functions
DECLARE_SHOW(message);
DECLARE_SAVE(message);
DECLARE_CLEAR(message);


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

  // since sendmessage and postmessage are nearly identical commands,
  // we're going to use the same functions for show, save, and clear
  // each function will still call its own exec function

  // we're using INIT_COMMAND_CUSTOM to override the show, save, and clear commands

  //                    Command     Name           Description         Icon        EXEC    SHOW       SAVE       CLEAR
  //                    -------     ---------      --------------      --------    ----    -------    -------    -------
  INIT_COMMAND_CUSTOM  (send,       STR(SEND),     STR(SEND_DESC),     ICON_MSG,   send,   message,   message,   message);
  INIT_COMMAND_CUSTOM  (post,       STR(SEND),     STR(SEND_DESC),     ICON_MSG,   post,   message,   message,   message);

  INIT_RETURN("Send Window Message");
}




/**************************************************************************\
 *                                                                        *
 *  EXEC functions are called when the command needs to be performed      *
 *                                                                        *
 *  Any return value will be saved and can be retrieved with              *
 *  EXEC_GET_DATA() on any subsequent calls to the EXEC function.         *
 *                                                                        *
\**************************************************************************/

EXEC(send)
{
  // see if we have any saved data
  s_msg *msg = (s_msg *) EXEC_GET_DATA();
  if (!msg)
    // no saved data, create and initialize a new s_msg structure
    msg = InitParams( EXEC_GET_PARAMS() );

  // Find our target and send the message
  HWND hWndTarget = GetTarget(msg, EXEC_GET_HWND() );
  SendMessage(hWndTarget, msg->uMsg, msg->wParam, msg->lParam);

  // save our s_msg pointer
  EXEC_RETURN(msg);
}

EXEC(post)
{
  // see if we have any saved data
  s_msg *msg = (s_msg *) EXEC_GET_DATA();
  if (!msg)
    // no saved data, create and initialize a new s_msg structure
    msg = InitParams( EXEC_GET_PARAMS() );

  // Find our target and send the message
  HWND hWndTarget = GetTarget(msg, EXEC_GET_HWND() );
  PostMessage(hWndTarget, msg->uMsg, msg->wParam, msg->lParam);

  // save our s_msg pointer
  EXEC_RETURN(msg);
}





/**************************************************************\
 *                                                            *
 *  SHOW is called when a command is going to be edited.      *
 *  Create and initialize the appropriate dialog box.         *
 *                                                            *
\**************************************************************/


SHOW(message)
{
  // Create the dialog box inside the containing window
  HWND hWnd = SHOW_DIALOG(DLG_CLASSMSG, DlgProc);

  SET(IDC_WINDOW_FRAME,   STR(WINDOW));
  SET(IDC_RADIO_CURRENT,  STR(WINDOW_CURRENT));
  SET(IDC_RADIO_CLASS,    STR(WINDOW_CLASS));   
  SET(IDC_RADIO_CAPTION,  STR(WINDOW_CAPTION));

  SET(IDC_CHECK_PATTERN,  STR(PATTERN));

  SET(IDC_MESSAGE_FRAME,  STR(MESSAGE));
  SET(IDC_MSG_DESC,       STR(MSG));
  SET(IDC_WPARAM_DESC,    STR(WPARAM));
  SET(IDC_LPARAM_DESC,    STR(LPARAM));


  RestoreItem(IDC_MESSAGE);
  RestoreItem(IDC_WPARAM);
  RestoreItem(IDC_LPARAM);


  CHAR *szMatch = NULL;

  NEWRADIO(3);
  RADIO(IDC_RADIO_CURRENT, "");
  RADIO(IDC_RADIO_CLASS,   "CLASS");
  RADIO(IDC_RADIO_CAPTION, "CAPTION");
  szMatch = RESTORERADIO();

  // we're just matching the current window
  if (!szMatch || !*szMatch)
  {
    EnableWindow(GetDlgItem(hWnd, IDC_CLASS), FALSE);
    EnableWindow(GetDlgItem(hWnd, IDC_CHECK_PATTERN), FALSE);
  }

  RestoreItem(IDC_CLASS);
  RestoreCheck(IDC_CHECK_PATTERN, "PATTERN");



  // Get currentpath\message.ids

  char fileName[MAX_PATH+1];

  GetModuleFileName(ghInstance, fileName, MAX_PATH);
  char *end = fileName + lstrlen(fileName)-1;
  while (end >= fileName && *end != '\\') end--;
  end++;

  lstrcpy(end, "message.ids");
  PopulateCombo(hWnd, IDC_MESSAGE, fileName);

  lstrcpy(end, "wparam.ids");
  PopulateCombo(hWnd, IDC_WPARAM, fileName);

  lstrcpy(end, "lparam.ids");
  PopulateCombo(hWnd, IDC_LPARAM, fileName);


  SHOW_RETURN();
}



/*****************************************************************\
 *                                                               *
 *  SAVE is called when the config dialog is about to be closed  *
 *  Save the command parameters using save commands              *
 *  return with SAVE_RETURN()                                    *
 *                                                               *
\*****************************************************************/

SAVE(message)
{
  SaveItem(IDC_MESSAGE);
  SaveItem(IDC_WPARAM);
  SaveItem(IDC_LPARAM);

  NEWRADIO(3);
  RADIO(IDC_RADIO_CURRENT, "");
  RADIO(IDC_RADIO_CLASS,   "CLASS");
  RADIO(IDC_RADIO_CAPTION, "CAPTION");
  SAVERADIO();

  SaveItem(IDC_CLASS);
  SaveCheck(IDC_CHECK_PATTERN, "PATTERN");

  SAVE_RETURN();
}



/***********************************************************\
 *                                                         *
 *  CLEAR is called when a command associated with this    *
 *  plugin is about to be destroyed.  This is your chance  *
 *  to delete any data you have stored in the command      *
 *                                                         *
\***********************************************************/

CLEAR(message) {

  s_msg *msg = (s_msg *) CLEAR_GET_DATA();

  if (!msg)
    return;

  delete msg->sIdentifyText;
  delete msg;
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





HWND ghWndFound;

BOOL CALLBACK EnumWindowProc(HWND hWnd, LPARAM lParam) {
   char title[512];
   GetWindowText(hWnd, title, 512);   
   if (siFuncs->WildMatch(title, (char *) lParam)) {
      ghWndFound = hWnd;
      return FALSE;
   }
   return TRUE;
}

HWND FindWindowCaption(char *caption, BOOL bPattern) {
   if (!bPattern)
      return FindWindow(NULL, caption);
   else {
      ghWndFound = NULL;
      EnumWindows(EnumWindowProc, (LPARAM) caption);
      return ghWndFound;
   }
}


char *ReadFile(char *fileName) {
   HANDLE hFile;
   DWORD fileLength;
  
   hFile = CreateFile(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
   if (hFile == INVALID_HANDLE_VALUE)
      return NULL;
   else {
      fileLength = GetFileSize(hFile, NULL);
      char *fileData = new char[fileLength + 1];
      ReadFile(hFile, fileData, fileLength, &fileLength, NULL);
      fileData[fileLength] = 0;
      CloseHandle(hFile);
      return fileData;
   }   
}

DWORD GetValueFromFile(char *fileName, char *id) {
   
   char *fileData = ReadFile(fileName);
   
   DWORD val = 0;
   
   char *pos = mstrstri(fileData, id);
   if (pos) {
      pos = GetStringEnd(pos);
      if (pos) {
         pos = SkipWhiteSpace(pos);
         val = matol(pos);
      }
   }

   delete fileData;

   return val;
}



s_msg *InitParams(char *params) {
   
   s_msg *msg = new s_msg;
   msg->bIdentifyPattern = FALSE;
   
   InitRestore(NULL, params);   
   
   // Get currentpath\message.ids
   
   char fileName[MAX_PATH+1];
   
   GetModuleFileName(ghInstance, fileName, MAX_PATH);
   char *end = fileName + lstrlen(fileName)-1;
   while (end >= fileName && *end != '\\') end--;
   end++;
   
   
   // get message      
   char *str = RestoreStr();
   msg->uMsg = matol(str);
   if (!msg->uMsg) {
      lstrcpy(end, "message.ids");
      msg->uMsg = GetValueFromFile(fileName, str);
   }
   
   // get wparam
   str = RestoreStr();
   msg->wParam = matol(str);
   if (!msg->wParam) {
      lstrcpy(end, "wparam.ids");
      msg->wParam = GetValueFromFile(fileName, str);
   }
   
   // get lparam
   str = RestoreStr();
   msg->lParam = matol(str);
   if (!msg->lParam) {
      lstrcpy(end, "lparam.ids");
      msg->lParam = GetValueFromFile(fileName, str);
   }     

   char *type = RestoreStr();
   if (!lstrcmpi(type, "CLASS")) {
      msg->iIdentifyType = IDENTIFY_CLASS;
      msg->sIdentifyText = mstrdup(RestoreStr());
   }
   else if (!lstrcmpi(type, "CAPTION")) {
      msg->iIdentifyType = IDENTIFY_CAPTION;
      msg->sIdentifyText = mstrdup(RestoreStr());
      type = RestoreStr();
      if (!lstrcmpi(type, "PATTERN"))
         msg->bIdentifyPattern = TRUE;
   }
   else if (*type) {
      msg->iIdentifyType = IDENTIFY_CLASS;
      msg->sIdentifyText = mstrdup(type);
   }
   else
      msg->iIdentifyType = IDENTIFY_CURRENT;

   EndRestore();
   
   return msg;
}

HWND GetTarget(s_msg *msg, HWND hwndDefault)
{
  if (msg->iIdentifyType == IDENTIFY_CLASS && *msg->sIdentifyText)
    return FindWindow(msg->sIdentifyText, NULL);
  else if (msg->iIdentifyType == IDENTIFY_CAPTION && *msg->sIdentifyText)
    return FindWindowCaption(msg->sIdentifyText, msg->bIdentifyPattern);
  else
    return hwndDefault;
}

void PopulateCombo(HWND hWnd, int id, char *fileName) {

   char *fileData = ReadFile(fileName);
   if (!fileData)
      return;
   
   int val, cur;
   char *pos = fileData;
   char *string;
   while (pos) {
      string = SkipComments(pos);
      
      pos = GetStringEnd(string);
      if (pos && string) {
         *pos++ = 0;
         pos = SkipWhiteSpace(pos);
        
         val = matol(pos);
         
         cur = SendDlgItemMessage(hWnd, id, CB_ADDSTRING, 0, (LPARAM) string);
         SendDlgItemMessage(hWnd, id, CB_SETITEMDATA, cur, val);
         
         pos = NextLine(pos);
      }         
   }
   
   delete fileData;   

}






BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   switch (uMsg) {
      case WM_COMMAND:
         if (HIWORD(wParam) == BN_CLICKED) {
            if (LOWORD(wParam) == IDC_RADIO_CLASS) {
               EnableWindow(GetDlgItem(hWnd, IDC_CLASS), TRUE);
               EnableWindow(GetDlgItem(hWnd, IDC_CHECK_PATTERN), FALSE);
            }
            else if (LOWORD(wParam) == IDC_RADIO_CAPTION) {
               EnableWindow(GetDlgItem(hWnd, IDC_CLASS), TRUE);
               EnableWindow(GetDlgItem(hWnd, IDC_CHECK_PATTERN), TRUE);
            }
            else if (LOWORD(wParam) == IDC_RADIO_CURRENT) {
               EnableWindow(GetDlgItem(hWnd, IDC_CLASS), FALSE);
               EnableWindow(GetDlgItem(hWnd, IDC_CHECK_PATTERN), FALSE);
            }
         }
         break;
//		case WM_INITDIALOG:
//         break;
      case WM_CLOSE:
			DestroyWindow(hWnd);
			break;
		default:
			return FALSE;
   }
   return TRUE;
}
