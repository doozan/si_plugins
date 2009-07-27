/*
 *  Copyright (C) 2001-2009 Jeff Doozan
 */

#ifndef PLUGIN_INC
#  define PLUGIN_INC

#include <windows.h>


// Optimize Linker options for MS Compilers
#  ifdef _MSC_VER
#     pragma comment(linker, "/nodefaultlib")
#     pragma comment(linker, "/entry:DllMain")
#     ifndef _DEBUG
#      pragma optimize("gsy",on) // "g" - global optimizations; "s" - small code, "y" - no frame pointers
#    endif
#  endif

// Handle Strings.h

// Load Strings.h, extract the STR() definitions
#define STR(VAR) STRINGDEF_##VAR,
#define VERSION(VER)
enum {
   STRINGDEF_ERROR_OLD_STRINGS=0,
   #include "Strings.h"
   _STRING_COUNT
};  
#undef STR
#undef VERSION

extern char *_strings[_STRING_COUNT];
#define STR(VAR) _strings[STRINGDEF_##VAR]


#ifdef PRO_PLUGIN

struct s_point {
   short      x;
   short      y;
   BYTE      recorded;   // indicates whether the point was recorded from mouse movements, or interpolated
   s_point  *next;
};

struct s_gesture {
  char      *name;
   s_point  *point_head;
   s_point  *point_tail;
   int      left;
   int      right;
   int      top;
   int      bottom;
};

struct s_execParams {
   HWND        hWnd;
   s_gesture   *gesture;
   int         id;
   void        *lpData;
   char        *lpszSavedParams;
};


#else

struct s_execParams {
   HWND   hWnd;
   int    id;
   char   *lpszSavedParams;
   void   *lpData;
};

#endif

// Function pointer definitions

typedef void *(__cdecl *ExecFunc)  (s_execParams *lpAction);
typedef HWND  (__cdecl *ShowFunc)  (HWND hWndContainer, int id, char *lpszSavedParams, void *lpData);
typedef void *(__cdecl *SaveFunc)  (HWND hWndParent, int id, int *length, char *lpszBuf, void *lpData);
typedef void  (__cdecl *ClearFunc) (int id, void *lpData);



struct s_pluginCommand {
   int   id;            // the id of the window (filled automatically by StrokeIt)
   char  *pluginName;   // pointer to info on this plugin (filled automatically by StrokeIt)

   HICON icon;
   char  *command;
   char  *commandName;
   char  *commandDesc;

   ExecFunc  exec;       // function called when this command should be executed
   ShowFunc  show;       // function called to display config dialog
   SaveFunc  save;       // function called to save info in config dialog
   ClearFunc clear;      // function called to delete any data we have saved in a command

   struct s_pluginCommand *next;
};

struct s_siData {
   char *szDataDir;
   char *szInstallDir;
};

struct s_siFunctions {
   void (*SetTarget) (HWND);
   void (*StopProcessing) (DWORD);
   BOOL (*WildMatch)(const char *, const char *);
   void (*DoCommand) (char *szPluginName, char *szCmdName, char *szCmdParams);
};


struct s_pluginInitData {
   WORD   version;               // SI Plugin version (should be 0x01 0x00)
   char   *pluginData;           // Plugin description
   s_pluginCommand *commands;    // pointer to command_head
};

struct s_strings {
   char szFilePath[MAX_PATH];
   int  iCount;
   char *szVersion;
   char **pszStringIndex;
   BOOL bUnicode;
};

struct s_initParams {
   s_strings       *strings;              // pointer to s_strings structure
   s_siFunctions   *siFuncs;              // pointer to s_siFunctions
   s_siData        *siData;
};






#define EXPORT extern "C"__declspec(dllexport)

EXPORT   s_pluginInitData *Init(s_initParams *);
EXPORT   void Quit();

#undef EXPORT



void InitStrings(s_strings *strings);
s_pluginCommand *NewCommand(char *command, char *commandName, char *commandDesc, int icon, ExecFunc lpfExec, ShowFunc lpfShow, SaveFunc lpfSave, ClearFunc lpfClear);
void DeleteCommands();
BOOL APIENTRY DllMain(HANDLE hModule, DWORD reason, LPVOID lpReserved );
HWND Dialog(HWND hWndContainer, int iDlg, DLGPROC proc);


extern s_pluginCommand *command_head;
extern HINSTANCE ghInstance;
extern s_pluginInitData data;
extern s_siFunctions *siFuncs;
extern s_siData *siData;



typedef s_pluginInitData *  (__cdecl *InitFunc)(s_initParams *);
typedef void                (__cdecl *QuitFunc)(void);



// Exec


#define EXEC(CMD) \
void *_exec_step2_##CMD(s_execParams *_lpAction);         \
void *_exec_##CMD(s_execParams *_lpAction)                \
{                                                         \
  InitRestore(NULL, _lpAction->lpszSavedParams);          \
  void *ret = _exec_step2_##CMD(_lpAction);               \
  return ret;                                             \
}                                                         \
void *_exec_step2_##CMD(s_execParams *_lpAction)


#define EXEC_GET_ACTION()  _lpAction
#define EXEC_GET_ID()      _lpAction->id
#define EXEC_GET_DATA()    _lpAction->lpData
#define EXEC_GET_HWND()    _lpAction->hWnd
#define EXEC_GET_PARAMS()  _lpAction->lpszSavedParams
#define EXEC_GET_GESTURE() _lpAction->gesture


#define EXEC_RETURN(RET) \
  EndRestore();          \
  return RET




// Show


#define SHOW(CMD)                                                                                               \
HWND _show_step2_##CMD(HWND _hWnd_show_dlg, HWND hWndContainer, int id, char *lpszSavedParams, void *lpData);   \
HWND _show_##CMD(HWND hWndContainer, int id, char *lpszSavedParams, void *lpData)                               \
{                                                                                                               \
  return _show_step2_##CMD(NULL, hWndContainer, id, lpszSavedParams, lpData);                                   \
}                                                                                                               \
HWND _show_step2_##CMD(HWND _hWnd_show_dlg, HWND hWndContainer, int id, char *lpszSavedParams, void *lpData)


#define SHOW_DIALOG(ID, PROC)                        \
  _hWnd_show_dlg = Dialog(hWndContainer, ID, PROC);  \
  InitRestore(_hWnd_show_dlg, lpszSavedParams);

#define SHOW_RETURN()    \
  EndRestore();          \
  return _hWnd_show_dlg

#define SHOW_GET_ID()     id
#define SHOW_GET_DATA()   lpData
#define SHOW_GET_PARAMS() lpszSavedParams



// Quit


#define QUIT()          void Quit()
#define QUIT_RETURN()   DeleteCommands()




// Clear

#define CLEAR(CMD) void  _clear_##CMD(int id, void *lpData)
#define CLEAR_GET_ID()   id
#define CLEAR_GET_DATA() lpData
#define CLEAR_RETURN() return



// Save

#define SAVE(CMD)                                                                                              \
void *_save_step2_##CMD(char *lpszSavedParams, HWND hWnd, int id, int *iMaxBuf, char *lpszBuf, void *lpData);  \
void *_save_##CMD(HWND hWnd, int id, int *iMaxBuf, char *lpszBuf, void *lpData) {                              \
  char *lpszSavedParams = new char[4096];                                                                      \
  InitSave(hWnd, lpszSavedParams, 4095);                                                                       \
  void *ret = _save_step2_##CMD(lpszSavedParams, hWnd, id, iMaxBuf, lpszBuf, lpData);                          \
  delete lpszSavedParams;                                                                                      \
  return ret;                                                                                                  \
}                                                                                                              \
void *_save_step2_##CMD(char *lpszSavedParams, HWND hWnd, int id, int *iMaxBuf, char *lpszBuf, void *lpData)

#define SAVE_GET_DATA()   lpData
#define SAVE_GET_HWND()   hWnd
#define SAVE_GET_ID()     id
#define SAVE_GET_PARAMS() lpszSavedParams


#define SAVE_RETURN()  \
   int iSavedDataLen = lstrlen(lpszSavedParams); \
   if (lpszBuf && *iMaxBuf >= iSavedDataLen) \
      lstrcpy(lpszBuf, lpszSavedParams); \
   *iMaxBuf = iSavedDataLen; \
   EndSave(); \
   return NULL;




// Declare

#define DECLARE_COMMAND(CMD)                                                                     \
void *_exec_##CMD   (s_execParams *_lpAction);                                                   \
HWND  _show_##CMD   (HWND hWndContainer, int id, char *lpszSavedParams, void *lpData);           \
void  _clear_##CMD  (int id, void *lpData);                                                      \
void *_save_##CMD   (HWND hWndParent, int id, int *iMaxBuf, char *lpszBuf, void *lpData);        \
s_pluginCommand *_pCmd_##CMD = NULL;


#define DECLARE_EXEC(CMD) \
void *_exec_##CMD   (s_execParams *_lpAction);

#define DECLARE_SHOW(CMD) \
HWND  _show_##CMD   (HWND hWndContainer, int id, char *lpszSavedParams, void *lpData);

#define DECLARE_CLEAR(CMD) \
void  _clear_##CMD  (int id, void *lpData);

#define DECLARE_SAVE(CMD) \
void *_save_##CMD   (HWND hWndParent, int id, int *iMaxBuf, char *lpszBuf, void *lpData);




// INIT

#define INIT_COMMAND(CMD, NAME, DESC, ICON) \
_pCmd_##CMD = NewCommand(#CMD, NAME, DESC, ICON, _exec_##CMD, _show_##CMD, _save_##CMD, _clear_##CMD);

#define INIT_COMMAND_CUSTOM(CMD, NAME, DESC, ICON, EXEC, SHOW, SAVE, CLEAR) \
_pCmd_##CMD = NewCommand(#CMD, NAME, DESC, ICON, _exec_##EXEC, _show_##SHOW, _save_##SAVE, _clear_##CLEAR);


#define INIT()                                       \
struct s_pluginInitData *PostInit();                 \
struct s_pluginInitData *Init(s_initParams *ip) {    \
 siFuncs = ip->siFuncs;                              \
 siData = ip->siData;                                \
 InitStrings(ip->strings);                           \
 return PostInit();                                  \
}                                                    \
struct s_pluginInitData *PostInit()

#ifdef PRO_PLUGIN
#define INIT_RETURN(DESC) \
   data.version = 0x0102; \
   data.pluginData = DESC; \
   data.commands = command_head; \
   return &data;
#else
#define INIT_RETURN(DESC) \
   data.version = 0x0100; \
   data.pluginData = DESC; \
   data.commands = command_head; \
   return &data;
#endif




#define GET_CMD_ID(CMD)  _pCmd_##CMD->id




#ifdef USE_UNICODE

#define SET(ID, STRING) \
   if (gbUnicode) \
      SetDlgItemTextW(hWnd, ID, (LPWSTR) STRING); \
   else \
      SetDlgItemText(hWnd, ID, STRING);
#else

#define SET(ID, STRING) \
  SetDlgItemText(_hWnd_show_dlg, ID, STRING);
#endif
  


#define COMBO(ID, STR, VAL) AddCombo(_hWnd_show_dlg, ID, STR, VAL);

#define HIDE_CONTROL(ID) ShowWindow(GetDlgItem(_hWnd_show_dlg, ID), SW_HIDE)

#define IDC_PARENT_CONTAINER  1031

#define SP_STOP_ALL     0x0001
#define SP_NEXT_APP     0x0002

#define SI_SHOW           49001
#define SI_LEARN          49002
#define SI_DISABLE        49003
#define SI_SHUTDOWN       49004
#define SI_IGNORENEXT     49005
#define SI_DONELEARNING   49006
#define SI_DONEEDITING    49007
#define SI_HIDETRAY       49008
#define SI_SHOWTRAY       49009
#define SI_RELOAD         49010



#endif // PLUGIN_INC

