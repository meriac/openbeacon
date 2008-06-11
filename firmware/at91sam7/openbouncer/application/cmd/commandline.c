/***************************************************************
 *
 * OpenBeacon.org - OpenBeacon link layer protocol
 *
 * Copyright 2007 Angelo Cuccato <cuccato@web.de>
 *
 ***************************************************************

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include <AT91SAM7.h>
#include <FreeRTOS.h>

//#include <stdio.h>                    // for sprintf
#include "vsprintf.h"		// for sprintf
#include <string.h>		// include standard C string functions
#include <stdlib.h>		// strtol

#include "commandline.h"
#include "usbio.h"
#include "vt100.h"


#define CMDLINE_BUFFERSIZE		80
// Numbers of History lines
#define CMDLINE_HISTORYSIZE		5

// History commands
typedef enum
{
  CMDLINE_HISTORY_SAVE,
  CMDLINE_HISTORYREV,
  CMDLINE_HISTORY_NEXT
} tCMDLINE_HISTORY;

// Input Buffer
char CmdlineBuffer[CMDLINE_BUFFERSIZE];
// Input Buffer used lenght
unsigned char CmdlineBufferLength;
// Input Buffer Cursor Position
unsigned char CmdlineBufferEditPos;

// History Buffer
char CmdlineHistory[CMDLINE_HISTORYSIZE][CMDLINE_BUFFERSIZE];
// History Buffer position
unsigned char CmdlineHistoryPos;

// Error Output Start String
char *Notice = "cmdline: ";
// Error Output Ende String
char *NotFound = ": command not found\n\r";


// Commandline Menu ItemsArray
tCmdlineMenu *CmdlineMenuItem;
// Commandline Menucount
unsigned char CmdlineMenuAnzahl;
// MenuPrompt
char CmdlineMenuPrompt[10];


/* **********************************************************************
 *	Print Command Prompt (Public)
 ********************************************************************* */
void
cmdlinePrintPrompt (void)
{
  DumpStringToUSB (CmdlineMenuPrompt);
}


/* **********************************************************************
 *	Refreach Commandline  (Public)
 ********************************************************************* */
void
cmdlineRepaint (void)
{
  char *ptr;
  unsigned char i;

  vSendByte (ASCII_CR);
  cmdlinePrintPrompt ();
  vt100EraseToLineEnd ();

  // print the new command line buffer
  i = CmdlineBufferLength;
  ptr = CmdlineBuffer;
  while (i--)
    vSendByte (*ptr++);

  // set cursor
  i = CmdlineBufferLength - CmdlineBufferEditPos;
  while (i--)
    vt100MoveCursorLeft ();
}


/* **********************************************************************
 *	Print Command Error (Private)
 ********************************************************************* */
void
cmdlinePrintError (void)
{
  char *ptr;

  DumpStringToUSB (Notice);

  // print the offending command
  ptr = CmdlineBuffer;
  while ((*ptr) && (*ptr != ' '))
    vSendByte (*ptr++);

  DumpStringToUSB (NotFound);
}


/* **********************************************************************
 *	Init Command Line  (Public)
 ********************************************************************* */
void
cmdlineInit (void)
{
  CmdlineBufferLength = 0;
  CmdlineBufferEditPos = 0;
  CmdlineHistoryPos = CMDLINE_HISTORYSIZE;
  //cmdlinePrintPrompt();
}


/* **********************************************************************
 *	decode the normal input (Private).
 *	a normal input is as input without spezialkeys. 
 ********************************************************************* */
void
cmdlineNormalChar (char c)
{
  unsigned char i;

  if (CmdlineBufferLength == (CMDLINE_BUFFERSIZE - 1))
    {
      vSendByte (ASCII_BEL);
    }
  else if (CmdlineBufferEditPos == CmdlineBufferLength)
    {
      vSendByte (c);
      CmdlineBuffer[CmdlineBufferEditPos++] = c;
      CmdlineBufferLength++;
    }
  else
    {
      for (i = CmdlineBufferLength; i >= CmdlineBufferEditPos; i--)
	CmdlineBuffer[i + 1] = CmdlineBuffer[i];
      CmdlineBuffer[CmdlineBufferEditPos++] = c;
      CmdlineBufferLength++;
      cmdlineRepaint ();
    }
}


/* **********************************************************************
 * Delte a caracter befor the Cursor (Private).
 ********************************************************************* */
void
cmdlineBS_deleteBack (void)
{
  unsigned char i;

  if (CmdlineBufferEditPos == 0)
    {
      vSendByte (ASCII_BEL);
      return;
    }

  if (CmdlineBufferEditPos == CmdlineBufferLength)
    {
      vSendByte (ASCII_BS);
      CmdlineBufferEditPos--;
      CmdlineBufferLength--;
      vt100EraseToLineEnd ();
    }
  else
    {
      for (i = CmdlineBufferEditPos; i < CmdlineBufferLength; i++)
	CmdlineBuffer[i - 1] = CmdlineBuffer[i];
      CmdlineBufferEditPos--;
      CmdlineBufferLength--;
      cmdlineRepaint ();
    }
}


/* **********************************************************************
 * Delete the caracter under the Cursor (Private).
 ***********************************************************************/
void
cmdlineDEL_deleteForword (void)
{
  unsigned char i;

  if (CmdlineBufferEditPos == CmdlineBufferLength)
    {
      vSendByte (ASCII_BEL);
      return;
    }

  if (CmdlineBufferEditPos < CmdlineBufferLength)
    {
      for (i = CmdlineBufferEditPos + 1; i < CmdlineBufferLength; i++)
	CmdlineBuffer[i - 1] = CmdlineBuffer[i];
      CmdlineBufferLength--;
      cmdlineRepaint ();
    }
}


/* **********************************************************************
 * Historymanager (Private).
 ***********************************************************************/
void
cmdlineDoHistory (tCMDLINE_HISTORY action)
{
  switch (action)
    {
    case CMDLINE_HISTORY_SAVE:
      // copy CmdlineBuffer to history if not null string
      if (strlen ((char *) CmdlineBuffer))
	{
	  // copy old lines
	  for (CmdlineHistoryPos = 1;
	       CmdlineHistoryPos < CMDLINE_HISTORYSIZE; CmdlineHistoryPos++)
	    strcpy ((char *) CmdlineHistory[CmdlineHistoryPos - 1],
		    (char *) CmdlineHistory[CmdlineHistoryPos]);
	  // save new line
	  CmdlineHistoryPos = CMDLINE_HISTORYSIZE - 1;
	  strcpy ((char *) CmdlineHistory[CmdlineHistoryPos++],
		  (char *) CmdlineBuffer);
	}
      break;
    case CMDLINE_HISTORYREV:
      // count CmdlineHistoryPos one down and test the pos
      if (CmdlineHistoryPos > 0)
	{
	  --CmdlineHistoryPos;
	  // copy history to current buffer
	  strcpy ((char *) CmdlineBuffer,
		  (char *) CmdlineHistory[CmdlineHistoryPos]);
	  // set new length
	  CmdlineBufferLength = strlen ((char *) CmdlineBuffer);
	  CmdlineBufferEditPos = CmdlineBufferLength;
	  // "re-paint" line
	  cmdlineRepaint ();
	}
      else
	{
	  // else, ring the bell
	  vSendByte (ASCII_BEL);
	}
      break;
    case CMDLINE_HISTORY_NEXT:
      CmdlineHistoryPos++;
      // count CmdlineHistoryPos one up and test the pos
      if (CmdlineHistoryPos < CMDLINE_HISTORYSIZE)
	{
	  // copy history to current buffer
	  strcpy ((char *) CmdlineBuffer,
		  (char *) CmdlineHistory[CmdlineHistoryPos]);
	  // set new length
	  CmdlineBufferLength = strlen ((char *) CmdlineBuffer);
	  CmdlineBufferEditPos = CmdlineBufferLength;
	  cmdlineRepaint ();
	}
      else
	{
	  // empty line
	  CmdlineBufferLength = 0;
	  CmdlineBufferEditPos = 0;
	  cmdlineRepaint ();
	  vSendByte (ASCII_BEL);
	}
      break;
    }
}


/* **********************************************************************
 * Decode Input (Private).
 ***********************************************************************/
void
cmdlineCR_Enter (void)
{
  unsigned char CommandEnd = 0;
  unsigned char cmdIndex;
  CmdlineFuncPtrType CmdlineExecFunction = 0;

  DumpStringToUSB (("\n\r"));

  // close String
  CmdlineBuffer[CmdlineBufferLength] = 0;

  // Command Ende suchen
  while (!((CmdlineBuffer[CommandEnd] == ' ') ||
	   (CmdlineBuffer[CommandEnd] == 0)))
    CommandEnd++;

  // no Input Eingabe  identified
  if (!CommandEnd)
    {
      cmdlinePrintPrompt ();
      return;
    }

  cmdlineDoHistory (CMDLINE_HISTORY_SAVE);

  // Command search
  for (cmdIndex = 0; cmdIndex < CmdlineMenuAnzahl; cmdIndex++)
    {
      if (!strncmp
	  (CmdlineBuffer, CmdlineMenuItem[cmdIndex].Command, CommandEnd))
	{
	  CmdlineExecFunction = CmdlineMenuItem[cmdIndex].Function;
	  break;
	}
    }

  // Command found
  if (CmdlineExecFunction)
    CmdlineExecFunction ();
  else
    cmdlinePrintError ();

  // var reset
  CmdlineBufferEditPos = 0;
  CmdlineBufferLength = 0;

  cmdlinePrintPrompt ();
}


/* **********************************************************************
 * Make a TAB compleation (Private)
 ***********************************************************************/
void
cmdlineTABcompletion (void)
{
  unsigned char cmdIndex;
  unsigned char CommandMatchCount = 0;
  unsigned char CommandMatchCount2;
  unsigned char i;
  unsigned char ErsterEintrag = 0;
  unsigned char LetzterEintrag = 0;
  char Temp;

  // 1. Count Numbers of match commands
  for (cmdIndex = 0; cmdIndex < CmdlineMenuAnzahl; cmdIndex++)
    {
      if (!strncmp
	  (CmdlineBuffer, CmdlineMenuItem[cmdIndex].Command,
	   CmdlineBufferEditPos))
	{
	  if (CommandMatchCount == 0)
	    ErsterEintrag = cmdIndex;
	  else
	    LetzterEintrag = cmdIndex;
	  CommandMatchCount++;
	}
    }

  // 2. check for found nothing
  if (CommandMatchCount == 0)
    {
      // nothing fond, ring the bell
      vSendByte (ASCII_BEL);
      return;
    }

  // 3. found one, show this
  else if (CommandMatchCount == 1)
    {
      for (cmdIndex = 0; cmdIndex < CmdlineMenuAnzahl; cmdIndex++)
	{
	  if (!strncmp
	      (CmdlineBuffer, CmdlineMenuItem[cmdIndex].Command,
	       CmdlineBufferEditPos))
	    {
	      strcpy (CmdlineBuffer, CmdlineMenuItem[cmdIndex].Command);
	      CmdlineBufferLength =
		strlen (CmdlineMenuItem[cmdIndex].Command);
	      CmdlineBuffer[CmdlineBufferLength] = ' ';
	      CmdlineBufferLength++;
	      CmdlineBufferEditPos = CmdlineBufferLength;
	      cmdlineRepaint ();
	      return;
	    }
	}
    }

  // 4. search in commandlist for matches to complete entered command
  do
    {
      CommandMatchCount2 = 1;
      Temp = CmdlineMenuItem[ErsterEintrag].Command[CmdlineBufferEditPos];
      for (cmdIndex = ErsterEintrag + 1;
	   cmdIndex <= LetzterEintrag; cmdIndex++)
	{
	  if (Temp == CmdlineMenuItem[cmdIndex].Command[CmdlineBufferEditPos])
	    {
	      CommandMatchCount2++;
	    }
	}
      if (CommandMatchCount2 >= CommandMatchCount)
	{
	  if (CmdlineBufferEditPos == CmdlineBufferLength)
	    {
	      // echo character to the output
	      vSendByte (Temp);
	      // add it to the command line buffer
	      CmdlineBuffer[CmdlineBufferEditPos++] = Temp;
	      // update buffer length
	      CmdlineBufferLength++;
	    }
	  else
	    {
	      // edit/cursor position != end of buffer
	      // we're inserting characters at a mid-line edit position
	      // make room at the insert point
	      CmdlineBufferLength++;
	      for (i = CmdlineBufferLength; i > CmdlineBufferEditPos; i--)
		CmdlineBuffer[i] = CmdlineBuffer[i - 1];
	      // insert character
	      CmdlineBuffer[CmdlineBufferEditPos++] = Temp;
	      // repaint
	      cmdlineRepaint ();
	      // reposition cursor
	      for (i = CmdlineBufferEditPos; i < CmdlineBufferLength; i++)
		vSendByte (ASCII_BS);
	    }
	}
    }
  while (CommandMatchCount2 >= CommandMatchCount);

  // 5. search in commandlist for match with entered command
  DumpStringToUSB ("\n\r");
  Temp = 0;
  for (cmdIndex = 0; cmdIndex < CmdlineMenuAnzahl; cmdIndex++)
    {
      if (!strncmp
	  (CmdlineBuffer, CmdlineMenuItem[cmdIndex].Command,
	   CmdlineBufferEditPos))
	{
	  // show command
	  DumpStringToUSB (CmdlineMenuItem[cmdIndex].Command);
	  if (Temp++ % 2)
	    {
	      DumpStringToUSB ("\n\r");
	    }
	  else
	    {
	      for (i = strlen (CmdlineMenuItem[cmdIndex].Command);
		   i <= CMDLINE_MAX_CMD_LENGTH; i++)
		{
		  DumpStringToUSB (" ");
		}
	    }
	}
    }
  DumpStringToUSB ("\n\r");
  cmdlineRepaint ();
}


/* **********************************************************************
 * Move Cursor left (Private).
 ***********************************************************************/
void
cmdlineCursorLeft (void)
{
  if (CmdlineBufferEditPos)
    {
      CmdlineBufferEditPos--;
      vt100MoveCursorLeft ();
    }
  else
    vSendByte (ASCII_BEL);
}


/* **********************************************************************
 * Move Cursor right (Private).
 ***********************************************************************/
void
cmdlineCursorRight (void)
{
  if (CmdlineBufferEditPos < CmdlineBufferLength)
    {
      CmdlineBufferEditPos++;
      vt100MoveCursorRight ();
    }
  else
    vSendByte (ASCII_BEL);
}


/* **********************************************************************
 * Move Cursor to start of line (Private).
 ***********************************************************************/
void
cmdlineCursorStart (void)
{
  CmdlineBufferEditPos = 0;
  cmdlineRepaint ();
}


/* **********************************************************************
 * Move Cursor to end of line (Private).
 ***********************************************************************/
void
cmdlineCursorEnde (void)
{
  CmdlineBufferEditPos = CmdlineBufferLength;
  cmdlineRepaint ();
}


/* **********************************************************************
 * decode user input caracter (Public)
 ***********************************************************************/
void
cmdlineDecodeInput (char c)
{
  enum enStatus
  { Default,
    Sonder1,
    Sonder2
  };
  static enum enStatus InputStatus = Default;

  if (InputStatus == Default)
    {
      // normal input (no arrow keys )
      if ((c >= 0x20) && (c <= 0x7E))	// Normal Key?
	cmdlineNormalChar (c);
      else if (c == ASCII_ESC)	// 1B Hex       ESC startet vt100
	InputStatus = Sonder1;
      else if (c == ASCII_BS)	// 08 Hex       Backspace
	cmdlineBS_deleteBack ();
      else if (c == ASCII_DEL)	// 7F Hex       Entf
	cmdlineDEL_deleteForword ();
      else if (c == ASCII_CR)	// 13 Hex       ENTER
	cmdlineCR_Enter ();
      else if (c == ASCII_POS1)	// 32 Hex       Pos1
	cmdlineCursorStart ();
      else if (c == ASCII_END)	// 15 Hex       End
	cmdlineCursorEnde ();
      else if (c == ASCII_TAB)	// 07 Hex       Tab
	cmdlineTABcompletion ();
    }
  else if (InputStatus == Sonder1)
    {
      if (c == '[')		// 5B Hex
	InputStatus = Sonder2;
      else
	InputStatus = Default;
    }
  else if (InputStatus == Sonder2)
    {
      // arrow keys 
      switch (c)
	{
	case VT100_ARROWUP:	// History up
	  cmdlineDoHistory (CMDLINE_HISTORYREV);
	  break;
	case VT100_ARROWDOWN:	// History down
	  cmdlineDoHistory (CMDLINE_HISTORY_NEXT);
	  break;
	case VT100_ARROWRIGHT:	// Cursor right
	  cmdlineCursorRight ();
	  break;
	case VT100_ARROWLEFT:	// Cursor left
	  cmdlineCursorLeft ();
	  break;
	case VT100_POS1:	// start of line
	  cmdlineCursorStart ();
	  break;
	case VT100_END:	//      end of line
	  cmdlineCursorEnde ();
	  break;

	}
      InputStatus = Default;
    }

  // debug Ascii output
  //char Text_Buffer[10];         sprintf( Text_Buffer, ("#%2X\n\r"), c);         DumpStringToUSB( Text_Buffer);
}



/* **********************************************************************
 * seach Startadress of parameter (Public)
 ***********************************************************************/
char *
cmdlineGetArgStr (unsigned char argnum)
{
  // find the offset of argument number [argnum]
  unsigned char idx = 0;
  unsigned char arg;

  // find the first non-whitespace character
  while ((CmdlineBuffer[idx] != 0) && (CmdlineBuffer[idx] == ' '))
    idx++;

  // we are at the first argument
  for (arg = 0; arg < argnum; arg++)
    {
      // find the next whitespace character
      while ((CmdlineBuffer[idx] != 0) && (CmdlineBuffer[idx] != ' '))
	idx++;
      // find the first non-whitespace character
      while ((CmdlineBuffer[idx] != 0) && (CmdlineBuffer[idx] == ' '))
	idx++;
    }
  // we are at the requested argument or the end of the buffer
  return &CmdlineBuffer[idx];
}


/* **********************************************************************
 * Returned Parameter decode as Long. (Public)
 ***********************************************************************/
long
cmdlineGetArgInt (unsigned char argnum)
{
  char *endptr;
  return strtol ((char *) cmdlineGetArgStr (argnum), &endptr, 10);
}


/* **********************************************************************
 * Returned Parameter Hexwert as Long. (Public)
 ***********************************************************************/
long
cmdlineGetArgHex (unsigned char argnum)
{
  char *endptr;
  return strtol ((char *) cmdlineGetArgStr (argnum), &endptr, 16);
}
