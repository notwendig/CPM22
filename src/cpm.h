#ifndef CPM_H
#define CPM_H

/*
 * Declaration file for the CP/M 2.2 System Simulator.
 * Copyright (C) 2015  Juergen Willi Sievers @notwendiger.
 *
 * This file is part of libZ80 - Zilog's Z80 instruction set emulator.
 *
 * CP/M 2.2 is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * If you got some questions or find bugs or parts that could be made better
 * don't hesitate to drop a line by email: <notwendig@augenpunkte.de> or also
 * by Twitter @notwendiger
 *
 * MANY THANKS TO Grant Searle. Thanks a lot for your wonderful documentation about ZX80 video interface.
 * Visit his grate electronic Suite at http://searle.hostei.com/grant or follow him
 * at Twitter by using @zx80nut
 *
 * Declarations for CP/M 2.2 emulation system
 * See: https://sourceforge.net/projects/zilogz80/
 *
*/

#include <fstream>

typedef enum
{
    CPMPORT_CONSTA = 0,		//;console status port
    CPMPORT_CONDAT = 1,		//;console data port
    CPMPORT_PRTSTA = 2,		//;printer status port
    CPMPORT_PRTDAT = 3,		//;printer data port
    CPMPORT_AUXDAT = 5,		//;auxiliary data port
    CPMPORT_FDCD = 10,		//2;fdc-port: # of drive
    CPMPORT_FDCT = 11,		//;fdc-port: # of track
    CPMPORT_FDCS = 12,		//;fdc-port: # of sector
    CPMPORT_FDCOP = 13,		//;fdc-port: command
    CPMPORT_FDCST = 14,		//;fdc-port: status
    CPMPORT_DMAL = 15,		//;dma-port: dma address low
    CPMPORT_DMAH = 16,		//;dma-port: dma address high
    CPMPORT_DPBL = 17,		//;dpb low
    CPMPORT_DPBH = 18,		//;dpb high

    CPMPORT_TRACE   = 0x80, //; 1=set, 0=unset trace
    CPMPORT_BREAKL	= 0x81,	//; break-point low
    CPMPORT_BREAKH	= 0x82,	//; break-point high
    CPMPORT_CBREAK	= 0x83,	//; 1=set, 0=clr breakpoint

} port_t;

typedef enum {
    LOFF,
    LERR,
    LWRN,
    LINF
} MSGLEVEL;

extern MSGLEVEL    verbosity;
extern MSGLEVEL    debug;
extern std::ofstream    nullstream;

#define LOG(L)  ((((unsigned) verbosity <= (unsigned) (L)) && (verbosity != LOFF)) ? clog: nullstream)
#define DBG(L)  ((((unsigned) verbosity <= (unsigned) (L)) && (verbosity != LOFF)) ? clog: nullstream)


#endif // CPM_H


