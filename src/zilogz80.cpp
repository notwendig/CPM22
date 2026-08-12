/*
 * Declaration file for the CP/M 2.2 System Simulator.
 * Copyright (C) 2015  Juergen Willi Sievers @notwendig.
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
 * by Twitter @notwendig
 *
 * MANY THANKS TO Grant Searle. Thanks a lot for your wonderful documentation about ZX80 video interface.
 * Visit his grate electronic Suite at http://searle.hostei.com/grant or follow him
 * at Twitter by using @zx80nut
 *
 * Interface between the CP/M emulator and the CPU
 * See: https://sourceforge.net/projects/zilogz80/
 *
*/

#include "cpm.h"
#include "boot.h"
#include "zilogz80.h"

#include <sys/stat.h>
#include <fstream>
#include <algorithm>

char ConIn(int newsockfd);
int ConOut(int newsockfd, char c);
int ConChk(int newsockfd);

#define MAXDRIVE 16

const char *drive[MAXDRIVE] =
{
    BOOTDISK,           // A
    "./disks/driveb",     // B
    "./disks/drivea.cpm", //"./disks/drivec",     // C
    /*"./disks/22src2.img",*/ "./disks/drived",     // D
    0, 0, 0, 0,         // E, F, G, H
    "./disks/drivei",     // I
    "./disks/drivei.img", // J
    0, 0, 0, 0, 0       // K, L, M, O, P
};

static unsigned currentdrive = MAXDRIVE;

static simdrive *diskptr[MAXDRIVE] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

static const dpb_t DPB_IBM8 = { 26, 3, 7, 0, 242, 63, 192, 0, 16, 2 } ;
static const uint8_t TRANS_IBM8[] = {
    1,  7,13,19,	// sectors 1,2,3,4
    25, 5,11,17,	// sectors 5,6,7,8
    23, 3, 9,15,	// sectors 9,10,11,12
    21, 2, 8,14,	// sectors 13,14,15,16
    20,26, 6,12,	// sectors 17,18,19,20
    18,24, 4,10,	// sectors 21,22,23,24
    16,22           // sectors 25,26
};



IntelCPU::IntelCPU()
    :Z80()
    ,m_trace(false)
    ,m_reset(0)
    ,m_FDCD(-1)
    ,m_FDCT(0)
    ,m_FDCS(0)
    ,m_FDCOP(0)
    ,m_FDCST(0)
    ,m_DMA(0)
    ,m_DPH(0)
    ,m_BREAK(0)
{
}

uint16_t IntelCPU::bus(Z80Bus &iobus)
{
    // M1 opcode zycle
    if(m_reset)
    {
        iobus.setPin(RESET, true);
        m_reset = 0;
        return 0;
    }

    if(iobus.obus() & BUSACK)
    {
        usleep(100);
        return 0;
    }

    if (iobus.check(RD|M1|MEMREQ))
    {
        map<uint16_t,uint16_t>::iterator it = breakpoints_.find((Z80Bus::address_t)iobus);
        if( it != breakpoints_.end() && it->second)
        {
            cout << *this << endl;
            //raise(SIGTRAP);
            if(it->second != 0xFFFF)
                it->second--;
        }

        if(m_trace)
            cout << *this << endl;

        iobus = m_ram[(Z80Bus::address_t)iobus];
    }

    // Mem read zycle
    else if (iobus.check(RD|MEMREQ))
        iobus = m_ram[(Z80Bus::address_t)iobus];

    // Mem write zycle
    else if (iobus.check(WR|MEMREQ))
        m_ram[(Z80Bus::address_t)iobus] = iobus;

    // interrupt ack cycle
    else if (iobus.check(RD|IOREQ|M1))
    {
        //intreq();
        m_ram[(Z80Bus::address_t)iobus] = iobus;
    }
    // input operation
    else if (iobus.check(IOREQ|RD))
        iobus = In(iobus);

    // output operation
    else if (iobus.check(IOREQ|WR))
        Out(iobus, iobus);

    // refresch cycle.
    else if (iobus.check(MEMREQ|REFRESH))
    {

    }
    return 0;
}


void IntelCPU::Out(Z80Bus::address_t Port, Z80Bus::data_t Value)
{
    LOG(LINF) << "out (" << hex << setw(4) << setfill(' ') << Port << "h)," << Value << 'h' << endl;

    switch ((port_t)(Port & 0xFF))
    {
        case CPMPORT_CONSTA:	//;console status port
        break;
        case CPMPORT_CONDAT:	//;console data port
            ConOut(0, Value);
        break;
        case CPMPORT_PRTSTA: 	//;printer status port
        break;
        case CPMPORT_PRTDAT:	//;printer data port
            putchar(Value);
        break;
        case CPMPORT_AUXDAT:	//;auxiliary data port
        break;
        case CPMPORT_FDCD:		//;fdc-port: # of drive
        {
            m_FDCD = Value;
            const dpb_t *pdpb;
            const uint8_t *ptrans;
            if(m_DPH)
            {
                const dph_t *pdbh = (const dph_t*)(m_ram + m_DPH);
                pdpb = (const dpb_t *) (m_ram + pdbh->DPBLK);
                ptrans=(const uint8_t*) (m_ram + pdbh->TRANS);
            }
            else
            {
                pdpb = &DPB_IBM8;
                ptrans = TRANS_IBM8;
            }
            m_FDCST = selectdisk(m_FDCD, dpb(*pdpb), ptrans);
        }
        break;
        case CPMPORT_FDCT:		//;fdc-port: # of track
            m_FDCT = Value;
        break;
        case CPMPORT_FDCS:		//;fdc-port: # of sector
            m_FDCS = Value;
        break;
        case CPMPORT_FDCOP:		//;fdc-port: command
            m_FDCOP = Value;
            if (Value == 0)
                m_FDCST = rcpmdisk(m_FDCT, m_FDCS, m_ram + m_DMA);
            else if (Value == 1)
                m_FDCST = wcpmdisk(m_FDCT, m_FDCS, m_ram + m_DMA);
            else
                m_FDCST=1;
        break;
        case CPMPORT_FDCST:		//;fdc-port: status
        break;
        case CPMPORT_DMAL:		//;dma-port: dma address low
            m_DMA = (m_DMA & 0xFF00) | Value;
            m_FDCST=0;
        break;
        case CPMPORT_DMAH:		//;dma-port: dma address high
            m_DMA = (m_DMA & 0xFF) | (Value << 8);
            m_FDCST=0;
        break;
        case CPMPORT_DPBL:    //;dpb low
            m_DPH = (m_DPH & 0xFF00) | Value;
            m_FDCST=0;
        break;
        case CPMPORT_DPBH:    //;dpb high
            m_DPH = (m_DPH & 0xFF) | (Value << 8);
            m_FDCST=0;
        break;
        case CPMPORT_TRACE:
            trace(Value);
        break;
        case CPMPORT_BREAKL:
            m_BREAK = (m_BREAK & 0xFF00) | Value;
        break;
        case CPMPORT_BREAKH:
            m_BREAK = (m_BREAK & 0xFF) | (Value << 8);
        break;
        case CPMPORT_CBREAK:
            breakpoints_[m_BREAK] = Value;
        break;
        default:
            LOG(LWRN) << "Unassigned port outpup " << Value << " on port " << Port << endl;
        break;
    }
}

Z80Bus::data_t IntelCPU::In(Z80Bus::address_t Port)
{
    unsigned char res = 0;

    switch ((port_t)(Port & 0xFF))
    {
        case CPMPORT_CONSTA:	//;console status port
            res = ConChk(0);
            if(!res)
                usleep(300);
        break;
        case CPMPORT_CONDAT:	//;console data port
            res = ConIn(0);
        break;
        case CPMPORT_PRTSTA: 	//;printer status port
            res = 0xFF;
        break;
        case CPMPORT_PRTDAT:	//;printer data port
        break;
        case CPMPORT_AUXDAT:	//;auxiliary data port
        break;
        case CPMPORT_FDCD:		//;fdc-port: # of drive
            res = m_FDCD;
        break;
        case CPMPORT_FDCT:		//;fdc-port: # of track
            res = m_FDCT;
        break;
        case CPMPORT_FDCS:		//;fdc-port: # of sector
            res = m_FDCS;
        break;
        case CPMPORT_FDCOP:		//;fdc-port: command
            res = m_FDCOP;
        break;
        case CPMPORT_FDCST:		//;fdc-port: status
            if(m_FDCST)
                res = 1;
            res = m_FDCST;
        break;
        case CPMPORT_DMAL:		//;dma-port: dma address low
            res = m_DMA & 0xFF;
        break;
        case CPMPORT_DMAH:		//;dma-port: dma address high
            res = (m_DMA >> 8) & 0xFF;
        break;
        case CPMPORT_DPBL:    //;dpb low
            res = m_DPH & 0xFF;
        break;
        case CPMPORT_DPBH:    //;dpb high
            res = (m_DPH >> 8) & 0xFF;
        break;
        case CPMPORT_TRACE:
            res = trace();
        break;
        case CPMPORT_BREAKL:
            res = m_BREAK;
        break;
        case CPMPORT_BREAKH:
            res = m_BREAK >> 8;
        break;
        case CPMPORT_CBREAK:
        {
            map<uint16_t,uint16_t>::const_iterator cit = breakpoints_.find(m_BREAK);
            res = (cit != breakpoints_.end())? cit->second : 0;
        }
        break;
        default:
            res = 0xFF;
            DBG(LWRN) << "Unassigned port input on Port" << Port << endl;
        break;
    }

    LOG(LINF) << "in" << hex << setw(4) << setfill(' ') << res << "h, (" << Port << "h)" << endl;

    return res;
}

void IntelCPU::initBIOS(void)
{
    ifstream os;
    os.exceptions ( std::ifstream::failbit | std::ifstream::badbit );

    uint16_t secsize = 128;

    LOG(LINF) << "bootloader " << BOOTSECTOR << endl;
    try {
//        os.open(BOOTSECTOR);	// load cpm/2.2 bootloader
        writemem(os, 0, secsize);
        os.close();
    } catch (const std::ifstream::failure& e)
    {
        LOG(LWRN) << "Error extern BOOT-Sector " << BOOTSECTOR << ". Using build in." << e.what() << endl;
        writemem(boot, 0, boot_length);
    }
}

void IntelCPU::powercycle(void)
{
    setPower(false);
    initBIOS();
    setPower(true);
    setMHz(0);
}


unsigned IntelCPU::selectdisk(unsigned id, const dpb& dpb, const uint8_t* tltbl)
{

    // drive id 0=A,1=B...
    if(id < MAXDRIVE && drive[id])
    {
        if (id != currentdrive)
        {
            // currentdrive == MAXDRIVE means "no drive selected".
            // Never index diskptr[] with that sentinel value.
            if(currentdrive < MAXDRIVE && diskptr[currentdrive])
            {
               diskptr[currentdrive]->unSelect();
            }

            currentdrive = MAXDRIVE;

            struct stat st;
            if( stat(drive[id], &st))
            {
                LOG(LERR) << " Can't stat " << drive[id] << endl;
                return 1;
            }

            if(!diskptr[id])
            {
                if((st.st_mode & S_IFMT) == S_IFDIR)
                {
                    diskptr[id] = new disk((const char*)drive[id], CPM22, dpb, tltbl);
                }
                else if((st.st_mode & S_IFMT) == S_IFREG)
                {
                        diskptr[id] = new dskimage((const char*)drive[id], CPM22, dpb, tltbl);
                }
                else
                {
                    cerr << drive[id] << " not a file or directory" << endl;
                    return 1;
                }

            }
            else
                diskptr[id]->Select();

            if( diskptr[id])
                currentdrive = id;
            else
                return 1;

            LOG(LINF) << "init disk " << id << ", " << drive[id] << (char*) (diskptr[id] ? " OK":" ER") << endl;
        }
        return 0;
    }

    cerr << "No disk inserted or drive " << id << " not present" << endl;
    return 1;
}

unsigned IntelCPU::rcpmdisk(unsigned track, unsigned sector, uint8_t* dma)
{
    int res = 1;

    if ( currentdrive < MAXDRIVE && diskptr[currentdrive])
        res = diskptr[currentdrive]->Read(track, sector, dma);
    return res;
}

unsigned IntelCPU::wcpmdisk(unsigned track, unsigned sector, uint8_t* dma)
{
    int res = 1;
    if (currentdrive < MAXDRIVE && diskptr[currentdrive])
        res = diskptr[currentdrive]->Write(track, sector, dma);
    return res;
}

void IntelCPU::readmem(uint8_t *buffer, uint16_t addr, uint16_t size)
{
    busrequest(true);
    while(size)
    {
        const size_t n = min(static_cast<size_t>(0x10000u - addr),
                             static_cast<size_t>(size));
        memcpy(buffer, m_ram + addr, n);
        buffer += n;
        addr = static_cast<uint16_t>(addr + n);
        size = static_cast<uint16_t>(size - n);
    }
    busrequest(false);
}

void IntelCPU::writemem(const uint8_t *buffer, uint16_t addr, uint16_t size)
{
    busrequest(true);
    while(size)
    {
        const size_t n = min(static_cast<size_t>(0x10000u - addr),
                             static_cast<size_t>(size));
        memcpy(m_ram + addr, buffer, n);
        buffer += n;
        addr = static_cast<uint16_t>(addr + n);
        size = static_cast<uint16_t>(size - n);
    }
    busrequest(false);
}

ostream& IntelCPU::readmem(ostream &f, uint16_t addr, uint16_t& size)
{
    busrequest(true);
    while(f && size)
    {
        const size_t n = min(static_cast<size_t>(size),
                             static_cast<size_t>(0x10000u - addr));
        f.write(reinterpret_cast<const char*>(m_ram + addr),
                static_cast<std::streamsize>(n));
        if(!f)
            break;
        addr = static_cast<uint16_t>(addr + n);
        size = static_cast<uint16_t>(size - n);
    }
    busrequest(false);
    return f;
}

istream& IntelCPU::writemem(istream &f, uint16_t addr, uint16_t& size)
{
    busrequest(true);
    while(f && size)
    {
        const size_t n = min(static_cast<size_t>(size),
                             static_cast<size_t>(0x10000u - addr));
        f.read(reinterpret_cast<char*>(m_ram + addr),
               static_cast<std::streamsize>(n));
        const std::streamsize got = f.gcount();
        if(got <= 0)
            break;
        addr = static_cast<uint16_t>(addr + static_cast<size_t>(got));
        size = static_cast<uint16_t>(size - static_cast<size_t>(got));
    }
    busrequest(false);
    return f;
}


