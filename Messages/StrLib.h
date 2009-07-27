/*
 *  Copyright (C) 2001-2009 Jeff Doozan
 */


#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


struct s_radio {
   int   id;
   char *val;
};


#define NEWRADIO(COUNT) \
   { \
      int radiocount = 0; \
      s_radio radio[COUNT];

#define RADIO(ID, STR) \
      radio[radiocount].id = ID; \
      radio[radiocount].val = STR; \
      radiocount++;

#define SAVERADIO() \
      SaveRadio((s_radio *) &radio, radiocount); \
   }

#define RESTORERADIO() \
      RestoreRadio((s_radio *) &radio, radiocount); \
   }


void AddCombo(HWND hWnd, int id, char *str, char *val);
void  InitRestore(HWND hWnd, char *params);
char *RestoreItem(int id);
char *RestoreCheck(int id, char *value);
char *RestoreCombo(int id);
char *RestoreRadio(s_radio *radio, int count);
int   RestoreInt();
char *RestoreStr();
void  EndRestore();


#define BEGIN_RESTORE(HWND, PARAMS) \
   InitRestore(HWND, PARAMS);

#define END_RESTORE() \
   EndRestore();



void InitSave(HWND hWnd, char *params, int maxLength);
void SaveStr(char *param);
void SaveInt(long val);
void SaveItem(int id);
void SaveCheck(int id, char *val);
void SaveCombo(int id);
void SaveRadio(s_radio *radio, int count);
void EndSave();

#define BEGIN_SAVE() \
   InitSave(hWnd, params);

#define END_SAVE() \
   EndSave();



char *GetStringEnd(char *buf);
int GetDlgInt(HWND hWnd, int id);

char *SkipWhiteSpace(char *data);
void TrimWhiteSpace(char *data);

char *FindSeparator(char *string, char separator);

int Unescape(char *string);
int Escape(char *out, char *in, char escapeChar);
int EscapeBuf(char *string, char escapeChar);

char *mstrchr(char *str, char chr);
char *mstrrchr(char *str, char chr);

char *mstrstr(char *string, char *match);
char *mstrstri(char *string, char *match);
char *mstrtok (char *string, char *match);

void mmemcpy(void *dest, void const *src, int len);
char *mstrdup (const char *string);

void mltoa(long n, char *str);
long matol(char *str);

int mmemcmp(void *dest, void const *src, int len);

char *NextLine(char *data);
char *SkipComments(char *data);
