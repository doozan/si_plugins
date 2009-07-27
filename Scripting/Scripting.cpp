/*
 *  Copyright (C) 2001-2009 Jeff Doozan
 */

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#define PRO_PLUGIN
#include "Scripting.h"
#include "resource.h"

// maximum number of exported commands per script
#define MAX_COMMANDS 100

/****************************************************
  Global Variables
*****************************************************/

lua_State *L_global = NULL;
lua_State *L_new = NULL;
s_cmd_data gData;

char *gszUserScripts[MAX_PATH];
char *gszGlobalScripts[MAX_PATH];

// used to hold the combo box string values while the dialog is visible
s_commandData *gpCmdData[MAX_COMMANDS];
int giCmdData;


/*******************************************
   Lua function tables
********************************************/

static const struct luaL_reg gesture_lib [] =
{
  {"hwnd",        l_gesture_get_hwnd},
  {"point",       l_gesture_get_point},
  {"point_count", l_gesture_get_point_count},
  {"rect",        l_gesture_get_bounding_rect},
  {"name",        l_gesture_get_name},
  {NULL, NULL}  /* sentinel */
};

static const struct luaL_reg plugin_lib [] =
{
  {"call",          l_plugin_call},
  {NULL, NULL}  /* sentinel */
};



/*******************************************
   Lua functions
********************************************/


int l_gesture_get_hwnd(lua_State *L)
{
  lua_pushnumber(L, (LUA_NUMBER) (DWORD) gData.params->hWnd );
  return 1; // number of return values
}

int l_gesture_get_name(lua_State *L)
{
  lua_pushstring(L, gData.params->gesture->name );
  return 1; // number of return values
}

int l_gesture_get_bounding_rect(lua_State *L)
{
  lua_pushnumber(L, (LUA_NUMBER) gData.params->gesture->left );
  lua_pushnumber(L, (LUA_NUMBER) gData.params->gesture->top );
  lua_pushnumber(L, (LUA_NUMBER) gData.params->gesture->right );
  lua_pushnumber(L, (LUA_NUMBER) gData.params->gesture->bottom );
  return 4; // number of return values
}


int get_point_count(s_point *point)
{
  int i = 0;
  while (point)
  {
    i++;
    point = point->next;
  }
  return i;
}

int l_gesture_get_point_count(lua_State *L)
{
  if (!gData.point_count)
    gData.point_count = get_point_count(gData.params->gesture->point_head);

  lua_pushnumber(L, (LUA_NUMBER) gData.point_count );
  return 1; // number of return values
}



int l_gesture_get_point(lua_State *L)
{
  int idx = (int) luaL_checknumber(L, 1);

  if (idx == 0)
  {
   lua_pushnumber(L, (LUA_NUMBER) gData.params->gesture->point_head->x );
   lua_pushnumber(L, (LUA_NUMBER) gData.params->gesture->point_head->y );
  }
  else if (idx == -1)
  {
   lua_pushnumber(L, (LUA_NUMBER) gData.params->gesture->point_tail->x );
   lua_pushnumber(L, (LUA_NUMBER) gData.params->gesture->point_tail->y );
  }
  else
  {
    // script is looking at more than just the first and the last points,
    // chances are it's going to want to walk through the entire list so
    // let's convert the linked list to an array for faster repeat access
    if (!gData.points)
    {
      if (!gData.point_count)
        gData.point_count = get_point_count(gData.params->gesture->point_head);

      gData.points = new POINT[gData.point_count];

      s_point *point = gData.params->gesture->point_head;
      for (int i=0; i<gData.point_count; i++)
      {
        gData.points[i].x = point->x;
        gData.points[i].y = point->y;
        point = point->next;
      }
    }

    if (idx < 0)
      idx = 0;

    if (idx >= gData.point_count)
      idx = gData.point_count-1;

    lua_pushnumber(L, (LUA_NUMBER) gData.points[idx].x );
    lua_pushnumber(L, (LUA_NUMBER) gData.points[idx].y );

  }


  return 2; // number of return values
}





void mlua_append(lua_State *L, char *table, char *value, char *str)
{
  // append 
  char new_val[4096];
  const char *val;

  lua_getglobal(L, table);

  lua_pushstring(L, value);
  lua_rawget(L, -2);

  val = lua_tostring(L, -1);
  lstrcpy(new_val, val);
  lstrcat(new_val, str);

  lua_pop(L, 1);                           //get rid of old val
  lua_pushstring(L, value);
  lua_pushstring(L, new_val);

  lua_rawset(L, -3);

  lua_pop(L, 1);  // remove table

}



int l_plugin_call(lua_State *L)
{
  char *plugin = (char *) luaL_checkstring(L, 1);
  char *command = (char *) luaL_checkstring(L, 2);
  char *params = (char *) luaL_checkstring(L, 3);

  int escaped_count = Escape(NULL, params, '\\');
  if (escaped_count)
  {
    char *escaped_params = new char[ lstrlen(params) + escaped_count + 1];
    Escape(escaped_params, params, '\\');

    siFuncs->DoCommand(plugin, command, escaped_params);
    delete escaped_params;
  }
  else
    siFuncs->DoCommand(plugin, command, params);

  return 0; // number of return values
}


lua_State *new_state()
{
  lua_State *L = luaL_newstate();

  lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
  luaL_openlibs(L);  /* open libraries */
  lua_gc(L, LUA_GCRESTART, 0);

 
  luaL_openlib(L, "gesture", gesture_lib, 0);
  luaL_openlib(L, "plugin", plugin_lib, 0);

  char add_path[MAX_PATH*2];

  wsprintf(add_path, ";%slib\\?.dll;%slib\\?.dll", gszUserScripts, gszGlobalScripts);
  mlua_append(L, "package", "cpath", add_path);

  wsprintf(add_path, ";%sinclude\\?.lua;%sinclude\\?.lua", gszUserScripts, gszGlobalScripts);
  mlua_append(L, "package", "path", add_path);

  return L;
}










BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// predeclare our functions so we can use them as parameters in NewCommand
DECLARE_COMMAND(script);


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

   INIT_COMMAND(script, STR(SCRIPT), STR(SCRIPT_DESC), ICON_LUA);

   wsprintf((char *)gszGlobalScripts, "%sScripts\\", siData->szInstallDir);
   wsprintf((char *)gszUserScripts, "%sScripts\\", siData->szDataDir);

   gData.params = NULL;
   gData.point_count = 0;
   gData.points = NULL;

   L_global = new_state();
   L_new = new_state();

   INIT_RETURN("Scripting Engine");
}




/**************************************************************************\
 *                                                                        *
 *  EXEC functions are called when the command needs to be performed      *
 *                                                                        *
 *  Any return value will be saved and can be retrieved with              *
 *  EXEC_GET_DATA() on any subsequent calls to the EXEC function.         *
 *                                                                        *
\**************************************************************************/

BOOL FileExists(char *path) {

  HANDLE hFile;
  hFile = CreateFile(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile != INVALID_HANDLE_VALUE) {
    CloseHandle(hFile);
    return TRUE;
  }
  else
    return FALSE;
}

int StrStartsWith(char *str, char *start)
{
  while (*start && *str && (*str == *start) )
  {
    str++;
    start++;
  }
  if (!*start)
    return 1;

  return 0;
}

char *findfile(char *filename)
{
  char file[MAX_PATH];

  lstrcpy(file, filename);
  if (mstrchr(file, '\\') && FileExists(file))
    return mstrdup(file);

  wsprintf(file, "%s%s", gszUserScripts, filename);
  if (FileExists(file))
    return mstrdup(file);

  wsprintf(file, "%s%s", gszGlobalScripts, filename);
  if (FileExists(file))
    return mstrdup(file);

  return NULL;
}



data_item *InitParams(char *params) {

  data_item *d = new data_item;

  InitRestore(NULL, params);   
    
  // get message      
  char *str = RestoreStr();
  if (!lstrcmp(str, "global"))
  {
    d->state_type = STATE_GLOBAL;
    d->state = NULL;
  }
  else if (!lstrcmp(str, "new"))
  {
    d->state_type = STATE_NEW;
    d->state = NULL;
  }
  else
  {
    d->state_type = STATE_REUSE;
    d->state = new_state();
  }

  str = RestoreStr();
  if (str)
  {
    d->script = findfile(str);
  }
  else
  {
    d->script = NULL;
  }

  int i=0;

  str = RestoreStr();
  if (str)
  {
    d->command = mstrdup(str);
  }
  else
  {
    d->command = NULL;
  }

  str = RestoreStr();
  if (str)
  {
    d->args = mstrdup(str);
  }
  else
  {
    d->args = NULL;
  }


  EndRestore();

  return d;
}


static int getargs (lua_State *L, char **argv) {
  int n = 1;
  int narg;
  int i;
  int argc = 0;
  while (argv[argc]) argc++;  /* count total number of arguments */
  narg = argc - (n + 1);  /* number of arguments to the script */
  luaL_checkstack(L, narg + 3, "too many arguments to script");
  for (i=n+1; i < argc; i++)
    lua_pushstring(L, argv[i]);
  lua_createtable(L, narg, n + 1);
  for (i=0; i < argc; i++) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i - n);
  }
  return narg;
}

PCHAR* StringToArgv(PCHAR szCmdLine, int* _argc)
{
  PCHAR*  argv;
  PCHAR  _argv;
  ULONG   len;
  ULONG   argc;
  CHAR    a;
  ULONG   i, j;

  BOOLEAN  in_QM;
  BOOLEAN  in_TEXT;
  BOOLEAN  in_SPACE;

  len = lstrlen(szCmdLine);
  i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);
  int memsize = i + (len+2)*sizeof(CHAR);

  argv = (PCHAR*) HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE, memsize);

  _argv = (PCHAR)(((PUCHAR)argv)+i);

  argc = 0;
  argv[argc] = _argv;
  in_QM = FALSE;
  in_TEXT = FALSE;
  in_SPACE = TRUE;
  i = 0;
  j = 0;

  while( a = szCmdLine[i] )
  {
    if(in_QM)
    {
      if(a == '\"')
      {
        in_QM = FALSE;
      }
      else
      {
        _argv[j] = a;
        j++;
      }
    }
    else
    {
      switch(a)
      {

        case '\"':
          in_QM = TRUE;
          in_TEXT = TRUE;
          if(in_SPACE)
          {
            argv[argc] = _argv+j;
            argc++;
          }
          in_SPACE = FALSE;
          break;

        case ' ':
        case '\t':
        case '\n':
        case '\r':
          if(in_TEXT)
          {
            _argv[j] = '\0';
            j++;
          }
          in_TEXT = FALSE;
          in_SPACE = TRUE;
          break;

        default:
          in_TEXT = TRUE;
          if(in_SPACE)
          {
            argv[argc] = _argv+j;
            argc++;
          }
          _argv[j] = a;
          j++;
          in_SPACE = FALSE;
          break;
      }
    }
    i++;
  }

  _argv[j] = '\0';
  argv[argc] = NULL;

  (*_argc) = argc;
  return argv;
}




EXEC(script)
{

  struct data_item *d;
  lua_State *L;

  // save the gesture data in a global variable
  gData.params = EXEC_GET_ACTION();

  // delete any old point data from the last command
  if (gData.points)
  {
    delete gData.points;
    gData.points = NULL;
  }
  gData.point_count = 0;

  // get the associated data,
  // if this is the first time the command has run,
  // initialize new data
  d = (data_item *) EXEC_GET_DATA();
  if ( !d )
    d = InitParams( EXEC_GET_PARAMS() );

  // get the appropriate lua state
  switch (d->state_type)
  {
    case STATE_GLOBAL:
      L = L_global; break;
    case STATE_NEW:
      L = L_new; break;
    default:
      L = d->state; break;
  }

  char *szCommand=NULL;
  char *szArgs=NULL;

  // If we're calling a specific command in the script, read the script
  // details and get the associated command data
  if (d->command && *d->command)
  {
    s_commandData *pCmdData[MAX_COMMANDS];
    int iCmdData = GetScriptDetails(d->script, (s_commandData **) &pCmdData);

    int i=0;
    while (i<iCmdData && lstrcmp(pCmdData[i]->szCmdName, d->command)) i++;
    // found a match
    if (i < iCmdData)
    {
      szCommand = mstrdup(pCmdData[i]->szCmdData);
      szArgs = mstrdup(d->args);
    }
    DeleteScriptDetails( (s_commandData **) &pCmdData, iCmdData);
  }

  char szLua[MAX_PATH+1];
  GetModuleFileName(ghInstance, (char *) szLua, MAX_PATH);

  char *szCommandLine = new char[ lstrlen((char *)szLua) + 3 + lstrlen(d->script) + 3 + lstrlen(d->args) +1 ];
  wsprintf(szCommandLine, "\"%s\" \"%s\" %s", szLua, d->script, szArgs);

  int argc;
  char **argv = StringToArgv(szCommandLine, &argc);

  int narg = getargs(L, argv);  /* collect arguments */
  lua_setglobal(L, "arg");

  delete argv;
  argv = NULL;

  int iResult = luaL_loadfile(L, d->script);
  iResult = lua_pcall(L, 0, LUA_MULTRET, 0);

  // call a command in the file
  if ( iResult==0 && szCommand )
    luaL_dostring(L, szCommand);

  delete szCommand;
  delete szArgs;

  switch (iResult)
  {
    case 0:			// no errors
	    break;
    case LUA_ERRSYNTAX:	// syntax error during pre-compilation
    {
	    MessageBox(NULL, "Syntax error during pre-compilation", "Script Error", MB_ICONERROR);
	    break;
    }
    case LUA_ERRMEM:	// memory allocation error.
    {
	    MessageBox(NULL, "memory allocation error during load", "Script Error", MB_ICONERROR);
	    break;
    }
    case LUA_ERRFILE:	//  error reading file
    {
      char err[MAX_PATH+50];
      wsprintf(err, "Error reading file '%s'",  d->script);
	    MessageBox(NULL, err, "Script Error", MB_ICONERROR);
 		  break;
    }
    default:		// display the error message.
    {
      const char* szErrorText;
 		  szErrorText = lua_tostring(L,-1);

      char err[MAX_PATH];
      wsprintf(err, "Script Error in '%s'",  d->script);
	    MessageBox(NULL, szErrorText, err, MB_ICONERROR);

      break;
    }
  }

  // if we used a new, clean environment, we need to destroy it
  // and create a new one for the next command that needs it
  if ( d->state_type == STATE_NEW )
  {
    lua_close(L);
    L_new = new_state();
  }

  // save our data
  return d;
}



/**************************************************************\
 *                                                            *
 *  SHOW is called when a command is going to be edited.      *
 *  Create and initialize the appropriate dialog box.         *
 *                                                            *
\**************************************************************/

void DeleteScriptDetails(s_commandData **pCmdData, int iCmdData)
{
  for (int i=0; i<iCmdData; i++)
  {
    delete pCmdData[i]->szArgsDesc;
    delete pCmdData[i]->szCmdName;
    delete pCmdData[i]->szCmdData;
    delete pCmdData[i];
    pCmdData[i] = NULL;
  }
}


int GetScriptDetails(char *szFileName, s_commandData **pCmdData)
{
  if (!szFileName) return 0;

  int iCmdData = NULL;

  FILE *pFile;
  pFile = fopen (szFileName, "r");

  if (pFile != NULL)
  {
    char szLine[1024+1];
    BOOL bInitLine;

    do
    {
      fgets (szLine, 1024, pFile);
      char *c = szLine;

      // strip off any trailing newline characters
      while ( *c && '\n' != *c && '\r' != *c) c++;
      if (*c) *c = '\0';

      if( StrStartsWith(szLine, "--!SI:") )
      {
        bInitLine = TRUE;
        char *szArgsDesc = NULL;
        char *szCmdName = NULL;
        char *szCmdData = NULL;

        char *szData = szLine + lstrlen("--!SI:");

        // check for @ArgsDescription
        if (*szData == '@')
        {
          szArgsDesc = szData + 1;

          szCmdName = strchr(szArgsDesc, ':');
          if ( szCmdName )
          {
            *szCmdName = 0;
            szCmdName++;
          }
        }
        else
          szCmdName = szData;

        // check for Name:Command
        szCmdData = strchr(szCmdName, ':');
        if ( szCmdData )
        {
          *szCmdData='\0'; // null terminate the 'data'
          szCmdData++; // skip past the ':'
          
          pCmdData[iCmdData] = new s_commandData;
          pCmdData[iCmdData]->bHasArgs = (szArgsDesc != NULL);
          pCmdData[iCmdData]->szArgsDesc = mstrdup(szArgsDesc);
          pCmdData[iCmdData]->szCmdData = mstrdup(szCmdData);
          pCmdData[iCmdData]->szCmdName = mstrdup(szCmdName);

          iCmdData++;
        }
        
      }
      else
      {
        bInitLine = FALSE;
      }
    } while(bInitLine);

    fclose (pFile);
  }

  return iCmdData;
}


SHOW(script)
{
  // Create the dialog box inside the containing window
  HWND hWnd = SHOW_DIALOG(DLG_COMMAND, DlgProc);

  SET(IDC_IDENTIFIER, STR(IDENTIFIER));
  SET(IDC_INFO, STR(HELP));

  SET(IDC_STATIC_OPTIONS, STR(OPTIONS));

  SET(IDC_STATIC_ENVIRONMENT, STR(ENVIRONMENT));
  SET(IDC_STATIC_COMMAND, STR(COMMAND));
  SET(IDC_STATIC_ARGS, STR(ARGUMENTS));



  COMBO(IDC_COMBO_ENVIRONMENT, STR(ENV_NEW), "new");
  COMBO(IDC_COMBO_ENVIRONMENT, STR(ENV_REUSE), "reuse");
  COMBO(IDC_COMBO_ENVIRONMENT, STR(ENV_GLOBAL), "global");
  RestoreCombo(IDC_COMBO_ENVIRONMENT);

  char *szFileName = mstrdup( RestoreItem(IDC_EDIT_FILE) );



  BOOL bHasArgs=FALSE;
  BOOL bHasCommands=FALSE;

  if (szFileName && *szFileName)
  {
    char *szFullFileName = findfile((char *)szFileName);

    DeleteScriptDetails((s_commandData **) &gpCmdData, giCmdData);
    giCmdData = GetScriptDetails(szFullFileName, (s_commandData **) &gpCmdData);
    delete szFullFileName;

    // populate the dropdown
    for (int i = 0; i<giCmdData; i++)
      AddCombo(hWnd, IDC_COMBO_COMMAND, gpCmdData[i]->szCmdName, gpCmdData[i]->szCmdName);
  }

  if (giCmdData > 0)
  {
    SendDlgItemMessage(hWnd, IDC_COMBO_COMMAND, CB_SETCURSEL, 0, 0);
  }
  else
  {
    HIDE_CONTROL(IDC_STATIC_COMMAND);
    HIDE_CONTROL(IDC_COMBO_COMMAND);
  }

  RestoreCombo(IDC_COMBO_COMMAND);
  RestoreItem(IDC_EDIT_ARGS);

  // show/hide the args depending on the selection
  PostMessage(hWnd, WM_COMMAND, MAKELONG(IDC_COMBO_COMMAND, CBN_SELCHANGE), (LPARAM) GetDlgItem(hWnd, IDC_COMBO_COMMAND));

  SHOW_RETURN();
}



/*****************************************************************\
 *                                                               *
 *  SAVE is called when the config dialog is about to be closed  *
 *  Save the command parameters using save commands              *
 *  return with SAVE_RETURN()                                    *
 *                                                               *
\*****************************************************************/

SAVE(script)
{
  SaveCombo(IDC_COMBO_ENVIRONMENT);
  SaveItem(IDC_EDIT_FILE); 
  SaveCombo(IDC_COMBO_COMMAND);
  SaveItem(IDC_EDIT_ARGS);

  // if the string buffer is set, then we're safe to delete the combo data
  if (lpszBuf)
  {
    DeleteScriptDetails((s_commandData **) gpCmdData, giCmdData);
  }

  SAVE_RETURN();
}



/***********************************************************\
 *                                                         *
 *  CLEAR is called when a command associated with this    *
 *  plugin is about to be destroyed.  This is your chance  *
 *  to delete any data you have stored in the command      *
 *                                                         *
\***********************************************************/

CLEAR(script)
{
  data_item *d = (data_item *) CLEAR_GET_DATA();

  if (d)
  {
    delete d->args;
    delete d->command;
    delete d->script;
    if (d->state)
      lua_close(d->state);
    delete d;
  }

}




/**************************************************\
 *                                                *
 *  Quit is called before the plugin is unloaded  *
 *  This is the place to do you code cleanup      *
 *                                                *
\**************************************************/

QUIT()
{
  lua_close(L_global);
  lua_close(L_new);
  delete gData.points;

  QUIT_RETURN();
}



BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
   switch (uMsg) {
//		case WM_INITDIALOG:
//         break;
   case WM_COMMAND:
      switch (LOWORD(wParam)) {

      case IDC_COMBO_COMMAND:
      {
        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
          int i = SendDlgItemMessage(hWnd, IDC_COMBO_COMMAND, CB_GETCURSEL, 0, 0);
          if (i != CB_ERR && i >= 0 && i < giCmdData && gpCmdData[i]->bHasArgs)
          {
            ShowWindow(GetDlgItem(hWnd, IDC_STATIC_ARGS_DESCRIPTION), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_STATIC_ARGS), SW_SHOW);
            ShowWindow(GetDlgItem(hWnd, IDC_EDIT_ARGS), SW_SHOW);
            SetDlgItemText(hWnd, IDC_STATIC_ARGS_DESCRIPTION, gpCmdData[i]->szArgsDesc);
          }
          else
          {
            ShowWindow(GetDlgItem(hWnd, IDC_STATIC_ARGS_DESCRIPTION), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_STATIC_ARGS), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_EDIT_ARGS), SW_HIDE);
          }

        }
        break;
      }

      case IDC_CMD_EDIT:
      {
        char szFileName[MAX_PATH+1];
        GetDlgItemText(hWnd, IDC_EDIT_FILE, szFileName, MAX_PATH);

        char *szFile = findfile(szFileName);
        if (!szFile)
        {
          szFile = new char[MAX_PATH+1];
          wsprintf(szFile, "%s%s", gszUserScripts, szFileName);
        }


        char szViewer[MAX_PATH+1];

        char temp[MAX_PATH];
        GetTempPath(MAX_PATH, temp);
        lstrcat(temp, "sitemp.txt");
        HANDLE hFile = CreateFile(temp, 0, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        DWORD err = GetLastError();
        if (hFile != INVALID_HANDLE_VALUE) {
           FindExecutable(temp, NULL, szViewer);
           CloseHandle(hFile);
           if (err != ERROR_ALREADY_EXISTS)
              DeleteFile(temp);
        }

        if (!*szViewer)
        {
          lstrcpy(szViewer, "notepad.exe");
        }
        
        SHELLEXECUTEINFO se = { 0 };

        se.cbSize = sizeof(SHELLEXECUTEINFO);
        se.fMask = SEE_MASK_DOENVSUBST | SEE_MASK_NOCLOSEPROCESS;
        se.lpVerb = "Open";
        se.lpFile = szViewer;
        se.lpParameters = szFile;
        se.lpDirectory = NULL;
        se.nShow = SW_SHOWNORMAL;

        ShellExecuteEx(&se);
        

        break;
      }

      case IDC_CMD_BROWSE:
         char *buf = new char[MAX_PATH+1];
         *buf = 0;
         
         OPENFILENAME ofn = {0};
         ofn.lStructSize = sizeof(OPENFILENAME);
         ofn.hInstance = ghInstance;
         ofn.hwndOwner = hWnd;
         
         // "Lua Files\0*.lua\0All Files\0*.*\0";
         char *filter = new char[256];
         char *end = filter;

         lstrcpy(end, STR(LUA_FILES));
         end += lstrlen(end) + 1;
         lstrcpy(end, "*.lua");
         end += lstrlen(end) + 1;

         lstrcpy(end, STR(ALL_FILES));
         end += lstrlen(end) + 1;
         lstrcpy(end, "*.*");
         end += lstrlen(end) + 1;

         *end = 0;

         char szFile[MAX_PATH+1];
         GetDlgItemText(hWnd, IDC_EDIT_FILE, szFile, MAX_PATH);

         ofn.lpstrFile = szFile;
         ofn.lpstrInitialDir = (char *)gszUserScripts;
         ofn.lpstrFilter = filter;
         ofn.lpstrFile = buf;
         ofn.nMaxFile = MAX_PATH;
         ofn.Flags = OFN_PATHMUSTEXIST | 	OFN_LONGNAMES | OFN_EXPLORER;
         
         if (GetOpenFileName(&ofn))
         {
           if ( StrStartsWith(buf, (char *)gszUserScripts) )
           {
             SetDlgItemText(hWnd, IDC_EDIT_FILE, buf + lstrlen((char *)gszUserScripts));
           }
           else if ( StrStartsWith(buf, (char *)gszGlobalScripts) )
           {
             SetDlgItemText(hWnd, IDC_EDIT_FILE, buf + lstrlen((char *)gszGlobalScripts));
           }
           else
           {
             SetDlgItemText(hWnd, IDC_EDIT_FILE, buf);
           }

            // clear the dropdown
            SendDlgItemMessage(hWnd, IDC_COMBO_COMMAND, CB_RESETCONTENT, 0, 0);

            DeleteScriptDetails((s_commandData **) &gpCmdData, giCmdData);
            giCmdData = GetScriptDetails(buf, (s_commandData **) &gpCmdData);


            if (giCmdData > 0)
            {
              // populate the dropdown
              for (int i = 0; i<giCmdData; i++)
                 AddCombo(hWnd, IDC_COMBO_COMMAND, gpCmdData[i]->szCmdName, gpCmdData[i]->szCmdName);

              SendDlgItemMessage(hWnd, IDC_COMBO_COMMAND, CB_SETCURSEL, 0, 0);

              ShowWindow(GetDlgItem(hWnd, IDC_STATIC_COMMAND), SW_SHOW);
              ShowWindow(GetDlgItem(hWnd, IDC_COMBO_COMMAND), SW_SHOW);
            }
            else
            {
              ShowWindow(GetDlgItem(hWnd, IDC_STATIC_COMMAND), SW_HIDE);
              ShowWindow(GetDlgItem(hWnd, IDC_COMBO_COMMAND), SW_HIDE);
            }

            // show/hide the args depending on the selection
            SendMessage(hWnd, WM_COMMAND, MAKELONG(IDC_COMBO_COMMAND, CBN_SELCHANGE), (LPARAM) GetDlgItem(hWnd, IDC_COMBO_COMMAND));

         }

         delete filter;
         delete buf;

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
