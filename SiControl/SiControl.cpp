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

#define APP_NAME "StrokeIt"

// function definitions
BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

DECLARE_COMMAND(show);
DECLARE_COMMAND(quit);
DECLARE_COMMAND(learn);
DECLARE_COMMAND(disable);
DECLARE_COMMAND(ignore);

DECLARE_SHOW(cmd);
DECLARE_SAVE(cmd);
DECLARE_CLEAR(cmd);

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

  // none of these commands need to show, save, or clear anything so we're
  // we're going to use the same function for each of them and later declare those
  // functions as simple return calls
  // each function will still call its own exec function

  // we're using INIT_COMMAND_CUSTOM to override the show, save, and clear commands

  //                    Command     Name           Description         Icon        EXEC      SHOW    SAVE    CLEAR
  //                    -------     ---------      --------------      --------    -----     -----   -----   -----
  INIT_COMMAND_CUSTOM  (show,       STR(SHOW),     STR(SHOW_DESC),     ICON_SI,    show,     cmd,    cmd,    cmd);
  INIT_COMMAND_CUSTOM  (quit,       STR(QUIT),     STR(QUIT_DESC),     ICON_SI,    quit,     cmd,    cmd,    cmd);
  INIT_COMMAND_CUSTOM  (learn,      STR(LEARN),    STR(LEARN_DESC),    ICON_SI,    learn,    cmd,    cmd,    cmd);
  INIT_COMMAND_CUSTOM  (disable,    STR(DISABLE),  STR(DISABLE_DESC),  ICON_SI,    disable,  cmd,    cmd,    cmd);
  INIT_COMMAND_CUSTOM  (ignore,     STR(IGNORE),   STR(IGNORE_DESC),   ICON_SI,    ignore,   cmd,    cmd,    cmd);
   
  INIT_RETURN(APP_NAME "Control");
}


/**************************************************************************\
 *                                                                        *
 *  EXEC functions are called when the command needs to be performed      *
 *                                                                        *
 *  Any return value will be saved and can be retrieved with              *
 *  EXEC_GET_DATA() on any subsequent calls to the EXEC function.         *
 *                                                                        *
\**************************************************************************/

EXEC(show)
{
  HWND hWnd = FindWindow(APP_NAME, NULL);
  SendMessage(hWnd, SI_SHOW, NULL, NULL);
  EXEC_RETURN(NULL);
}
EXEC(learn)
{
  HWND hWnd = FindWindow(APP_NAME, NULL);
  SendMessage(hWnd, SI_LEARN, NULL, NULL);
  EXEC_RETURN(NULL);
}
EXEC(disable)
{
  HWND hWnd = FindWindow(APP_NAME, NULL);
  SendMessage(hWnd, SI_SETSTATE, STATE_DISABLED, NULL);
  EXEC_RETURN(NULL);
}
EXEC(quit)
{
  HWND hWnd = FindWindow(APP_NAME, NULL);
  SendMessage(hWnd, SI_SHUTDOWN, NULL, NULL);
  EXEC_RETURN(NULL);
}
EXEC(ignore)
{
  HWND hWnd = FindWindow(APP_NAME, NULL);
  SendMessage(hWnd, SI_IGNORENEXT, NULL, NULL);
  EXEC_RETURN(NULL);
}






/**************************************************************\
 *                                                            *
 *  SHOW is called when a command is going to be edited.      *
 *  Create and initialize the appropriate dialog box.         *
 *                                                            *
\**************************************************************/

SHOW(cmd)
{
  // we don't need to show anything
  SHOW_RETURN();
}


/*****************************************************************\
 *                                                               *
 *  SAVE is called when the config dialog is about to be closed  *
 *  Save the command parameters using save commands              *
 *  return with SAVE_RETURN()                                    *
 *                                                               *
\*****************************************************************/


SAVE(cmd)
{
  SAVE_RETURN();
}


/***********************************************************\
 *                                                         *
 *  CLEAR is called when a command associated with this    *
 *  plugin is about to be destroyed.  This is your chance  *
 *  to delete any data you have stored in the command      *
 *                                                         *
\***********************************************************/

CLEAR(cmd)
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







BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   switch (uMsg) {
      case WM_CLOSE:
			DestroyWindow(hWnd);
			break;
		default:
			return FALSE;
   }
   return TRUE;
}
