#include "Plugin.h"
#include "StrLib.h"

#define BUTTON_RIGHT     1
#define BUTTON_LEFT      2
#define BUTTON_MIDDLE    3

#define ACTION_DOWN      1
#define ACTION_UP        2
#define ACTION_CLICK     3 
#define ACTION_DBLCLICK  4
#define ACTION_MOVE      5

#define POS_START        1
#define POS_END          2
#define POS_SCREEN       3
#define POS_WINDOW       4

struct s_params {
  char button;
  char action;
  char pos;
  int  x;
  int  y;
};

void RunMouseCommand(HWND hWnd, s_params *cmd, s_gesture *gesture);

BOOL CALLBACK DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
