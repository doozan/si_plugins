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


#define PRO_PLUGIN  // we're using features only availble in the pro version

#include "mouse.h"
#include "resource.h"


// declare our commands
DECLARE_COMMAND(click);
DECLARE_COMMAND(move);

// shared show, save, clear functions
DECLARE_SHOW(mouse);
DECLARE_SAVE(mouse);
DECLARE_CLEAR(mouse);



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

  INIT_COMMAND_CUSTOM(click,     STR(CMD_CLICK),     STR(CLICK_DESC),    ICON_MOUSE,  click,  mouse,  mouse,  mouse);
  INIT_COMMAND_CUSTOM(move,      STR(CMD_MOVE),      STR(MOVE_DESC),     ICON_MOUSE,  move,   mouse,  mouse,  mouse);
   
  INIT_RETURN("Mouse Controls");
}




/**************************************************************************\
 *                                                                        *
 *  EXEC functions are called when the command needs to be performed      *
 *                                                                        *
 *  Any return value will be saved and can be retrieved with              *
 *  EXEC_GET_DATA() on any subsequent calls to the EXEC function.         *
 *                                                                        *
\**************************************************************************/


EXEC(click)
{
  s_params *pData = (s_params *) EXEC_GET_DATA();
  if (!pData)
  {
    pData = new s_params;

    char *button = RestoreStr();
    if (!lstrcmpi(button, "left"))
      pData->button = BUTTON_LEFT;
    else if (!lstrcmpi(button, "middle"))
      pData->button = BUTTON_MIDDLE;
    else
      pData->button = BUTTON_RIGHT;
    
    char *action = RestoreStr();
    if (!lstrcmpi(action, "down"))
      pData->action = ACTION_DOWN;
    else if (!lstrcmpi(action, "up"))
      pData->action = ACTION_UP;
    else if (!lstrcmpi(action, "dblclick"))
      pData->action = ACTION_DBLCLICK;
    else
      pData->action = ACTION_CLICK;

    char *pos = RestoreStr();
    if (!lstrcmpi(pos, "end"))
      pData->pos = POS_END;
    else if (!lstrcmpi(pos, "pos"))
      pData->pos = POS_SCREEN;
    else
      pData->pos = POS_START;
    
    pData->x = RestoreInt();
    pData->y = RestoreInt();
    
    char *relative = RestoreStr();
    if (pData->pos == POS_SCREEN && !lstrcmpi(relative, "relative"))
        pData->pos = POS_WINDOW;
  }

  RunMouseCommand(EXEC_GET_HWND(), pData, EXEC_GET_GESTURE());

  EXEC_RETURN(pData);
}

EXEC(move)
{
  s_params *pData = (s_params *) EXEC_GET_DATA();
  if (!pData)
  {
    pData = new s_params;
    
    pData->button = NULL;
    pData->action = ACTION_MOVE;

    char *pos = RestoreStr();
    if (!lstrcmpi(pos, "end"))
      pData->pos = POS_END;
    else if (!lstrcmpi(pos, "pos"))
      pData->pos = POS_SCREEN;
    else
      pData->pos = POS_START;
    
    pData->x = RestoreInt();
    pData->y = RestoreInt();
    
    char *relative = RestoreStr();
    if (pData->pos == POS_SCREEN && !lstrcmpi(relative, "relative"))
      pData->pos = POS_WINDOW;
  }

  RunMouseCommand(EXEC_GET_HWND(), pData, EXEC_GET_GESTURE());

  EXEC_RETURN(pData);
}






/**************************************************************\
 *                                                            *
 *  SHOW is called when a command is going to be edited.      *
 *  Create and initialize the appropriate dialog box.         *
 *                                                            *
\**************************************************************/

SHOW(mouse)
{

   // Create the dialog box inside the containing window
   HWND hWnd = SHOW_DIALOG(DLG_MOUSE, DlgProc);

   SET(IDC_LOCATION, STR(LOCATION));
   SET(IDC_X, STR(X));
   SET(IDC_Y, STR(Y));

   SET(IDC_RADIO_START, STR(START));
   SET(IDC_RADIO_END, STR(END));
   SET(IDC_RADIO_EXACT, STR(EXACT));

   SET(IDC_CHECK_RELATIVE, STR(RELATIVE));

   if (SHOW_GET_ID() == GET_CMD_ID(move))
   {
     SET(IDC_COMMAND, STR(CLICK_DESC));

     HIDE_CONTROL(IDC_BUTTON);
     HIDE_CONTROL(IDC_ACTION);
     HIDE_CONTROL(IDC_COMBO_BUTTON);
     HIDE_CONTROL(IDC_COMBO_ACTION);

   }
   else
   {
     SET(IDC_COMMAND, STR(MOVE_DESC));

     SET(IDC_BUTTON, STR(BUTTON));
     SET(IDC_ACTION, STR(ACTION)); 

     COMBO(IDC_COMBO_BUTTON, STR(RIGHT), "right");
     COMBO(IDC_COMBO_BUTTON, STR(LEFT), "left");
     COMBO(IDC_COMBO_BUTTON, STR(MIDDLE), "middle");
     RestoreCombo(IDC_COMBO_BUTTON);
       
     COMBO(IDC_COMBO_CLICK, STR(DOWN), "down");
     COMBO(IDC_COMBO_CLICK, STR(UP), "up");
     COMBO(IDC_COMBO_CLICK, STR(CLICK), "click");
     COMBO(IDC_COMBO_CLICK, STR(DBLCLICK), "dblclick");
     RestoreCombo(IDC_COMBO_CLICK);
   }

   NEWRADIO(3);
   RADIO(IDC_RADIO_START, "start");
   RADIO(IDC_RADIO_END, "end");
   RADIO(IDC_RADIO_EXACT, "pos");
   RESTORERADIO();

/*
   BOOL bState = IsDlgButtonChecked(hWnd, IDC_RADIO_EXACT);
   EnableWindow(GetDlgItem(hWnd, IDC_EDIT_X), bState);
   EnableWindow(GetDlgItem(hWnd, IDC_EDIT_Y), bState);
   EnableWindow(GetDlgItem(hWnd, IDC_CHECK_RELATIVE), bState);
*/

   RestoreItem(IDC_EDIT_X);
   RestoreItem(IDC_EDIT_Y);

   RestoreCheck(IDC_CHECK_RELATIVE, "relative");

   SHOW_RETURN();
}




/*****************************************************************\
 *                                                               *
 *  SAVE is called when the config dialog is about to be closed  *
 *  Save the command parameters using save commands              *
 *  return with SAVE_RETURN()                                    *
 *                                                               *
\*****************************************************************/

SAVE(mouse)
{
  if ( SAVE_GET_ID() == GET_CMD_ID(click) )
  {
    SaveCombo(IDC_COMBO_BUTTON);
    SaveCombo(IDC_COMBO_ACTION);
  }

  NEWRADIO(3);
  RADIO(IDC_RADIO_START, "start");
  RADIO(IDC_RADIO_END,   "end");
  RADIO(IDC_RADIO_EXACT, "pos");
  SAVERADIO();

  SaveItem(IDC_EDIT_X);
  SaveItem(IDC_EDIT_Y);

  SaveCheck(IDC_CHECK_RELATIVE, "relative");

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

CLEAR(mouse)
{
  delete CLEAR_GET_DATA();
}




/**************************************************\
 *                                                *
 *  Quit is called before the plugin is unloaded  *
 *  This is the place to do your code cleanup     *
 *                                                *
\**************************************************/

QUIT()
{
  QUIT_RETURN();
}













void RunMouseCommand(HWND hWnd, s_params *cmd, s_gesture *gesture)
{
  int x=0, y=0;

  POINT curPt;
  GetCursorPos(&curPt);

  // if it's not emulating something at the end (where the cursor currently is)
  // thenwe'll have to move the mouse
  if (cmd->pos != POS_END)
  {
  
    if (cmd->pos == POS_START)
    {
      x = gesture->points[0].x;
      y = gesture->points[0].y;
    }
    else if (cmd->pos == POS_SCREEN)
    {
      x = cmd->x;
      y = cmd->y;
    }
    else if (cmd->pos == POS_WINDOW)
    {
      POINT pt;
      pt.x = cmd->x;
      pt.y = cmd->y;
      ClientToScreen(hWnd, &pt);
      x = pt.x;
      y = pt.y;
    }
      
    // map points into mouse_event coordinate space
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    x = ((x * 65535) / (width-1));
    y = ((y * 65535) / (height-1));

    curPt.x = ((curPt.x * 65535) / (width-1));
    curPt.y = ((curPt.y * 65535) / (height-1));
  }


  // get the button specifics
  DWORD buttonDown, buttonUp;
  if (cmd->button == BUTTON_LEFT)
  {
    buttonDown = MOUSEEVENTF_LEFTDOWN;
    buttonUp = MOUSEEVENTF_LEFTUP;
  }

  else if (cmd->button == BUTTON_RIGHT)
  {
    buttonDown = MOUSEEVENTF_RIGHTDOWN;
    buttonUp = MOUSEEVENTF_RIGHTUP;
  }

  else   // BUTTON_MIDDLE
  {
    buttonDown = MOUSEEVENTF_MIDDLEDOWN;
    buttonUp = MOUSEEVENTF_MIDDLEUP;
  }


  DWORD flags;
  if (cmd->pos != POS_END)
    flags = MOUSEEVENTF_ABSOLUTE;
  else
    flags = 0;

  // move to the starting point
  mouse_event(flags | MOUSEEVENTF_MOVE, x, y, 0, 0);

  // if all we're doing is moving the mouse, we're done
  if (cmd->action == ACTION_MOVE)
    return;

  Sleep(10);

  // emulate the clicks
  
  if (cmd->action == ACTION_DOWN)
    mouse_event(buttonDown, 0, 0, 0, 0);
  else if (cmd->action == ACTION_UP)
    mouse_event(buttonUp, 0, 0, 0, 0);
  else if (cmd->action == ACTION_CLICK)
  {
    mouse_event(buttonDown, 0, 0, 0, 0);
    Sleep(10);
    mouse_event(buttonUp, 0, 0, 0, 0);
    Sleep(10);
  }
  else  // DBLCLICK
  {
    mouse_event(buttonDown, 0, 0, 0, 0);
    Sleep(1);
    mouse_event(buttonUp, 0, 0, 0, 0);
    Sleep(1);
    mouse_event(buttonDown, 0, 0, 0, 0);
    Sleep(1);
    mouse_event(buttonUp, 0, 0, 0, 0);
  }

  Sleep(1);

  // move the cursor back to the original position
  if (cmd->pos != POS_END)
  {
    mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, curPt.x, curPt.y, 0, 0);
    Sleep(1);
  }

}





BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg)
  {
    case WM_COMMAND:
      if (LOWORD(wParam) == IDC_RADIO_START ||  LOWORD(wParam) == IDC_RADIO_END || LOWORD(wParam) == IDC_RADIO_EXACT)
      {          
        BOOL bState = IsDlgButtonChecked(hWnd, IDC_RADIO_EXACT);
        EnableWindow(GetDlgItem(hWnd, IDC_EDIT_X), bState);
        EnableWindow(GetDlgItem(hWnd, IDC_EDIT_Y), bState);
        EnableWindow(GetDlgItem(hWnd, IDC_CHECK_RELATIVE), bState);
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
