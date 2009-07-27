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

#include "Plugin.h"
#include "StrLib.h"


char *_strings[_STRING_COUNT];


HINSTANCE ghInstance;
s_pluginCommand *command_head=NULL, *command_tail=NULL;
s_pluginInitData data;
s_siFunctions *siFuncs;
s_siData *siData;


void InitStrings(s_strings *strings) {

  int iCopyCount = strings->iCount;
  if (_STRING_COUNT < strings->iCount)
    iCopyCount = _STRING_COUNT;

  mmemcpy(&_strings, strings->pszStringIndex, (sizeof(char *) * (iCopyCount) ) );
  
  if (_STRING_COUNT > strings->iCount) {
    for (int x = strings->iCount; x<_STRING_COUNT; x++) {
      _strings[x] = "";
    }
  }


  // Extract the version from Strings.h

  #undef STR
  #define VERSION(VER) char *VERSION = #VER;
  #define STR(VAR)
    #include "Strings.h"
  #undef STR
  #undef VERSION
  #define STR(VAR) _strings[STRINGDEF_##VAR]


  if (lstrcmpi(strings->szVersion, VERSION))
    MessageBox(NULL, STR(ERROR_OLD_STRINGS), strings->szFilePath, MB_OK | MB_ICONERROR);

}


s_pluginCommand *NewCommand(char *command, char *commandName, char *commandDesc, int icon, ExecFunc lpfExec, ShowFunc lpfShow, SaveFunc lpfSave, ClearFunc lpfClear) {
  s_pluginCommand *command_new = new s_pluginCommand;

  if (command_head == NULL)
    command_head = command_tail = command_new;
  else {
    command_tail->next = command_new;
    command_tail = command_new;
  }

  command_new->next = NULL;
  command_new->command = command;
  command_new->commandName = commandName;
  command_new->commandDesc = commandDesc;
  command_new->exec  = lpfExec;
  command_new->save  = lpfSave;
  command_new->show  = lpfShow;
  command_new->clear = lpfClear;
  command_new->icon  = LoadIcon(ghInstance, MAKEINTRESOURCE(icon));

  return command_new;
}

void DeleteCommands() {
  s_pluginCommand *temp;
  while (command_head) {
    temp = command_head;
    command_head = command_head->next;
    delete temp;
  }
  command_tail = NULL;
}

extern HANDLE ghFileShared;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reason, LPVOID lpReserved ) {
  switch (reason) {
    case DLL_PROCESS_ATTACH: {
      ghInstance = (HINSTANCE) hModule;
      break; }
    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    break;
  }
  return TRUE;
}


HWND Dialog (HWND hWndParent, int iDlg, DLGPROC proc) {
  HWND hWnd = CreateDialogParam(ghInstance ,MAKEINTRESOURCE(iDlg), hWndParent, (DLGPROC)proc, 0);

  RECT rect;
  HWND hWndContainer = GetDlgItem(hWndParent, IDC_PARENT_CONTAINER);
  GetClientRect(hWndContainer, &rect);

  POINT pt;

  pt.x = rect.right;
  pt.y = rect.top;

  int width = rect.right - rect.left;  
  int height = rect.bottom - rect.top;
  
  ClientToScreen(hWndContainer, &pt);
  ScreenToClient(hWndParent, &pt);

  MoveWindow(hWnd, 0, pt.y, width, height, FALSE);
  ShowWindow(hWnd, SW_SHOW);

  return hWnd;
}


HANDLE _heap = NULL;
void * operator new(size_t size) {
  if (!_heap) _heap = GetProcessHeap();
  return HeapAlloc(_heap, HEAP_NO_SERIALIZE, size);
}

void * operator new [] (size_t size) {
  if (!_heap) _heap = GetProcessHeap();
  return HeapAlloc(_heap, HEAP_NO_SERIALIZE, size);
}

void operator delete(void *p) {
  if (!_heap) _heap = GetProcessHeap();
  HeapFree(_heap, HEAP_NO_SERIALIZE, p);
}

void operator delete [] (void *p) {
  if (!_heap) _heap = GetProcessHeap();
  HeapFree(_heap, HEAP_NO_SERIALIZE, p);
}


void * __cdecl memset (void *dst, int val, size_t count)
{
  void *start = dst;

  while (count--) {
   *(char *)dst = (char)val;
   dst = (char *)dst + 1;
  }

  return(start);
}

