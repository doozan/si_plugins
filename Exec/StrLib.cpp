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

#include "StrLib.h"

HWND ghWndRestore = 0; 
char *posRestore = 0;
char posCh;           // the character that was nulled
char *posNulled=0;    // the position of the character nulled
char *posHeadEscaped; // if the restored string was escaped

void InitRestore(HWND hWnd, char *params) {
   if (posHeadEscaped) {
      EscapeBuf(posHeadEscaped, '"');
      posHeadEscaped = NULL;
   }
   if (posNulled) {
      *posNulled = posCh;
      posNulled = NULL;
   }

   ghWndRestore = hWnd;
   posRestore = params;
   posNulled = NULL;
   posCh = NULL;
   posHeadEscaped = NULL;
}

void  EndRestore() {
   if (posHeadEscaped) {
      EscapeBuf(posHeadEscaped, '"');
      posHeadEscaped = NULL;
   }
   if (posNulled) {
      *posNulled = posCh;
      posNulled = NULL;
   }

   ghWndRestore = NULL;
   posRestore = NULL;
   posNulled = NULL;
   posCh = NULL;
   posHeadEscaped = NULL;
}

// the string returned is only valid until the next call
// to a restore function
char *RestoreItem(int id) {
   char *str = RestoreStr();
   SetDlgItemText(ghWndRestore, id, str);
   return str;
}


// the string returned is only valid until the next call
// to a restore function
char *RestoreCheck(int id, char *value) {

   char *str = RestoreStr();

   if (!lstrcmpi(str, value))
      CheckDlgButton(ghWndRestore, id, BST_CHECKED);
   else
      CheckDlgButton(ghWndRestore, id, BST_UNCHECKED);

   return str;
}

char *RestoreRadio(s_radio *radio, int count) {

   char *str = RestoreStr();

   int start = radio[0].id;
   int end = radio[count-1].id;

   while (count--) {
      if (!lstrcmpi(radio[count].val, str)) {
         CheckRadioButton(ghWndRestore, start, end, radio[count].id);
         return str;
      }
   }

   return str;

}



// the string returned is only valid until the next call
// to a restore function
char *RestoreCombo(int id) {

   char *str = RestoreStr();

   int count = SendDlgItemMessage(ghWndRestore, id, CB_GETCOUNT, 0, 0);
   int sel = -1;
   int x = 0;
   while (x<count && sel==-1) {
      char *val = (char *) SendDlgItemMessage(ghWndRestore, id, CB_GETITEMDATA, x, 0);
      if (val && !lstrcmpi(val, str))
         sel = x;
      x++;
   }
   
   // it wasn't found in the item data, check the strings
   if (sel == -1) {
      x = 0;
      char buf[1024];
      while (x<count && sel==-1) {
         SendDlgItemMessage(ghWndRestore, id, CB_GETLBTEXT, x, (LPARAM) buf);
         if (buf[0] && !lstrcmpi(buf, str))
            sel = x;
         x++;
      }
   }

   if (sel != -1)
      SendDlgItemMessage(ghWndRestore, id, CB_SETCURSEL, sel, 0);

   return str;
}


int RestoreInt() {

   if (posHeadEscaped) {
      EscapeBuf(posHeadEscaped, '"');
      posHeadEscaped = NULL;
   }
   if (posNulled) {
      *posNulled = posCh;
      posNulled = NULL;
   }

   if (!posRestore)
      return 0;

   posRestore = SkipWhiteSpace(posRestore);
   BOOL bQuotes = FALSE;
   if (*posRestore == '\"') {
      bQuotes = TRUE;
      posRestore++;
   }

   int val = 0;

   while (*posRestore && *posRestore >= '0' && *posRestore <= '9') {
      val *= 10;
      val += *posRestore - '0';
      posRestore++;
   }

   if (bQuotes && *posRestore)
      posRestore++;

   return val;
}



// the string returned is only valid until the next call
// to a restore function

char *RestoreStr() {

   if (posHeadEscaped) {
      EscapeBuf(posHeadEscaped, '"');
      posHeadEscaped = NULL;
   }
   if (posNulled) {
      *posNulled = posCh;
      posNulled = NULL;
   }

   if (!posRestore)
      return NULL;

   posRestore = SkipWhiteSpace(posRestore);
   char *end = GetStringEnd(posRestore);

   BOOL bQuotes = FALSE;
   if (*posRestore == '"') {
      // count the number of \'s preceeding the quote
      // if it's an odd number, then the final \ would
      // escape the ", meaning it's not the end quote
      int x=1;
      while ( *(end-1-x) == '\\')
         x++;
      x--;
      
      if (!(x & 1)) {  // if it's even, the " is valid
         posRestore++;
         end--;
         bQuotes = TRUE;
      }
   }
   
   posCh = *end;
   posNulled = end;
   *end = 0;
  

   // if there were characters that were unescaped,
   // we need to remember the beginning of the string, so
   // that we can restore the escaped data after we're done
   if (Unescape(posRestore))
      posHeadEscaped =  posRestore;
   else
      posHeadEscaped = NULL;

   char *ret = posRestore;

   posRestore = end+1;

   return ret;
}





HWND  ghWndSave;
char *posSave = 0;
int   posBytesLeft = 0; // space left in buffer

void InitSave(HWND hWnd, char *params, int maxLength) {
   ghWndSave = hWnd;
   *params = 0;
   posSave = params;
   posBytesLeft = maxLength-1;   // -1 for the null terminator
}

void EndSave() {
   ghWndSave = NULL;
   posSave = NULL;
   posBytesLeft = 0;
}

void SaveStr(char *param) {
   if ( (posBytesLeft <= 3) || !posSave)
      return;

   posBytesLeft -= 3;  // subtract 3 for the 2 quotation marks and the space

   if (*posSave == 0)
      *posSave++ = ' ';

   int len = lstrlen(param);
   
   *posSave++ = '"';
   len += Escape(posSave, param, '"');
   posSave += len;

   *posSave++ = '"';
   *posSave   = 0;
   posBytesLeft -= len;
}


void SaveInt(long val) {
   if (!posBytesLeft || !posSave)
      return;

   posBytesLeft -= 3;  // subtract 3 for the 2 quotation marks and the space

   if (*posSave == 0)
      *posSave++ = ' ';

   *posSave++ = '"';
   mltoa(val, posSave);
   int len = lstrlen(posSave);
   posSave += len;

   *posSave++ = '"';
   *posSave   = 0;
   posBytesLeft -= len;
}

void SaveItem(int id) {   
   if ( (posBytesLeft <= 3) || !posSave)
      return;

   posBytesLeft -= 3;  // subtract 3 for the 2 quotation marks and the space

   if (*posSave == 0)
      *posSave++ = ' ';

   *posSave++ = '"';
   int len = GetDlgItemText(ghWndSave, id, posSave, posBytesLeft);
   len += EscapeBuf(posSave, '"');
   posSave += len;

   *posSave++ = '"';
   *posSave   = 0;
   posBytesLeft -= len;
}

void SaveCombo(int id) {   
   if ( (posBytesLeft <= 3) || !posSave)
      return;

   posBytesLeft -= 3;  // subtract 3 for the 2 quotation marks and the space

   if (*posSave == 0)
      *posSave++ = ' ';

   *posSave++ = '"';
   *posSave = '\0';

   int len = 0;
   int sel = SendDlgItemMessage(ghWndSave, id, CB_GETCURSEL, 0, 0);
   if (sel != CB_ERR)
   {
     char *name = (char *) SendDlgItemMessage(ghWndSave, id, CB_GETITEMDATA, sel, 0);
     if (name) {
        len = lstrlen(name);
        lstrcpy(posSave, name);
     }
     else
        len = SendDlgItemMessage(ghWndSave, id, CB_GETLBTEXT, sel, (LPARAM) posSave);
   }

   len += EscapeBuf(posSave, '"');
   posSave += len;

   *posSave++ = '"';
   *posSave = '\0';
   posBytesLeft -= len;
}

void SaveCheck(int id, char *val) {

   if (IsDlgButtonChecked(ghWndSave, id))
      SaveStr(val);
   else
      SaveStr("");

}

void SaveRadio(s_radio *radio, int count) {

   while (count--) {
      if (IsDlgButtonChecked(ghWndSave, radio[count].id)) {
         SaveStr(radio[count].val);
         return;
      }
   }
   SaveStr("");

}



void AddCombo(HWND hWnd, int id, char *string, char *val) {
   int cur = SendDlgItemMessage(hWnd, id, CB_ADDSTRING, 0, (LPARAM) string);
   if (cur != CB_ERR)
      SendDlgItemMessage(hWnd, id, CB_SETITEMDATA, cur, (LPARAM) val);
}



// returns a pointer to the end of the current string
// if the string passed to it begins with a quotation mark
// it determines the end of the string as being the first
// unescaped quotation mark,  Otherwise, the end of the string
// is the first whitespace

char *GetStringEnd(char *buf) {
   if (!buf)
      return NULL;

   char *end = buf;

   if (*buf == '"') {    // if it's a string in quotations
      end++; // skip past opening quotation mark

      while (*end) {
         if (*end == '"') {

            // count the number of \'s preceeding the quote
            // if it's an odd number, then the final \ would
            // escape the ", meaning it's not the end quote
            int x=1;
            while ( *(end-x) == '\\')
               x++;
            x--;

            if (!(x & 1))   // if it's even, the " is valid
               break;
         }
         end++;
      }
      if (*end)
         end++;
   }
   else    // it's not in quotations
      while (*end && *end != ' ' && *end != '\t') end++;

   return end;
}

int GetDlgInt(HWND hWnd, int id) {
   char buf[11];
   GetDlgItemText(hWnd, id, buf, 10);

   int val = 0;
   char *p = buf;
   while (*p && *p >= '0' && *p <= '9') {
      val *= 10;
      val += *p - '0';
      p++;
   }

   return val;
}


char *SkipWhiteSpace(char *data) {
   if (data)
      while (*data && (*data == ' ' || *data == '\t'))   // skip whitespace
         data++;
   return data;
}

void TrimWhiteSpace(char *data) {
   if (!data)
      return;

   char *p = data + lstrlen(data)-1;

   while (p >= data && (*p == ' ' || *p == '\t')) p--;

   *(p+1) = 0;
}


char *FindSeparator(char *string, char separator) {
   while (*string) {
      if (*string == separator && *(string-1) != '\\')
         return string;
      string++;
   }
   return NULL;
}


// return the number of characters removed from the string
int Unescape(char *string) {
   char *in, *out;
   in = out = string;
   int count = 0;

   while (*in) {
      if (*in == '\\' && *(in+1)) {
         switch (*(in+1)) {
         case '\"':
            *out++ = '\"';
            count++;
            break;
         case 't':
            *out++ = '\t';
            count++;
            break;
         case 'n':
            *out++ = '\n';
            *out++ = '\r';
            break;
         default:
            *out++ = *(in+1);
            count++;
            break;
         }
         in+=2;
      }
      else
         *out++ = *in++;
   }
   *out = 0;
   return count;
}

int Escape(char *out, char *in, char escapeChar) {
   int escaped = 0;

   while (*in) {
      if (*in == escapeChar || *in == '\\') {
         if (out) {
            *out = '\\';
            out++;
         }
         escaped ++;
      }
      if (out) {
         *out++ = *in;
      }
      in++;
   }
   if (out)
      *out = 0;

   return escaped;
}

// this function assumes that you've got room to expand the string passed to it
int EscapeBuf(char *string, char escapeChar) {
   if (!string)
      return 0;

   int escaped = 0;

   while (*string) {
      if (*string == escapeChar || *string == '\\') {
         int len = lstrlen(string);
         char *ch = string + len;
         while (ch >= string) {
            *(ch+1) = *ch;
            ch--;
         }

         *string++ = '\\';
         escaped++;
      }
      string++;
   }

   return escaped;
}


char *mstrchr(char *str, char chr) {
   while (*str) {
      if (*str==chr)
         return str;
      str++;
   }

   return NULL;
}

char *mstrrchr(char *str, char chr) {
   char *start = str;
   str = str+lstrlen(str);
   while (str>=str) {
      if (*str==chr)
         return str;
      str--;
   }

   return NULL;

}


char *mstrstr(char *string, char *match) {
   int slen = lstrlen(string);
   int mlen = lstrlen(match);
   char *end = string+slen-mlen;

   char temp;
   for (char *p=string,*pend=string+mlen;p<=end;p++,pend++) {
      temp = *pend;
      *pend = 0;
      if (!lstrcmp(p, match)) {
         *pend = temp;
         return p;
      }
      *pend = temp;      
   }

   return NULL;
}

char *mstrstri(char *string, char *match) {
   int slen = lstrlen(string);
   int mlen = lstrlen(match);
   char *end = string+slen-mlen;

   char temp;
   for (char *p=string,*pend=string+mlen;p<=end;p++,pend++) {
      temp = *pend;
      *pend = 0;
      if (!lstrcmpi(p, match)) {
         *pend = temp;
         return p;
      }
      *pend = temp;      
   }

   return NULL;
}



char *toknext;
char *mstrtok (char *string, char *match) {
   if (string)
      toknext = string;

   char *tok;
   if ( (tok = mstrstr(toknext, match)) ) {
      *tok = 0;
      char *ret = toknext;
      toknext = tok + lstrlen(match);
      return ret;
   }
   else return 0;
}

char *mstrdup (const char *string) {
  if (!string) return NULL;

  char *ret = new char[lstrlen(string) +1];
  lstrcpy(ret, string);
  return ret;
}



// recursion...fun
static char *s;
static void num_to_chr(unsigned long n) {
    unsigned long q;
    if (q = n / 10)
        num_to_chr(q);
    *s++ = (char) n % 10 + '0';
}

void mltoa(long n, char *str) {
   s = str;
   if (n < 0) {
      n = -n;
      *s++ = '-';
   }
   num_to_chr( (unsigned long) n);
   *s = 0;
}





long matol(char *str) {
   long val = 0;
   // hexidecimal
   if (str[1] == 'x') {
      str += 2;               
      
      while (*str) {                                    
         if (*str >= '0' && *str <= '9') {
            val <<= 4;  // multiply by 16
            val += *str-'0'; // add the integer value
         }
         else if (*str >= 'a' && *str <= 'f') {
            val <<= 4;  // multiply by 16
            val += *str-'a'+10; // add the integer value
         }
         else if (*str >= 'A' && *str <= 'F') {
            val <<= 4;
            val += *str-'A'+10;
         }
         else 
            break; // not valid input, get outta here
         str++;
      }
   }
   // decimal
   else {
      while (*str >= '0' && *str <= '9') {
         val *= 10;
         val += *str-'0';
         str++;
      }
   }

   return val;
}

void mmemcpy(void *dest, void const *src, int len) {
   if (len>0) {
      do {
         *(char *) dest = *(char *) src;
         dest = (char *) dest+1;
         src = (char *) src+1;
      } while (--len);
   }
}

int mmemcmp(void *dest, void const *src, int len) {
   while (len-- > 0) {
      if ( *(char *)src != *(char *)dest)
         return (* (char *) src) - (* (char *) dest);
      else {
         dest = (char *) dest+1;
         src = (char *) src+1;
      }
   }
   return 0;
}

// skip commented and blank lines
char *SkipComments(char *data) {
   while (*data == '#' || *data == '\r')
      data = NextLine(data);
   return data;
}

char *NextLine(char *data) {
   while (*data && *data != '\r') data++;    // find next eol
   return (*data) ? data+2 : NULL;              // +2 to skip over \r\n
}

