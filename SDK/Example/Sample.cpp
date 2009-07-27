/*
 *  Copyright (C) 2001-2009 Jeff Doozan
 */

#include <windows.h>
#include "Plugin.h"
#include "StrLib.h"
#include "resource.h"


// declare our commands
DECLARE_COMMAND(max_rest);
DECLARE_COMMAND(min);
DECLARE_COMMAND(move);
DECLARE_COMMAND(size);


// declare our functions
BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);



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
  INIT_COMMAND(max_rest, STR(MAXRESTORE), STR(MAXRESTORE_DESC), ICON_MAXRESTORE);
  INIT_COMMAND(min,      STR(MINIMIZE),   STR(MINIMIZE_DESC),   ICON_MINIMIZE);
  INIT_COMMAND(move,     STR(MOVE),       STR(MOVE_DESC),       ICON_MOVE);
  INIT_COMMAND(size,     STR(SIZE),       STR(SIZE_DESC),       ICON_SIZE);

  // return the name of the plugin
  INIT_RETURN( STR(PLUGIN_NAME) );
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



/**************************************************************\
 *                                                            *
 *  SHOW is called when a command is going to be edited.      *
 *  Create and initialize the appropriate dialog box.         *
 *                                                            *
\**************************************************************/

SHOW(max_rest)
{
  // Nothing to show, this command has no configuration

  SHOW_RETURN();
}

SHOW(min)
{
  // Nothing to show, this command has no configuration

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



/*****************************************************************\
 *                                                               *
 *  SAVE is called when the config dialog is about to be closed  *
 *  Save the command parameters using save commands              *
 *  return with SAVE_RETURN()                                    *
 *                                                               *
\*****************************************************************/

SAVE(min)
{
  // nothing to save
  SAVE_RETURN();
}

SAVE(max_rest)
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




/***********************************************************\
 *                                                         *
 *  CLEAR is called when a command associated with this    *
 *  plugin is about to be destroyed.  This is your chance  *
 *  to delete any data you have stored in the command      *
 *                                                         *
\***********************************************************/

CLEAR(max_rest)
{
  // nothing to clean up
  CLEAR_RETURN();
}


CLEAR(min)
{
  // nothing to clean up
  CLEAR_RETURN();
}

CLEAR(move)
{
  CLEAR_RETURN();
}

CLEAR(size)
{
  CLEAR_RETURN();
}



/**************************************************\
 *                                                *
 *  Quit is called before the plugin is unloaded  *
 *  This is the place to do you code cleanup      *
 *                                                *
 *  return with QUIT_RETURN()                     *
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
