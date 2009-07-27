#include <windows.h>
#include "Plugin.h"
#include "StrLib.h"


BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


#define IDENTIFY_CURRENT   0
#define IDENTIFY_CLASS     1
#define IDENTIFY_CAPTION   2

struct s_msg {
   UINT   uMsg;
   WPARAM wParam;
   LPARAM lParam;
   int    iIdentifyType;
   BOOL   bIdentifyPattern;
   char   *sIdentifyText;
};


BOOL CALLBACK EnumWindowProc(HWND hWnd, LPARAM lParam);
char *ReadFile(char *fileName);
DWORD GetValueFromFile(char *fileName, char *id);
s_msg *InitParams(char *params);
HWND GetTarget(s_msg *msg, HWND hwndDefault);
void PopulateCombo(HWND hWnd, int id, char *fileName);
HWND FindWindowCaption(char *caption, BOOL bPattern);

