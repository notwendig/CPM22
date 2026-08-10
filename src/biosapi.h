/*
 * biosapi.h
 *
 *  Created on: 06.05.2015
 *      Author: juergen
 *
 * Declaration file for the cp/m disk Simulator.
 * Copyright (C) 2015  Juergen Willi Sievers @notwendig.
 *
 *
 * CPM22 is free software; you can redistribute it and/or modify it under
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
*/

#ifndef BIOSAPI_H_
#define BIOSAPI_H_

#include "cpm.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <exception>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace std;

/*
 * Os Verion
 */
typedef enum {
    P2DOS,  // PC DOS
    ZSDOS,  // Z DOS
    CPM22,  // CP/M 2.2
    CPM30,  // CP/M 3.0
}ostype_t;


/* CP/M Disk parameter block
 *
 * SPT is the total number of sectors per track.
 * BSH is the data allocation block shift factor, determined by the data block allocation size.
 * BLM is the data allocation block mask (2[BSH-1]).
 * EXM is the extent mask, determined by the data block allocation size and the number of disk blocks.
 * DSM determines the total storage capacity of the disk drive.
 * DRM determines the total number of directory entries that can be stored on this drive.
 * AL0, AL1 determine reserved directory blocks.
 * CKS is the size of the directory check vector.
 * OFF is the number of reserved tracks at the beginning of the (logical) disk.
 */
typedef struct __attribute__((__packed__)) dpb_s {

    uint16_t spt;   // sectors per track
    uint8_t  bsh;   // block shift factor
    uint8_t  blm;   // block mask
    uint8_t  exm;   // extent mask
    uint16_t dsm;   // disk size-1
    uint16_t drm;   // directory max-1
    uint8_t  al0;   // alloc 0
    uint8_t  al1;   // alloc 1
    uint16_t cks;   // check size
    uint16_t off;   // track offset

    friend ostream& operator << (ostream &os, const dpb_s &obj)
    {
        ios::fmtflags f(os.flags());
        os << setfill(' ') << dec
           << "spt " << setw(5) << (unsigned) obj.spt << endl
           << "bsh " << setw(3) << (unsigned) obj.bsh << endl
           << "blm " << setw(3) << (unsigned) obj.blm << endl
           << "exm " << setw(3) << (unsigned) obj.exm << endl
           << "dsm " << setw(5) << (unsigned) obj.dsm << endl
           << "drm " << setw(5) << (unsigned) obj.drm << endl
           << "al0 " << setw(3) << (unsigned) obj.al0 << endl
           << "al1 " << setw(3) << (unsigned) obj.al1 << endl
           << "cks " << setw(5) << (unsigned) obj.cks << endl
           << "off " << setw(5) << (unsigned) obj.off;
        os.flags(f);
        return os;
    }
} dpb_t;

#define DEFAULTPTRCNT   16       // use 8bit extend block pointers

typedef struct
{
    uint16_t
    TRANS,  TRACK,
    SECTOR, DIRNUM,
    DIRBUF, DPBLK,
    CHK00,  ALL00;
}__attribute__((packed)) dph_t;

/*
 *  Characteristic sizes
 *      Each CP/M disk format is described by the following specific sizes:
 *
 *             Sector size in bytes
 *             Number of tracks
 *             Number of sectors
 *             Block size
 *             Number of directory entries
 *             Logical sector skew
 *             Number of reserved system tracks
 *
 *      A  block is the smallest allocatable storage unit.  CP/M supports block
 *      sizes of 1024, 2048, 4096, 8192 and 16384 bytes.   Unfortunately,  this
 *      format  specification  is  not stored on the disk and there are lots of
 *      formats.  Accessing a block is  performed  by  accessing  its  sectors,
 *      which are stored with the given software skew.
 */


#pragma pack(push, 1)


/*
 * Abstraction of Disk parameter block.
 */
class dpb
{
public:
    dpb(const dpb_t& data);     // construct from dpb structure
    virtual ~dpb();

    unsigned BlockShift()       const { return dpb_.bsh;}
    unsigned BlockMsk()         const { return dpb_.blm;}
    unsigned ExtendMsk()        const { return dpb_.exm;}

    unsigned BlockSize() 		const { return 128 << BlockShift();}
    unsigned DirectoryEntries() const { return dpb_.drm + 1;}
    unsigned ReservedTracks()	const { return dpb_.off;}
    unsigned SectorsTrack()		const { return dpb_.spt;}
    unsigned MaxBlock()			const { return dpb_.dsm + 1;}
    unsigned A01()				const { return (dpb_.al0 << 8) | dpb_.al1;}
    unsigned BlockPtrType()     const { return (BlockSize() > 1024 && MaxBlock() >= 256) ? 8:16;}
    friend ostream& operator << (ostream &os, const dpb &obj) { os << obj.dpb_; return os;}
protected:
private:
    dpb_t   dpb_;
};
#pragma pack(pop)

/*
 * Directory entries
 *       The directory is a sequence of directory entries (also called extents),
 *      which contain 32 bytes of the following structure:
 *
 *              St F0 F1 F2 F3 F4 F5 F6 F7 E0 E1 E2 Xl Bc Xh Rc
 *              Al Al Al Al Al Al Al Al Al Al Al Al Al Al Al Al
 *
 *       St is the status; possible values are:
 *
 *              0-15: used for file, status is the user number
 *              16-31:  used for file, status is the user number (P2DOS) or used
 *              for password extent (CP/M 3 or higher)
 *              32: disc label
 *              33: time stamp (P2DOS)
 *              0xE5: unused
 *
 *       F0-E2 are the file name and its extension.  They  may  consist  of  any
 *       printable  7  bit ASCII character but: < > . , ; : = ? * [ ].  The file
 *       name must not be empty, the extension may be empty.   Both  are  padded
 *       with  blanks.   The  highest bit of each character of the file name and
 *       extension is used as attribute.   The  attributes  have  the  following
 *       meaning:
 *
 *              F0: requires set wheel byte (Backgrounder II)
 *              F1:   public   file   (P2DOS,  ZSDOS),  foreground-only  command
 *              (Backgrounder II)
 *              F2: date stamp (ZSDOS), background-only  commands  (Backgrounder
 *              II)
 *              F7: wheel protect (ZSDOS)
 *              E0: read-only
 *              E1: system file
 *              E2: archived
 *
 *       Public files (visible under each user number) are not supported by CP/M
 *       2.2, but there is a patch  and  some  free  CP/M  clones  support  them
 *       without any patches.
 *
 *       The  wheel  byte is (by default) the memory location at 0x4b.  If it is
 *       zero, only non-privileged commands may be executed.
 *
 *       Xl and Xh store the extent number.   A  file  may  use  more  than  one
 *       directory  entry,  if  it contains more blocks than an extent can hold.
 *       In this case, more extents are allocated and each of them  is  numbered
 *       sequentially  with  an extent number.  If a physical extent stores more
 *       than 16k, it is considered to contain multiple  logical  extents,  each
 *       pointing  to  16k  data, and the extent number of the last used logical
 *       extent is stored.  Note: Some formats decided to always store only  one
 *       logical  extent  in a physical extent, thus wasting extent space.  CP/M
 *       2.2 allows 512 extents per file, CP/M 3 and higher allow  up  to  2048.
 *       Bit 5-7 of Xl are 0, bit 0-4 store the lower bits of the extent number.
 *       Bit 6 and 7 of Xh are 0, bit 0-5 store the higher bits  of  the  extent
 *       number.
 *
 *       Rc  and  Bc  determine the length of the data used by this extent.  The
 *       physical extent is divided into logical extents, each of them being 16k
 *       in  size (a physical extent must hold at least one logical extent, e.g.
 *       a blocksize of 1024 byte with two-byte block pointers is not  allowed).
 *       Rc  stores  the  number  of  128  byte records of the last used logical
 *       extent.  Bc stores the number of bytes in the last  used  record.   The
 *       value  0  means 128 for backward compatibility with CP/M 2.2, which did
 *       not support Bc.
 *
 *       Al stores block pointers.  If  the  disk  capacity  is  less  than  256
 *       blocks,  Al  is  interpreted  as 16 byte-values, otherwise as 8 double-
 *       byte-values.  A block pointer of 0 marks a hole in the file.  If a hole
 *       covers  the  range  of a full extent, the extent will not be allocated.
 *       In particular, the first extent of a file  does  not  necessarily  have
 *       extent  number 0.  A file may not share blocks with other files, as its
 *       blocks would be freed if the other files is erased without a  following
 *       disk  system  reset.   CP/M returns EOF when it reaches a hole, whereas
 *       UNIX returns zero-value bytes, which makes holes invisible.
 *
 *  Time stamps
 *      P2DOS and CP/M Plus support time  stamps,  which  are  stored  in  each
 *      fourth  directory  entry.   This entry contains the time stamps for the
 *      extents using the previous three  directory  entries.   Note  that  you
 *      really  have  time stamps for each extent, no matter if it is the first
 *      extent of a file or not.  The structure of time stamp entries is:
 *
 *             1 byte status 0x21
 *             8 bytes time stamp for third-last directory entry
 *             2 bytes unused
 *             8 bytes time stamp for second-last directory entry
 *             2 bytes unused
 *             8 bytes time stamp for last directory entry
 *
 *      A time stamp consists of two dates: Creation and modification date (the
 *      latter  being  recorded  when  the  file is closed).  CP/M Plus further
 *      allows optionally to record the access  instead  of  creation  date  as
 *      first time stamp.
 *
 *             2 bytes (little-endian) days starting with 1 at 01-01-1978
 *             1 byte hour in BCD format
 *             1 byte minute in BCD format
 *
 *  Disc labels
 *      CP/M  Plus  support  disc  labels,  which  are  stored  in an arbitrary
 *      directory entry.  The structure of disc labels is:
 *
 *             1 byte status 0x20
 *             F0-E2 are the disc label
 *             1 byte mode: bit 7 activates password protection, bit  6  causes
 *             time   stamps   on   access,   but   5  causes  time  stamps  on
 *             modifications, bit 4 causes time stamps on creation and bit 0 is
 *             set when a label exists.  Bit 4 and 6 are exclusively set.
 *             1  byte  password  decode byte: To decode the password, xor this
 *             byte with the password bytes in  reverse  order.   To  encode  a
 *             password, add its characters to get the decode byte.
 *             2 reserved bytes
 *             8 password bytes
 *             4 bytes label creation time stamp
 *             4 bytes label modification time stamp
 *
 *  Passwords
 *      CP/M  Plus  supports  passwords,  which  are  stored  in  an  arbitrary
 *      directory entry.  The structure of these entries is:
 *
 *             1 byte status (user number plus 16)
 *             F0-E2 are the file name and its extension.
 *             1 byte password mode: bit 7 means password required for reading,
 *             bit 6 for writing and bit 5 for deleting.
 *             1  byte  password  decode byte: To decode the password, xor this
 *             byte with the password bytes in  reverse  order.   To  encode  a
 *             password, add its characters to get the decode byte.
 *             2 reserved bytes
 *             8 password bytes
 *
 */
#pragma pack(push, 1)
class cpmdir
{
public:
    typedef enum {
        CPMUSRNO,		// 0-15:  used for file, status is the user number
        PASSWDNO=16,	// 16-31: used for file, status is the user number (P2DOS) or used
                        //        for password extent (CP/M 3 or higher)
        DSKLABEL=32,    // 32:    disc label
        TMSTAMP,        // 33:    time stamp (P2DOS)
        UNUSED=0xE5     // 0xE5:  unused
    } status_t;

    typedef enum {
        SETWEEL, 		// F0: requires set wheel byte (Backgrounder II)
        PUBLIC,         // F1:   public   file   (P2DOS,  ZSDOS),  foreground-only  command (Backgrounder II)
        TIMEST,         // F2: date stamp (ZSDOS), background-only  commands  (Backgrounder II)
        PRTWEEL,        // F7: wheel protect (ZSDOS)
        READONLY,       // E0: read-only
        SYSTEM,         // E1: system file
        ARCHIVE         // E2: archived
    }attribut_t;

    cpmdir(unsigned pointersize=DEFAULTPTRCNT);
    explicit cpmdir(const uint8_t* dd, unsigned pointersize);
    cpmdir(const cpmdir&) = default;
    cpmdir& operator=(const cpmdir&) = default;
    virtual ~cpmdir();

    status_t GetStatus() const { return (status_t) St_;}
    string   GetName() const;
    unsigned GetXl() const { return Xl_;}
    unsigned GetXh() const { return Xh_;}
    unsigned GetRc() const { return Rc_;}
    unsigned GetBc() const { return Bc_;}

    unsigned GetALByte(unsigned idx) const { assert(idx < 16); return Al_.b[idx];}
    unsigned GetALWord(unsigned idx) const { assert(idx < 8);  return Al_.w[idx];}

    unsigned GetExtNumber()   const {return ((GetXh() << 8) | GetXl())  & 0x1FF;}
    unsigned FindBlockIndex(unsigned block, unsigned bps);

    void SetStatus(status_t s) { St_ = (uint8_t) s;}
    void SetName(const string& s);
    void SetXl(uint8_t s) { Xl_ = s;}
    void SetXh(uint8_t s) { Xh_ = s;}
    void SetRc(uint8_t s) { Rc_ = s;}
    void SetBc(uint8_t s) { Bc_ = s;}

    void SetPointerType(unsigned pt) { pointersize_ = pt;}
    void SetALByte(unsigned idx, uint8_t s) { assert(idx < 16); Al_.b[idx] = s;}
    void SetALWord(unsigned idx, uint16_t s) { assert(idx < 8); Al_.w[idx] = s;}

    int BlockOffset(unsigned block, unsigned blksize);

    // dump bynary content to byte stream
    void dump(ostream& os) const
    {
        os.write((const char*)&St_,1);
        os.write((const char*)Fx_,8);
        os.write((const char*)Ex_,3);
        os.write((const char*)&Xl_,1);
        os.write((const char*)&Bc_,1);
        os.write((const char*)&Xh_,1);
        os.write((const char*)&Rc_,1);
        os.write((const char*)&Al_,16);
    }

    // fill data from playn byte array
    uint8_t* read(uint8_t *dma) const
    {
        memcpy(dma,    (const char*)&St_,1);
        memcpy(dma+1,  (const char*)Fx_,8);
        memcpy(dma+9,  (const char*)Ex_,3);
        memcpy(dma+12, (const char*)&Xl_,1);
        memcpy(dma+13, (const char*)&Bc_,1);
        memcpy(dma+14, (const char*)&Xh_,1);
        memcpy(dma+15, (const char*)&Rc_,1);
        memcpy(dma+16, (const char*)&Al_,16);
        return dma+32;
    }

    friend ostream& operator << (ostream& os, const cpmdir& obj)
    {
        ios::fmtflags f(os.flags());
        os << hex << setw(2) << setfill(' ') << obj.GetStatus();

        if(obj.GetStatus() != UNUSED)
        {

            os << setfill(' ');

            os  << '-' << obj.GetName()
                << ", Xl " << dec << setw(3) << obj.GetXl()
                << ", Bc " << dec << setw(3) << obj.GetBc()
                << ", Xh " << dec << setw(3) << obj.GetXh()
                << ", Rc " << dec << setw(3) << obj.GetRc()
                << ", ";

            if(16 == obj.pointersize_)
                for(unsigned i = 0; i < 16; i++)
                    os << (char)(i ? ',':'<') << dec << setw(3) << obj.GetALByte(i);
            else
                for(unsigned i = 0; i < 8; i++)
                    os << (char)(i ? ',':'<') << dec << setw(5) << obj.GetALWord(i);

            os << '>';
        }
        os.flags(f);
        return os;
    }

    // comapte extends
    bool operator == (const cpmdir& obj)
    {
        return St_ == obj.St_ &&
               !strncmp((const char*)Fx_,(const char*)obj.Fx_,8) &&
               !strncmp((const char*)Ex_,(const char*)obj.Ex_,3) &&
               Xl_ == obj.Xl_ &&
               Bc_ == obj.Bc_ &&
               Xh_ == obj.Xh_ &&
               Rc_ == obj.Rc_ &&
               !memcmp((const void*)&Al_,(const void*)&obj.Al_,sizeof(Al_));
    }

    bool operator != (const cpmdir& obj) { return !(*this == obj);}

protected:
private:

    uint8_t St_;        // User number or status of extent
    uint8_t	Fx_[8];     // cp/m file name
    uint8_t Ex_[3];     // cp/m extansion
    uint8_t Xl_;        // extend number on file LSB
    uint8_t Bc_;        // sectors (128 byte) on this extend
    uint8_t Xh_;        // extend number on file MSB
    uint8_t	Rc_;        // Bytes on last sector (> cp/m 2.2)
    union {
        uint8_t b[16];  // block liste < 256 total blocks on media
        uint16_t w[8];  // block liste > 256 total blocks on media
    }Al_;
    unsigned pointersize_;
} ;
#pragma pack(pop)


class simdrive
{
public:
    simdrive(const char* path, ostype_t os, const dpb &dpb, const uint8_t *stbl);
    virtual ~simdrive();

    // read/write to cp/m disk
    virtual unsigned Read(unsigned track, unsigned sector, uint8_t *dma)=0;
    virtual unsigned Write(unsigned track, unsigned sector, const uint8_t *dma)=0;
    virtual unsigned Flush(void) { return 0;}

    virtual int Select();
    virtual int unSelect();
    virtual int Handle() const { return selected_;}

    // interface to disk parameter block.
    unsigned BlockShift()       const { return dpb_.BlockShift();}
    unsigned BlockMsk()         const { return dpb_.BlockMsk();}
    unsigned BlockSize() 		const { return dpb_.BlockSize();}
    unsigned DirectoryEntries() const { return dpb_.DirectoryEntries();}
    unsigned LastBlock()		const { return dpb_.MaxBlock();}
    unsigned ReservedTracks()	const { return dpb_.ReservedTracks();}
    unsigned SectorsTrack()     const { return dpb_.SectorsTrack();}
    unsigned BlockPtrType()     const { return dpb_.BlockPtrType();}
    unsigned A01()				const { return (dpb_.A01());}

    virtual unsigned translate(unsigned long sector) const { assert(sector != 0); return stbl_[(sector-1) % SectorsTrack()];}

    const char *path_;      // unix diroctory

protected:
private:
    ostype_t os_;           // kind of os (only cp/m 2.2 supported yet)
    dpb	dpb_;               // disk parameter block
    unsigned *stbl_;        // sector translation table
    int    selected_;
};

class dskimage: public simdrive
{
public:
    dskimage(const char* path, ostype_t os, const dpb &dpb, const uint8_t *stbl);
    ~dskimage();

    // read/write to cp/m disk
    virtual unsigned Read(unsigned track, unsigned sector, uint8_t *dma);
    virtual unsigned Write(unsigned track, unsigned sector, const uint8_t *dma);
private:
    fstream image_;
};

class disk : public simdrive
{
public:
    // create disk vrom unix directory
    // path to unix directory, kind of os, disk parameter block, sector translation list.
    disk(const char* path, ostype_t os, const dpb &dpb, const uint8_t *stbl_);
    virtual ~disk();


    // read/write to cp/m disk
    unsigned Read(unsigned track, unsigned sector, uint8_t *dma);
    unsigned Write(unsigned track, unsigned sector, const uint8_t *dma);
    // handles all writes of chached sectors, renames files ...changes userid
    unsigned Flush();

    virtual int Select() {return 1;}
    virtual int unSelect() { return 0;}
    virtual int Handle() const { return -1;}

    // start sector of director.
    unsigned DirStart()         const { return SectorsTrack() * ReservedTracks() * 128;}

    void    dump(ostream &os)
    {
        os.seekp(DirStart());
        for(unsigned i = 0; i < DirectoryEntries(); i++)
        {
            dir_.at(i).dump(os);
            LOG(LINF) << dir_.at(i);
        }
    }

    void    HexDump( ostream& os, const uint8_t* data, unsigned size)
    {
        stringstream hs;
        string as;
        os << endl;
        for(unsigned i=0; i < size; i++)
        {
            if(!(i % 16))
            {
                if(i)
                {
                    os << setw(53) << hs.str() << " : \'" << as << '\'' << endl;
                    hs.str("");
                    as.clear();
                }
                hs << hex << setw(4) << setfill('0') << i << ':';
            }
            hs << setw(2) << hex << (unsigned) data[i] << ' ';
            as += isprint(data[i]) ? data[i] : '.';
        }

        if(!as.empty())
            os << setw(53) << hs.str() << " : \'" << as << '\'' << endl;
    }

protected:

    // get next free (unused) extend
    cpmdir* NewDir();

    unsigned WriteHost(const string& hfile, unsigned long pos, const uint8_t *data128);
    unsigned ReadHost(const string& hfile, unsigned long pos, uint8_t *data128);

    // read in the host directory.
    void readhost(const string path);

    // add one host file to the cp/m disk
    void addfile(const string &path, cpmdir::status_t userno, const string &cpmname);

    // Retranslates sector and returns the absolute sector on linearised cp/m disk emulation.
    unsigned long GetAbsSector(unsigned track, unsigned sector)
    {
        unsigned map = (track >= ReservedTracks()) ? smap_[sector-1] : sector;
        LOG(LINF) << " map " << map;
        return track * SectorsTrack() + map -1;
    }

    string mkhostname(string cpmname)
    {
        string res(path_);
        res.append("/");

        // remove user no 00 on host files
        if(cpmname.compare("00_"))
            cpmname.erase(0,3);

        for( string::const_iterator cit = cpmname.begin(); cit != cpmname.end(); cit++)
            if( !isspace(*cit))
                res += *cit;
        return res;
    }

private:
    vector<cpmdir>dir_;     // cp/m directory entries (extents)
    map<string, string> cpm2unix_;  // maps unix filename to cp/m filename (hash)
    unsigned freeblock_;    // next free block during initializing
    string bootblock_;      // name of unix file to hold the reserved tracks content
    unsigned datastart_;    // offset to start of data sectors.
    unsigned dirstart_;     // offset to start of directory sectors;
    map<unsigned long, uint8_t*> cache_; // write data sector chache
    unsigned *smap_;        // sector re translation table
};
#endif /* BIOSAPI_H_ */



