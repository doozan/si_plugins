Declaring Commands
==================

Plugins can provide multiple commands.  The simplest way to declare a command is with the
DECLARE_COMMAND(name) statement.  "name" is the internal name of your command, which should
be human readable and can only consist of alphanumeric characters plus the underscore "_".

Example:

DECLARE_COMMAND(mycmd);


Initializing Commands
=====================

All commands must be initialized the INIT() function.

INIT_COMMMAND(mycmd, name, description, icon);

mycmd        - the internal name of your command and must match what you used in DECLARE_COMMAND
name         - the "friendly name" of the command that will be displayed to the user
description  - a brief description of what the command does that will be displayed to the user
icon         - integer id of the icon resource to use when displaying this command







Executing commands
==================
When a command is executed it will call the matching EXEC command.

Example:

EXEC(mycmd)
{
  // this is where we do stuff
  
  EXEC_RETURN(NULL);
}


If your command needs to save data, it can save a pointer with the EXEC_RETURN
function.  You can retrieve this pointer on subsequent calls to EXEC using
EXEC_GET_DATA();

Example:

struct mycmd_data {
   int iRunCount;
};

EXEC(mycmd)
{
  mycmd_data *pData = EXEC_GET_DATA();

  // this is the first time the command has run, create mycmd_data
  if (!pData)
  {
    pData = new mycmd_data;
    pData->iRunCount = 0;
  }
  
  pData->iRunCount++;
  
  EXEC_RETURN(pData);
}


Command Cleanup
===============

The CLEAR() command is called whenever you command should delete any saved data.
This is usually called when the program is closing or when the configuration
parameters have changed.

Example:

CLEAR(mycmd)
{
  mycmd_data *pData = CLEAR_GET_DATA();
  
  // delete our saved data
  delete pData;
  
  CLEAR_RETURN();
}




Displaying the configuration
============================

Most commands will have a configuration page where the user can set
options for the commands.

the SHOW() command is called whenever your command should show it's
configuration dialog.

Example:

SHOW(mycmd)
{
  HWND hWnd = SHOW_DIALOG(DIALOG_MYCMD, DlgProc);
  SHOW_RETURN();
}

The above example will create and show the DIALOG_MYCMD dialog. However, it does not
initialize any strings or restore any parameters so you would never use that for a
real command.

The following example does two additional import things:

  First, it initialize the label "IDC_LABEL_PARAM1" with a descriptive label from the strings table.
  This allows international users to transate your plugin without making any changes to your code.
  
  Second, it restores the first saved parameter into the edit dialog IDC_EDIT_PARAM1.  This is important
  because it will show users what existing data is "saved" for this command.

SHOW(mycmd)
{
  HWND hWnd = SHOW_DIALOG(DIALOG_MYCMD, DlgProc);

  SET( IDC_LABEL_PARAM1, STR(MYCMD_PARAM1) );
  RestoreItem( IDC_EDIT_PARAM1 );

  SHOW_RETURN();
}


Restore Functions
-----------------

Available Restore functions:

char *RestoreItem(int id);
char *RestoreCheck(int id, char *value);
char *RestoreCombo(int id);
int   RestoreInt();
char *RestoreStr();

RESTORERADIO();



To restore a text box:
----------------------
   RestoreItem(IDC_EDIT_BOX);


To restore a check box:
-----------------------
   RestoreCheck(IDC_CHECK, "checked");

If the parameter matches "checked", the box will be checked.


To restore a combo box:
--------------------
   COMBO(IDC_COMBO, STR(ONE), "one");
   COMBO(IDC_COMBO, STR(TWO), "two");
   COMBO(IDC_COMBO, STR(THREE), "three");
   RestoreCombo(IDC_COMBO);


To restore a radio setting:
---------------------------
   NEWRADIO(3);
   RADIO(IDC_RADIO_ONE, "one");
   RADIO(IDC_RADIO_TWO, "two");
   RADIO(IDC_RADIO_THREE, "three");
   RESTORERADIO();


To restore an exact string:
---------------------------
   RestoreStr(IDC_EDIT_BOX, "string");


To restore an integer:
----------------------
   RestoreInt(IDC_EDIT_BOX, 123);





Saving the configuration
========================

When the user is done configuring your command, the SAVE() function will be called.
This is the time to save the command data.

The following example will save the text in IDC_EDIT_PARAM1

Example:

SAVE(mycmd)
{
  SaveItem( IDC_EDIT_PARAM1 );
  
  SAVE_RETURN();
}


Save Functions
--------------

SaveItem(int id);
SaveCheck(int id, char *val);
SaveCombo(int id);
SAVERADIO();
SaveStr(char *param);
SaveInt(long val);


To save a textbox:
------------------
   SaveItem(IDC_EDIT_BOX);


To save a check box:
--------------------
   SaveCheck(IDC_CHECK, "checked");

"checked" is the value to save if the the box is checked.
If it is not checked, the paramater will be left blank.


To save a combo box:
--------------------
Assuming that the values in the dialog box have been added using AddCombo(),
(see the Restoring Parameters section to see how to do this) you can just call

   SaveCombo(IDC_COMBO);


To save a radio setting:
------------------------
   NEWRADIO(3);                              // we have 3 radio buttons
   RADIO(IDC_RADIO_ONE, "one");
   RADIO(IDC_RADIO_TWO, "two");
   RADIO(IDC_RADIO_THREE, "three");
   SAVERADIO();

The string value passed as the second paramater to ADDRADIO is what will be
saved as the paramater.


To save an exact string:
------------------------
   SaveStr("string");


To save an integer:
-------------------
   SaveInt(123);






Advanced Initialization
=======================

INIT_COMMAND_CUSTOM allows you to override the EXEC, SHOW, SAVE and CLEAR functions that your command
will use. By default your commands will call EXEC, SHOW, SAVE and CLEAR functions with the same name
as the command.

The following commands are functionally equivalent:

INIT_COMMAND       (mycmd, name, description, icon)
INIT_COMMAND_CUSTOM(mycmd, name, description, icon, mycmd, mycmd, mycmd, mycmd)


There are time when you may want to override the default EXEC, SHOW, SAVE, and CLEAR commands,
usually when you have several similar commands that use the same dialogs.

In these cases, you can declare additional EXEC, SHOW, SAVE, and CLEAR functions using

DECLARE_EXEC(myexec);
DECLARE_SHOW(myshow);
DECLARE_SAVE(mysave);
DECLARE_CLEAR(myclear);

To have your command call these functions instead of its default functions, initilize the command
with the INIT_COMMAND_CUSTOM function:

INIT_COMMAND_CUSTOM(mycmd, name, description, icon, myexec, myshow, mysave, myclear);

Note: you don't have to override all of the functions.  For example, if you only want to override
the CLEAR command, you can do the following:

DECLARE_CLEAR(myclear);
INIT_COMMAND_CUSTOM(mycmd, name, description, icon, mycmd, mycmd, mycmd, myclear);



Defining Strings
================

In an effort to make plugins as easily translateable as possible, string
resources are stored in a textfile name <Language>.lng, where <Language> is
the user selected language ("English", by default).

Strings are declared in the file Strings.h, and are simply declare as
shown below:

To declare a string (Strings.h)
----------------------------------
   STR(COMMAND_TEXT)


To define the string text, just add a line to <Language>.lng:

To define a string (English.lng)
--------------------------------
   This is the command text.

NOTE: The strings must be defined in <Language>.lng in the SAME ORDER THAT
THEY WERE DECLARED in StringDefs.h.



Using Strings
=============

Strings can be referenced as STR(STRING_ID), where STRING_ID is the name declared
in Strings.h (COMMAND_TEXT, in the example above)

Since the strings are defined in a language file, you'll need to explicitly set
each string used in your dialogs.  The SET() command makes this process pretty
painless:

To set a dialog item's string
-----------------------------
   SET(IDC_COMMAND, STR(COMMAND_TEXT));

IDC_COMMAND is the resource ID.




Ignoring the Default Libraries
==============================

By default, the project ignores the default libraries, which makes
the resulting dll much smaller (~30k smaller).

Unfortunately, this means that support for common C string managment
functions needs to be recreated.   Most common commands are available through
the Windows API, or functions contained in StrLib.cpp.


Old Function      New Function
============      ============
strcpy            lstrcpy
strlen            lstrlen
sprintf           wsprintf
strchr            mstrchr
strrchr           mstrrchr
strstr            mstrstr
strstri           mstrstri
strtok            mstrtok
strdup            mstrdup
ltoa              mltoa
atol              matol
memcpy            mmemcpy
memcmp            mmemcmp


That should be the most commonly used functions.  If there's anything that's
lacking, you'll either need to code it yourself, or compile in the
default libraries.



