#ifndef INTELCPU_H
#define INTELCPU_H
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
 * Interface between the CP/M emulator and the CPU
 * See: https://sourceforge.net/projects/zilogz80/
 *
*/

#include <unistd.h>
#include <map>

#include "Z80.h"
#include "cpm.h"
#include "biosapi.h"

class IntelCPU : public Z80
{
    friend  class monitor;

public:
    IntelCPU();

    void powercycle(void);

    void trace(bool onoff)
    {
        m_trace = onoff;
    }

    bool trace() const { return m_trace;}

    void reset() { m_reset =1;}

    void setBrk(uint16_t addr, uint16_t cnt)
    {
        breakpoints_[addr] = cnt;
    }

    unsigned selectdisk(unsigned id, const dpb& dpb, const uint8_t* tltbl);
    void     readmem(uint8_t *buffer, uint16_t addr, uint16_t size);
    ostream& readmem(ostream &f, uint16_t addr, uint16_t &size);
    void     writemem(const uint8_t *buffer, uint16_t addr, uint16_t size);
    istream& writemem(istream &f, uint16_t addr, uint16_t &size);

protected:
    bool m_trace;
    int  m_reset;

    void initBIOS(void);

    uint8_t rmem(Z80Bus::address_t addr) const
    {
        return m_ram[addr];
    }

    void Out(Z80Bus::address_t Port, Z80Bus::data_t data);
    Z80Bus::data_t In(Z80Bus::address_t Port);

    unsigned rcpmdisk(unsigned track, unsigned sector, uint8_t* dma);
    unsigned wcpmdisk(unsigned track, unsigned sector, uint8_t* dma);
    map<uint16_t,uint16_t>breakpoints_;

private:
    int      m_FDCD;
    uint16_t m_FDCT;
    uint16_t m_FDCS;
    uint16_t m_FDCOP;
    uint16_t m_FDCST;
    uint16_t m_DMA;
    uint16_t m_DPH;
    uint16_t m_BREAK;

    uint16_t bus(Z80Bus &iobus);
    Z80Bus::data_t m_ram[0x10000]
#ifdef DEBUG
    __attribute__((aligned(0x10000)));
#else
    ;
#endif

};


#endif // INTELCPU_H
