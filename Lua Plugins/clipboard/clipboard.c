/*
Copyright (c) 2009 Jeff Doozan

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

#pragma comment(linker, "/entry:DllMain")

#include "lauxlib.h"
#include "lua.h"

#include <windows.h>



/* clipboard.get() */

static int cb_get(lua_State* L)
{
  char *lpszCBData;
  if (OpenClipboard (NULL))
  {
    lpszCBData = GetClipboardData(CF_TEXT);
    CloseClipboard();
  }

  /* Save the return string */
  lua_pushstring(L, lpszCBData);

  /* We are returning 1 parameter */
  return 1;
}


/* clipboard.set("mytext") */

static int cb_set(lua_State* L)
{
  char *lpszCopy;
  char *lpszCBData;
  
  /* Get the string passed to this function */
  lpszCBData = (char *) luaL_checkstring(L, 1);

  if (lpszCBData && *lpszCBData)
  {
    HGLOBAL hgCBData = GlobalAlloc( GMEM_MOVEABLE, (lstrlen(lpszCBData) + 1) * sizeof(char) ); 
    if ( hgCBData && OpenClipboard (NULL) )
    {
      /* clear the existing contents */
      EmptyClipboard(); 

      /* copy the string into the global memory */
      lpszCopy = GlobalLock(hgCBData);
      lstrcpy(lpszCopy, lpszCBData);
      GlobalUnlock(hgCBData); 

      /* set the clipboard */
      SetClipboardData(CF_TEXT, hgCBData);
      CloseClipboard();
    };
  }

  /* We are returning 0 parameters */
  return 0;
}

/* clipboard.clear() */

static int cb_clear(lua_State* L)
{
  if (OpenClipboard (NULL))
  {
    EmptyClipboard();
    CloseClipboard();
  };
  
  /* We are returning 0 parameters */
  return 0;
}


static const struct luaL_reg clipboard[] = {
  {"get",    cb_get},
  {"set",    cb_set},
  {"clear",  cb_clear},
  {NULL, NULL}
};

extern __declspec(dllexport) int luaopen_clipboard (lua_State *L)
{
  luaL_openlib(L, "clipboard", clipboard, 0);
  return 1;
}



BOOL APIENTRY DllMain(HANDLE hModule, DWORD reason, LPVOID lpReserved )
{
  return TRUE;
}
