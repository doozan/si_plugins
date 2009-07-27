/*
Copyright (c) 2001-2009 Jeff Doozan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so.

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


// declare our commands
DECLARE_COMMAND(mycmd);


// declare our functions
BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);



/*************************************************************\
 *                                                           *
 *  INIT() is called when the plugin is loaded               * 
 *  Initialize all of the plugin commands and do any         *
 *  additional initializations your plugin may require       *
 *                                                           *
 *  Return the name of the plugin using INIT_RETURN(name)    *
 *                                                           *
\*************************************************************/

INIT()
{
  // initialize the commands (command, command_name, command_description, command_icon)
  INIT_COMMAND(mycmd, STR(MYCMD), STR(MYCMD_DESC), ICON_MYCMD);

  // return the name of the plugin
  INIT_RETURN( STR(PLUGIN_NAME) );
}




/**************************************************************************\
 *                                                                        *
 *  EXEC functions are called when the command needs to be performed      *
 *                                                                        *
 *  Any return value will be saved with EXEC_RETURN(param) can be         *
 *  retrieved with EXEC_GET_DATA() on any subsequent calls.               *
 *                                                                        *
\**************************************************************************/

EXEC(mycmd)
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



/**************************************************************\
 *                                                            *
 *  SHOW is called when a command is going to be edited.      *
 *  Create and initialize the appropriate dialog box.         *
 *                                                            *
 *  return with SAVE_RETURN()                                 *
 *                                                            *
\**************************************************************/


SHOW(mycmd)
{
  // Show the dialog box
  SHOW_DIALOG(DLG_MYCMD, DlgProc);

  SHOW_RETURN();
}


/*****************************************************************\
 *                                                               *
 *  SAVE is called when the config dialog is about to be closed  *
 *  Save the command parameters using save commands              *
 *                                                               *
 *  return with SAVE_RETURN()                                    *
 *                                                               *
\*****************************************************************/

SAVE(mycmd)
{
  // nothing to save
  SAVE_RETURN();
}



/***********************************************************\
 *                                                         *
 *  CLEAR is called when a command associated with this    *
 *  plugin is about to be destroyed.  This is your chance  *
 *  to delete any data you have stored in the command      *
 *                                                         *
 *  return with CLEAR_RETURN()                             *
 *                                                         *
\***********************************************************/

CLEAR(mycmd)
{
  // nothing to clean up
  CLEAR_RETURN();
}




/**************************************************\
 *                                                *
 *  Quit is called before the plugin is unloaded  *
 *  This is the place to do you code cleanup      *
 *                                                *
 *  return with QUIT_RETURN()                     *
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
