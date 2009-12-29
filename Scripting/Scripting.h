#include <windows.h>


#include "Plugin.h"
#include "StrLib.h"



int l_gesture_get_hwnd(lua_State *L);
int l_gesture_get_hwnd_child(lua_State *L);
int l_gesture_get_point(lua_State *L);
int l_gesture_get_point_count(lua_State *L);
int l_gesture_get_bounding_rect(lua_State *L);
int l_gesture_get_name(lua_State *l);

int l_plugin_call(lua_State *l);

enum {
  STATE_GLOBAL=0,
  STATE_NEW,
  STATE_REUSE
};


struct s_commandData
{
  BOOL bHasArgs;
  char *szArgsDesc;
  char *szCmdName;
  char *szCmdData;
};


struct s_cmd_data
{
  s_execParams *params;
  POINT *points;
  int point_count;
};


// Data structure used by InitParams
struct data_item {
  int state_type;
  lua_State *state; 
  char *script;
  char *command;
  char *args;
};


void DeleteScriptDetails(s_commandData **pCmdData, int iCmdData);
int GetScriptDetails(char *szFileName, s_commandData **pCmdData);
