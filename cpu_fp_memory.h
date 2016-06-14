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
 * Contains code macros for the processor floating-point load/store instructions.
 * Based on ARM chapter 4.8.
 *
 * X-1.13       Brian Wheeler                                   27-FEB-2008
 *      Avoid compiler warnings.
 *
 * X-1.12       Camiel Vanderhoeven                             06-FEB-2008
 *      Check for FPEN in old floating point code. 
 *
 * X-1.11       Camiel Vanderhoeven                             05-FEB-2008
 *      Only use new floating-point code when HAVE_NEW_FP has been defined.
 *
 * X-1.10       Camiel Vanderhoeven                             28-JAN-2008
 *      Better floating-point exception handling.
 *
 * X-1.9        Camiel Vanderhoeven                             25-JAN-2008
 *      Trap on unalogned memory access. The previous implementation where
 *      unaligned accesses were silently allowed could go wrong when page
 *      boundaries are crossed.
 *
 * X-1.8        Camiel Vanderhoeven                             21-JAN-2008
 *      Implement new floating-point code.
 *
 * X-1.7        Camiel Vanderhoeven                             2-DEC-2007
 *      Changed the way translation buffers work. 
 *
 * X-1.6        Camiel Vanderhoeven                             08-NOV-2007
 *      Restructured conversion routines.
 *
 * X-1.5        Eduardo Marcelo Serrat                          31-OCT-2007
 *      Fixed conversion routines.
 *
 * X-1.4        Camiel Vanderhoeven                             11-APR-2007
 *      Moved all data that should be saved to a state file to a structure
 *      "state".
 *
 * X-1.3        Camiel Vanderhoeven                             30-MAR-2007
 *      Added old changelog comments.
 *
 * X-1.2        Camiel Vanderhoeven                             13-MAR-2007
 *      Basic floating point support added.
 *
 * X-1.1        Camiel Vanderhoeven                             18-FEB-2007
 *      File created. Contains code previously found in AlphaCPU.h
 **/

#if defined(HAVE_NEW_FP)

#define DO_LDF									                    \
	FPSTART; \
	if (FREG_1 != 31) {							                    \
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_READ, 3);	        \
	  state.f[FREG_1] = vax_ldf((u32)READ_PHYS(32));                \
    }

#define DO_LDG									                    \
	FPSTART; \
	if (FREG_1 != 31) {							                    \
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_READ, 7);          \
	  state.f[FREG_1] = vax_ldg(READ_PHYS(64));                     \
    }

#define DO_LDS									                    \
	FPSTART; \
	if (FREG_1 != 31) {							                    \
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_READ, 3);          \
	  state.f[FREG_1] = ieee_lds((u32)READ_PHYS(32));               \
    }

#define DO_LDT									                    \
	FPSTART; \
	if (FREG_1 != 31) {							                    \
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_READ, 7);          \
	  state.f[FREG_1] = READ_PHYS(64);                              \
    }

#define DO_STF									                    \
	FPSTART; \
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_WRITE, 3);         \
	  WRITE_PHYS(vax_stf(state.f[FREG_1]),32);

#define DO_STG									                    \
	FPSTART; \
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_WRITE, 7);         \
	  WRITE_PHYS(vax_stg(state.f[FREG_1]),64);

#define DO_STS									                    \
	FPSTART; \
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_WRITE, 3);	        \
	  WRITE_PHYS(ieee_sts(state.f[FREG_1]),32);

#define DO_STT									                    \
	FPSTART; \
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_WRITE, 7);	        \
	  WRITE_PHYS(state.f[FREG_1],64);

#else

#define DO_LDF                                                   \
  FPSTART;                                                          \
	if (FREG_1 != 31) {							\
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_READ,3);	\
	  state.f[FREG_1] = load_f((u32)READ_PHYS(32)); }

#define DO_LDG                                                   \
  FPSTART;                                                          \
  	if (FREG_1 != 31) {							\
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_READ,7);	\
	  state.f[FREG_1] = load_g(READ_PHYS(64)); }

#define DO_LDS                                                   \
  FPSTART;                                                          \
  	if (FREG_1 != 31) {							\
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_READ,3);	\
	  state.f[FREG_1] = load_s((u32)READ_PHYS(32)); }

#define DO_LDT                                                   \
  FPSTART;                                                          \
  	if (FREG_1 != 31) {							\
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_READ,7);	\
	  state.f[FREG_1] = READ_PHYS(64); }

#define DO_STF                                                   \
  FPSTART;                                                          \
  	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_WRITE,3);	\
	  WRITE_PHYS(store_f(state.f[FREG_1]),32);

#define DO_STG                                                   \
  FPSTART;                                                          \
  	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_WRITE,7);	\
	  WRITE_PHYS(store_g(state.f[FREG_1]),64);

#define DO_STS                                                   \
  FPSTART;                                                          \
  	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_WRITE,3);	\
	  WRITE_PHYS(store_s(state.f[FREG_1]),32);

#define DO_STT                                                   \
  FPSTART;									\
	  DATA_PHYS(state.r[REG_2] + DISP_16, ACCESS_WRITE,7);	\
	  WRITE_PHYS(state.f[FREG_1],64);

#endif