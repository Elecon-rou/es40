/* ES40 emulator.
 * Copyright (C) 2007-2008 by the ES40 Emulator Project
 *
 * WWW    : http://sourceforge.net/projects/es40
 * E-mail : camiel@camicom.com
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 * Although this is not required, the author would appreciate being notified of, 
 * and receiving any modifications you may make to the source code that might serve
 * the general public.
 */

/**
 * \file
 * Contains the code for the configuration file interpreter.
 *
 * $Id: Configurator.cpp,v 1.22 2008/03/04 19:28:08 iamcamiel Exp $
 *
 * X-1.22       David Hittner                                   04-MAR-2008
 *      Allow curly braces inside strings.
 *
 * X-1.21       Camiel Vanderhoeven                             04-MAR-2008
 *      Merged Brian wheeler's New IDE code into the standard controller.
 *
 * X-1.20       Pepito Grillo                                   02-MAR-2008
 *      Avoid compiler warnings.
 *
 * X-1.19       Camiel Vanderhoeven                             02-MAR-2008
 *      Natural way to specify large numeric values ("10M") in the config file.
 *
 * X-1.18       Brian Wheeler                                   27-FEB-2008
 *      Avoid compiler warnings.
 *
 * X-1.17       Brian Wheeler                                   26-FEB-2007
 *      Better syntax checking and error reporting.
 *
 * X-1.16       Camiel Vanderhoeven                             26-FEB-2008
 *      Moved DMA code into it's own class (CDMA)
 *
 * X-1.15       Camiel Vanderhoeven                             16-FEB-2008
 *      Forgot something on last change.
 *
 * X-1.14       Camiel Vanderhoeven                             16-FEB-2008
 *      Added Symbios 53C810 controller.
 *
 * X-1.13       Camiel Vanderhoeven                             12-FEB-2008
 *      Moved keyboard code into it's own class (CKeyboard)
 *
 * X-1.12       Camiel Vanderhoeven                             20-JAN-2008
 *      Added X11 GUI.
 *
 * X-1.11       Camiel Vanderhoeven                             19-JAN-2008
 *      Added win32 GUI.
 *
 * X-1.10       Camiel Vanderhoeven                             09-JAN-2008
 *      Save disk state to state file.
 *
 * X-1.9        Camiel Vanderhoeven                             08-JAN-2008
 *      Use Brian Wheeler's CNewIde class instead of the CAliM1543C_ide
 *      class if HAVE_NEW_IDE is defined. This change will be undone when
 *      the new ide controller will replace the old standard one.
 *
 * X-1.8        Camiel Vanderhoeven                             05-JAN-2008
 *      Added CDiskDevice class.
 *
 * X-1.7        Camiel Vanderhoeven                             02-JAN-2008
 *      Better handling of configuration errors.
 *
 * X-1.6        Camiel Vanderhoeven                             28-DEC-2007
 *      Throw exceptions rather than just exiting when errors occur.
 *
 * X-1.5        Camiel Vanderhoeven                             28-DEC-2007
 *      Keep the compiler happy.
 *
 * X-1.4        Camiel Vanderhoeven                             14-DEC-2007
 *      Add support for Symbios SCSI controller.
 *
 * X-1.3        Camiel Vanderhoeven                             12-DEC-2007
 *      Add support for file- and RAM-disk.
 *
 * X-1.2        Brian Wheeler                                   10-DEC-2007
 *      Better error reporting.
 *
 * X-1.1        Camiel Vanderhoeven                             10-DEC-2007
 *      Initial version in CVS.
 **/

#include "StdAfx.h"
#include "Configurator.h"
#include "System.h"
#include "AlphaCPU.h"
#include "Serial.h"
#include "Flash.h"
#include "DPR.h"
#include "AliM1543C.h"
#include "Keyboard.h"
#include "DMA.h"
#include "AliM1543C_ide.h"
#include "AliM1543C_usb.h"
#include "DiskFile.h"
#include "DiskDevice.h"
#include "DiskRam.h"
#include "Port80.h"
#include "S3Trio64.h"
#include "Cirrus.h"
#if defined(HAVE_RADEON)
#include "Radeon.h"
#endif
#include "gui/plugin.h"
#if defined(HAVE_PCAP)
#include "DEC21143.h"
#endif
#include "Sym53C895.h"
#include "Sym53C810.h"

/**
 * Constructor.
 *
 * The portion of the configuration file that corresponds with the device we are the configurator for
 * is passed as text, with a length of textlen.
 * We parse this text portion, creating values and children configurators as needed. 
 * If parent is NULL, we are the master configurator, and we will call initialize for our children
 * (probably GUI and System) so they can instantiate the classes that correspond to the devices that
 * they configure for. The children will in turn initialize their children.
 *
 * \bug This needs to be more robust! As it is now, this code was more or less "hacked together" in a
 *      few minutes. Also, more comments should be provided to make it more readable.
 **/

CConfigurator::CConfigurator(class CConfigurator * parent, char * name, char * value, char * text, size_t textlen)
{
  if(parent == 0) {
    /* Phase 1:  Basic Syntax Check & Strip [first pass only]
     * - Make sure quotes and comments are closed.
     * - Make sure braces are balanced
     * - remove everything except configuration data.
     */

    char *dst = (char *)malloc(textlen+1);
    char *p = dst;
    char *q = text;

    int cbrace=0;
    enum { STATE_NONE, STATE_C_COMMENT, STATE_CC_COMMENT, 
	   STATE_STRING } state=STATE_NONE;
    int state_start = 0;
    int line = 1;
    int col = 1;
    for(int i = 0; i < textlen; i++, q++, col++) 
    {
      if(*q == 0x0a) 
      {
	line++;
	col=1;
      }
      switch(state) 
      {
      case STATE_NONE:
	switch(*q) {
	case '"':
	  state=STATE_STRING;
	  state_start = line;
	  *p++=*q;
	  break;
	case '/':
	  if(i == textlen - 1) 
	  { 
	    printf("%%SYS-E-PARSE: file ends in mid-token.  Line %d, Col %d\n",
		   line,col);
	    exit(1); 
	  }
	  
	  if(*(q+1) == '/') 
	  {
	    state=STATE_CC_COMMENT;	
	    state_start = line;  
	  } else if(*(q+1) == '*') {
	    state=STATE_C_COMMENT;	  
	    state_start = line;
	  }
	  break;
	case '{':
	  cbrace++;
	  *p++=*q;
	  break;
	case '}':
	  if(cbrace==0) 
	  {
	    printf("%%SYS-E-PARSE: too many closed braces.  Line %d, Col %d\n",
		   line,col);
	    exit(1); 
	  }
	  cbrace--;
	  *p++=*q;
	  break;
	default:
	  if(!isspace(*q)) 
	  {
	    if(isalnum(*q) || *q=='_' || *q=='.' || *q=='=' || *q==';') 
	    {
	      *p++=*q;
	    } else {
	      printf("%%SYS-E-PARSE: Illegal character '%c'.  Line %d, Col %d\n",
		     *q,line,col);
	      exit(1); 
	    }
	  }
	}
	break;
      case STATE_CC_COMMENT: // c++ comment
	if(*q == 0x0d || *q == 0x0a) 
	{
	  state=STATE_NONE;
	  state_start = line;
	}
	break;
      case STATE_C_COMMENT: // c comment
	if(*q == '*') 
	{
	  if(i == textlen - 1) 
	  { 
	    printf("%%SYS-E-PARSE: file ends in mid-comment.  Line %d, Col %d\n",
		   line,col);
	    exit(1); 
	  }
	  if(*(q+1) == '/') 
	  { 
	    state=STATE_NONE;
	    state_start = line;
	  }
	}
	break;
      case STATE_STRING: // string
	if(*q == '"') 
	{
	  if(i == textlen - 1) 
	  { 
	    printf("%%SYS-E-PARSE: file ends in mid-string.  Line %d, Col %d\n",
		   line,col);
	    exit(1); 
	  }
	  if(*(q+1) != '"') 
	  {
	    state_start = line;
	    state=STATE_NONE;
	  }
      else
      {
        *p++ = *q;
        i++;
        q++;
      }
	} else if(*q == 0x0a || *q == 0x0d) 
	{
	  printf("%%SYS-E-PARSE: multi-line strings are forbidden.  Line %d, Col %d\n",
		 line,col);
	  exit(1); 
	}
	*p++=*q;
	break;
      }
    }
    *p++=0;
    
    if(state != 0 && state != 1) 
    {
      printf("%%SYS-E-PARSE: unclosed %s.  Started on line %d.\n",state==STATE_C_COMMENT?"comment":"string",state_start);
    }
    
    if(cbrace != 0) 
    {
      printf("%%SYS-E-PARSE: unclosed brace in file.\n");   
    }

    textlen=strlen(dst);
    memcpy(text,dst,textlen+1);
    free(dst);
  }
    
  /* Phase 2: Parse the file [every time]
   * The data is in a very compressed form at this point.  We also
   * know some important things about the data:
   * - There is no whitespace (except in strings)
   * - the braces are all closed
   * - comments have been removed.
   * - strings are valid.
   */
  enum { STATE_NONE, STATE_NAME, STATE_IS, STATE_VALUE, STATE_QUOTE,
	 STATE_CHILD } state = STATE_NONE;

  char * cur_name;
  char * cur_value;

  int child_depth = 0;

  size_t name_start = 0;
  size_t name_len = 0;

  size_t value_start = 0;
  size_t value_len = 0;

  size_t child_start = 0;
  size_t child_len = 0;

  pParent = parent;
  iNumChildren = 0;
  iNumValues = 0;
  myName = name;
  myValue = value;

  for (size_t curtext = 0; curtext < textlen; curtext++)
  {
    switch(state)
    {
    case STATE_NONE:
      if (isalnum((unsigned char)text[curtext]) || text[curtext]=='.' || text[curtext]=='_')
      {
        name_start = curtext;
        state = STATE_NAME;
      }
      break;
    case STATE_NAME:
      if (text[curtext] == '=')
      {
        state = STATE_IS;
        name_len = curtext - name_start;
      }
      break;
    case STATE_IS:
      if (isalnum((unsigned char)text[curtext]) || text[curtext]=='.' || text[curtext] == '_')
      {
        value_start = curtext;
        value_len = 1;
        state = STATE_VALUE;
      }
      if (text[curtext] == '\"')
      {
        value_start = curtext;
        state = STATE_QUOTE;
        //curtext--;
      }
      break;
    case STATE_VALUE:
      if (text[curtext] == ';')
      {
        value_len = curtext - value_start;
        cur_name = (char*)malloc(name_len+1);
        memcpy(cur_name,&text[name_start],name_len);
        cur_name[name_len] = '\0';
        cur_value = (char*)malloc(value_len+1);
        memcpy(cur_value,&text[value_start],value_len);
        cur_value[value_len] = '\0';

//        printf("Calling strip_string for  <%s>. \n",cur_value);
        strip_string(cur_value);
        add_value(cur_name, cur_value);

        state = STATE_NONE;
      }
      else if (text[curtext] == '{')
      {
        value_len = curtext - value_start;
        state = STATE_CHILD;
        child_start = curtext+1;
        child_depth = 1;
      }
      break;
	case STATE_QUOTE:
		if ((text[curtext] == '\"') && (text[curtext+1] == '\"')) {
			curtext++;
		}
		else if (text[curtext] == '\"') {
			state = STATE_VALUE;
		}
		break;
    case STATE_CHILD:
      if (text[curtext] == '{')
        child_depth++;
      else if (text[curtext] == '}')
      {
        child_depth--;
        if (!child_depth)
        {
          cur_name = (char*)malloc(name_len+1);
          memcpy(cur_name,&text[name_start],name_len);
          cur_name[name_len] = '\0';
          cur_value = (char*)malloc(value_len+1);
          memcpy(cur_value,&text[value_start],value_len);
          cur_value[value_len] = '\0';
          child_len = curtext-child_start;
          state = STATE_NONE;

          strip_string(cur_value);

          pChildren[iNumChildren++] = new CConfigurator(this, cur_name, cur_value, &text[child_start], child_len);
        }
      }
    }
  }

  int i;
  if (parent==0)
  {
    myFlags = 0;
    for (i=0;i<iNumChildren;i++)
    {
      pChildren[i]->initialize();
    }
  }
}

/**
 * Destructor.
 *
 * \bug This does nothing now; it should:
 *       - if we are a top-level component (System or GUI) delete the component (which 
 *         will delete the components children).
 *       - delete our children.
 *       - free memory we allocated for values.
 *       .
 **/

CConfigurator::~CConfigurator(void)
{
}

/**
 * Reduce a quoted string to it's real value.
 * Some values are enclosed in double quotes("), in this case, we take off the quotes, 
 * and replace all double double quotes ("") with single double quotes ("). Quoting values
 * is particularly useful if values contain forbidden characters such as spaces, quotes,
 * semicolons, etc. e.g. a text like
 *   "c:\program files\putty\putty.exe" telnet://localhost:8000
 * should be quoted as 
 *   """c:\program files\putty\putty.exe"" telnet://localhost:8000"
 **/

char * CConfigurator::strip_string(char * c)
{
  char * pos = c;
  char * org = c+1;
  bool end_it = false;

  if (c[0] =='\"')
  {
    while (!end_it)
    {
      if (*org=='\"')
      {
        org++;
        if (*org=='\"')
          *pos++ = *org++;
        else
          end_it = true;
      }
      else
        *pos++ = *org++;
    }
    *pos = '\0';
  }

  return c;
}

/**
 * Add a value to our list of values.
 **/

void CConfigurator::add_value(char * n, char * v)
{
  pValues[iNumValues].name = n;
  pValues[iNumValues].value = v;
  iNumValues++;
}

/**
 * Return a text value, if the name of the value can't be found in
 * our list of values, return def.
 **/

char * CConfigurator::get_text_value(const char * n, char * def)
{
  int i;
  for (i=0;i<iNumValues;i++)
  {
    if (!strcmp(pValues[i].name, n))
      return pValues[i].value;
  }
  return def;
}

/**
 * Return a boolean value, if the name of the value can't be found in
 * our list of values, or if the value isn't valid, return def.
 *
 * Valid values are strings that have a first character of:
 *  - t (true)
 *  - y (yes, evaluates to true)
 *  - 1 (evaluates to true)
 *  - f (false)
 *  - n (no, evaluates to false)
 *  - 0 (evaluates to false)
 *  .
 **/

bool CConfigurator::get_bool_value(const char * n, bool def)
{
  int i;
  for (i=0;i<iNumValues;i++)
  {
    if (!strcmp(pValues[i].name, n))
    {
      switch (pValues[i].value[0])
      {
      case 't':
      case 'T':
      case 'y':
      case 'Y':
      case '1':
        return true;
      case 'f':
      case 'F':
      case 'n':
      case 'N':
      case '0':
        return false;
      default:
        printf("Illegal boolean value: %s   \n", pValues[i].value);
        FAILURE("Configuration error");
      }
    }
  }
  return def;
}

/**
 * Return a numeric value, if the name of the value can't be found in
 * our list of values, return def.
 **/

u64 CConfigurator::get_num_value(const char * n, u64 def)
{
  int i;
  for (i=0;i<iNumValues;i++)
  {
    if (!strcmp(pValues[i].name, n))
    {
      u64 retval = 0;
      u64 partval = 0;
      char * val = pValues[i].value;
      int j = 0;
      for(;;) {
        while (val[j] && strchr("0123456789",val[j])) {
          partval *=10;
          partval += val[j]-'0';
          j++;
        }
        switch(val[j])
        {
        case 'T':
          partval *= 1024;
        case 'G':
          partval *= 1024;
        case 'M':
          partval *= 1024;
        case 'K':
          retval += partval*1024;
          partval=0;
          j++;
          break;
        case '\0':
          retval += partval;
          return retval;
        default:
          printf("%s: numeric value %s (%s) not in right format!\n",myName, n, val);
          FAILURE("Configuration error");
        }
      } 
    }
  }
  return def;
}

// THIS IS WHERE THINGS GET COMPLICATED...

#define NO_FLAGS   0

#define IS_CS      1
#define ON_CS      2

#define HAS_PCI    4
#define IS_PCI     8

#define HAS_ISA   16
#define IS_ISA    32

#define HAS_DISK  64
#define IS_DISK  128

#define IS_GUI   256
#define ON_GUI   512

#define IS_NIC  1024

#define N_P     2048 // no parent

typedef struct {
  char * name;
  classid id;
  int flags;
} classinfo;

classinfo classes[] = 
{
  {"tsunami", c_tsunami, N_P | IS_CS  | HAS_PCI                             },
  {"ev68cb",  c_ev68cb,        ON_CS                                        },
  {"ali",     c_ali,                    IS_PCI | HAS_ISA                    },
  {"ali_ide", c_ali_ide,                IS_PCI |           HAS_DISK         },
  {"ali_usb", c_ali_usb,                IS_PCI                              },
  {"serial",  c_serial,        ON_CS                                        },
  {"s3",      c_s3,                     IS_PCI |                    ON_GUI  },
  {"cirrus",  c_cirrus,                 IS_PCI |                    ON_GUI  },
  {"radeon",  c_radeon,                 IS_PCI |                    ON_GUI  },
  {"dec21143",c_dec21143,               IS_PCI |                    IS_NIC  },
  {"sym53c895", c_sym53c895,            IS_PCI |           HAS_DISK         },
  {"sym53c810", c_sym53c810,            IS_PCI |           HAS_DISK         },
  {"file",    c_file,                                      IS_DISK          },
  {"device",  c_device,                                    IS_DISK          },
  {"ramdisk", c_ramdisk,                                   IS_DISK          },
  {"sdl",     c_sdl,     N_P |                                      IS_GUI  },
  {"win32",   c_win32,   N_P |                                      IS_GUI  },
  {"X11",     c_x11,     N_P |                                      IS_GUI  },
  {0,         c_none,    0                                                  }
};

/**
 * Determine what device this configurator represents, and instantiate it; 
 * then call initialize for our children.
 **/

void CConfigurator::initialize()
{
  myClassId = c_none;
  int i = 0;
  int pcibus = 0;
  int pcidev = 0;
  int idedev = 0;
  int idebus = 0;
  int number;
  char * pt;

  for(i=0;classes[i].id != c_none; i++)
  {
    if (!strcmp(myValue,classes[i].name))
    {
      myClassId = classes[i].id;
      myFlags = classes[i].flags;
      break;
    }
  }

  if (myClassId == c_none)
  {
    printf("Class %s not known!!\n",myValue);
    throw((int)1);
  }

  if (myFlags & N_P)
  {
    if (pParent->get_flags())
    {
      printf("Error: %s(%s) should not have a parent!\n",myName,myValue);
      throw((int)1);
    }
  }

  if (myFlags & ON_CS)
  {
    if (!(pParent->get_flags() & IS_CS))
    {
      printf("Error: parent of SYSBUS device %s(%s) should be a chipset.\n",myName,myValue);
      throw((int)1);
    }
  }

  if (myFlags & ON_GUI)
  {
    if (!bx_gui)
    {
      printf("Error: %s(%s) needs a GUI.\n",myName,myValue);
      throw((int)1);
    }
  }

  if (myFlags & IS_GUI)
  {
    if (bx_gui)
    {
      printf("Error: %s(%s): another GUI was already instantiated.\n",myName,myValue);
      throw((int)1);
    }
  }

#if !defined(HAVE_PCAP)
  if (myFlags & IS_NIC)
  {
    printf("Error: %s(%s): For network support, compilation with libpcap support is required.\n",myName,myValue);
    throw((int)1);
  }
#endif

  if (myFlags & IS_PCI)
  {
    if (strncmp(myName,"pci",3))
    {
      printf("Error: name of PCI device %s should be pci<bus>.<device>, %s found.\n",myValue,myName);
      throw((int)1);
    }
    if (!(pParent->get_flags() & HAS_PCI))
    {
      printf("Error: parent of PCI device %s(%s) should be a pci-bus capable device.\n",myName,myValue);
      throw((int)1);
    }

    pt = &myName[3];
    pcibus = atoi(pt);
    pt = strchr(pt,'.');
    if (!pt)
    {
      printf("Error: name of PCI device %s should be pci<bus>.<device>, %s found.\n",myValue,myName);
      throw((int)1);
    }
    pt++;
    pcidev = atoi(pt);
  }

  if (myFlags & IS_DISK)
  {
    if (strncmp(myName,"disk",4))
    {
      printf("Error: name of Disk device %s should be disk<channel>.<device>, %s found.\n",myValue,myName);
      throw((int)1);
    }
    if (!(pParent->get_flags() & HAS_DISK))
    {
      printf("Error: parent of disk device %s(%s) should be a disk controller.\n",myName,myValue);
      throw((int)1);
    }

    pt = &myName[4];
    idebus = atoi(pt);
    pt = strchr(pt,'.');
    if (!pt)
    {
      printf("Error: name of Disk device %s should be disk<controller>.<device>, %s found.\n",myValue,myName);
      throw((int)1);
    }
    pt++;
    idedev = atoi(pt);
  }

  switch(myClassId)
  {
  case c_tsunami:
    myDevice = new CSystem(this);
    new CDPR(this,(CSystem *)myDevice);
    new CFlash(this,(CSystem *)myDevice);
    break;

  case c_ev68cb:
    myDevice = new CAlphaCPU(this,(CSystem *)pParent->get_device());
    break;

  case c_ali:
    myDevice = new CAliM1543C(this,(CSystem *)pParent->get_device(),pcibus,pcidev);
    new CPort80(this,(CSystem *)pParent->get_device());
    new CKeyboard(this,(CSystem *)pParent->get_device());
    new CDMA(this,(CSystem *)pParent->get_device());
    break;

  case c_ali_ide:
    myDevice = new CAliM1543C_ide(this,(CSystem *)pParent->get_device(),pcibus,pcidev);
    break;

  case c_ali_usb:
    myDevice = new CAliM1543C_usb(this,(CSystem *)pParent->get_device(),pcibus,pcidev);
    break;

  case c_s3:
    myDevice = new CS3Trio64(this,(CSystem *)pParent->get_device(),pcibus,pcidev);
    break;

  case c_cirrus:
    myDevice = new CCirrus(this,(CSystem *)pParent->get_device(),pcibus,pcidev);
    break;

  case c_radeon:
#if defined(HAVE_RADEON)
    myDevice = new CRadeon(this,(CSystem *)pParent->get_device(),pcibus,pcidev);
#else
    FAILURE("ES40 was not compiled with Radeon support");
#endif
    break;

#if defined(HAVE_PCAP)
  case c_dec21143:
    myDevice = new CDEC21143(this,(CSystem *)pParent->get_device(),pcibus,pcidev);
    break;
#endif

  case c_sym53c895:
    myDevice = new CSym53C895(this,(CSystem *)pParent->get_device(),pcibus,pcidev);
    break;

  case c_sym53c810:
    myDevice = new CSym53C810(this,(CSystem *)pParent->get_device(),pcibus,pcidev);
    break;

  case c_file:
    myDevice = new CDiskFile(this,theSystem,(CDiskController *)pParent->get_device(),idebus,idedev);
    break;

  case c_device:
    myDevice = new CDiskDevice(this,theSystem,(CDiskController *)pParent->get_device(),idebus,idedev);
    break;

  case c_ramdisk:
    myDevice = new CDiskRam(this,theSystem,(CDiskController *)pParent->get_device(),idebus,idedev);
    break;

  case c_serial:
    number = 0;
    if (!strncmp(myName,"serial",6))
    {
      pt = &myName[6];
      number = atoi(pt);
    }
    myDevice = new CSerial(this,(CSystem *)pParent->get_device(),number);
    break;

  case c_sdl:
#if defined(HAVE_SDL)
    PLUG_load_plugin (this, sdl);
#else
    FAILURE("Can't instantiate the SDL GUI without SDL support");
#endif
    break;

  case c_win32:
#if defined(_WIN32)
    PLUG_load_plugin (this, win32);
#else
    FAILURE("Can't instantiate the Win32 GUI on a non-Win32 platform");
#endif
    break;

  case c_x11:
#if defined(HAVE_X11)
    PLUG_load_plugin (this, x11);
#else
    FAILURE("Can't instantiate the X11 GUI on a non-X11 platform");
#endif
    break;

  case c_none:
    break;
  }

  for (i=0;i<iNumChildren;i++)
    pChildren[i]->initialize();
}
