/* ES40 emulator.
 * Copyright (C) 2007 by the ES40 Emulator Project
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
 * Contains the code for the VGA base class.
 *
 * X-1.3        Brian Wheeler                                   27-FEB-2008
 *      Avoid compiler warnings.
 *
 * X-1.2        Camiel Vanderhoeven                             28-DEC-2007
 *      Throw exceptions rather than just exiting when errors occur.
 *
 * X-1.1        Camiel Vanderhoeven                             10-DEC-2007
 *      Initial version in CVS.
 **/

#include "StdAfx.h"
#include "VGA.h"

CVGA::CVGA(class CConfigurator * cfg, class CSystem * c, int pcibus, int pcidev) : CPCIDevice(cfg,c,pcibus,pcidev)
{
  if (theVGA != 0)
    FAILURE("More than one VGA card not supported!!");
  theVGA = this;
}

CVGA::~CVGA(void)
{
}

CVGA * theVGA = 0;
