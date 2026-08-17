/*
 * biosapi.cpp
 *
 *  Created on: 06.05.2015
 *      Author: juergen
 *
 * Declaration file for the cp/m disk Simulator.
 * Copyright (C) 2015  Juergen Willi Sievers @notwendiger.
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
 * by Twitter @notwendiger
 *
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <dirent.h>
#include <unistd.h>
#include <cstddef>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <set>
#include <algorithm>
#include <climits>
#include <cerrno>
#include <memory>
#include <system_error>

#include "biosapi.h"
#include "cpm.h"

void disk::addfile(const string &path, cpmdir::status_t userno, const string &cpmname)
{
    // check if a file with this name already exist
    cpmdir tmp(BlockPtrType());
    tmp.SetStatus(userno);
    tmp.SetName(cpmname);

    LOG(LINF) << "add user " << userno << ", cpmfile " << cpmname << ", host-path " << path;

    string s = tmp.GetName();

    map<string, string>::const_iterator it = cpm2unix_.find(s);
    if(  it != cpm2unix_.end())
    {
        // file exist so skip it.
        LOG(LWRN) << " skip";
        return;
    }

    // get file parameters
    struct stat st;
    if( stat(path.c_str(), &st))
    {
        LOG(LINF) << " Can't stat " << endl;
        throw;
    }
    off_t size = st.st_size;    // size in bytes

    unsigned long blocks = (size + BlockSize() - 1) / BlockSize();  // size in blocks
    unsigned dirs = 1 + blocks / BlockPtrType();                    // extents occupied by this file

    LOG(LINF) << " size " << size << " blks " << blocks << " exts " << dirs << endl;

    // fill up the needed extents
    for( unsigned d = 0; d < dirs; d++)
    {
        cpmdir *dir = NewDir(); // get new directory entry (extent)

        if(!dir)
        {
            LOG(LERR) << "Nicht genügene Verzeichniseinträge vorhande." << endl;
            throw;
        }

        // Setze Parameter
        dir->SetXl( d & 0x0F);
        dir->SetXh((d >> 4) & 0x1F);

        dir->SetStatus(userno);
        dir->SetName(cpmname);

        //dir->SetPointerSize(BlockPtrType());

        unsigned n = ((blocks < BlockPtrType()) ? blocks : BlockPtrType());

        dir->SetRc( n * BlockSize() / 128);
        dir->SetBc( 0); // CP/M 2.2 size%128 vor cp/m above

        // Fill up the extent with free block-number
        for( unsigned i = 0; i < n; i++, blocks--)
        {
            if(BlockPtrType() == 8)
                dir->SetALWord(i, freeblock_++);
            else
                dir->SetALByte(i, freeblock_++);

            if(freeblock_ >= LastBlock())
            {
                LOG(LINF) << "Nicht genügend Speicherpaltz" << endl;
                throw;
            }
        }

        LOG(LINF) << "extent: " << *dir << endl;
    }

    // insert unix filename by cpmfile-name on the filename map-table
    cpm2unix_[s] = path;
}

// get e free extend
cpmdir* disk::NewDir()
{
    unsigned i;

    // search list of extends
    for( i=0; i < DirectoryEntries(); i++)
    {
        if(dir_.at(i).GetStatus() == cpmdir::UNUSED)
        {
            dir_.at(i).SetPointerType(BlockPtrType());
            return &dir_.at(i);
        }
    }
    return 0;
}

// Scann host (unix) dirctory and insert files as the given user in the
// cp/m directory
void disk::readhost(const string dpath)
{
    LOG(LINF) << "init disk from " << dpath << endl;

    struct DirCloser
    {
        void operator()(DIR *dir) const noexcept
        {
            if(dir)
                closedir(dir);
        }
    };

    // Import one host directory as one CP/M user area.
    const auto readuser = [this](const string& userpath, cpmdir::status_t userno)
    {
        std::unique_ptr<DIR, DirCloser> dir(opendir(userpath.c_str()));
        if(!dir)
        {
            const int error = errno;
            LOG(LERR) << "Can't open host directory " << userpath << endl;
            throw std::system_error(error, std::generic_category(), "opendir " + userpath);
        }

        for(;;)
        {
            errno = 0;
            struct dirent *result = readdir(dir.get());
            if(!result)
            {
                const int error = errno;
                if(error != 0)
                    throw std::system_error(error, std::generic_category(), "readdir " + userpath);
                break;
            }

            if(!strcmp(result->d_name, ".") || !strcmp(result->d_name, ".."))
                continue;

            const string filename = userpath + "/" + result->d_name;

            struct stat filestat;
            if(stat(filename.c_str(), &filestat))
            {
                const int error = errno;
                throw std::system_error(error, std::generic_category(), "stat " + filename);
            }

            // Only regular files are CP/M files. Nested directories are ignored.
            if(!S_ISREG(filestat.st_mode))
            {
                LOG(LINF) << "skip " << filename << endl;
                continue;
            }

            const char *dname = result->d_name;

            // not allowed on CP/M file names
            static const char noname[] = " <>,;:=?*[].";

            string cpmname("            ");
            int i;

            // copy basename uppercase
            for(i = 0; i < 8 && *dname && !strchr(noname, *dname); i++, dname++)
                cpmname[i] = static_cast<char>(toupper(static_cast<unsigned char>(*dname)));

            // skip all files starting with an unallowed character.
            if(!i)
                continue;

            while(*dname && *dname != '.')
                dname++;

            // if first unallowed character was the dot then file extension follows
            if(*dname == '.')
            {
                dname++;
                cpmname[8] = '.';
                for(i = 0; i < 3 && *dname && !strchr(noname, *dname); i++, dname++)
                    cpmname[9+i] = static_cast<char>(toupper(static_cast<unsigned char>(*dname)));
            }

            addfile(filename, userno, cpmname);
        }
    };

    // New host layout:
    //
    //   <drive>/FILE.EXT       -> CP/M user 0
    //   <drive>/1/FILE.EXT     -> CP/M user 1
    //   ...
    //   <drive>/15/FILE.EXT    -> CP/M user 15
    //
    // First import regular files in the drive root as user 0.
    readuser(dpath, cpmdir::CPMUSRNO);

    // Then import numeric user directories 1..15 when present.
    for(unsigned user = 1; user <= 15; ++user)
    {
        const string userpath = dpath + "/" + std::to_string(user);

        struct stat filestat;
        if(stat(userpath.c_str(), &filestat))
        {
            if(errno == ENOENT)
                continue;

            const int error = errno;
            throw std::system_error(error, std::generic_category(), "stat " + userpath);
        }

        if(!S_ISDIR(filestat.st_mode))
        {
            LOG(LWRN) << "skip CP/M user path (not a directory): " << userpath << endl;
            continue;
        }

        readuser(userpath, static_cast<cpmdir::status_t>(user));
    }
}

simdrive::simdrive(const char* path, ostype_t os, const dpb &dpb, const uint8_t *stbl)
    :path_(path)
    ,os_(os)
    ,dpb_(dpb)
    ,stbl_(new unsigned[SectorsTrack()])
    ,selected_(-1)
{
    LOG(LINF) << "create disk " << path << endl
         << "dpb " << endl << dpb << endl
         << "translation = [" << endl;

    for(unsigned i = 0; i < SectorsTrack(); i++)
    {
        stbl_[i] = stbl? stbl[i] : i+1;
        LOG(LINF) << (char*)(!(i % 8)? "\n" : ", ")
             << dec << setw(3) << i+1 << " => " << setw(3) << (unsigned) stbl_[i];
    }
    LOG(LINF) << endl;
}

simdrive::~simdrive()
{
    Flush();
    delete [] stbl_;
}

disk::disk(const char* path, ostype_t os, const dpb &dpb, const uint8_t *stbl_)
    :simdrive(path, os, dpb, stbl_)
    ,dir_(DirectoryEntries())
    ,freeblock_(0)
    ,bootblock_(path)
    ,smap_(new unsigned[SectorsTrack()])
{

    if(BlockPtrType() != DEFAULTPTRCNT)
        for( unsigned i=0; i < DirectoryEntries(); i++)
        {
            dir_.at(i).SetPointerType(BlockPtrType());
        }

     for(unsigned i = 1; i <= SectorsTrack(); i++)
        smap_[translate(i)-1] = i;

     LOG(LINF) << "re-translation = [" << endl;
     for(unsigned i = 0; i < SectorsTrack(); i++)
         LOG(LINF) << (char*)(!((i) % 8)? "\n" : ", ")
              << dec << setw(3) << i+1 << " => " << setw(3) << (unsigned) smap_[i];

    LOG(LINF) << ']' << endl;

    uint16_t a01 = A01();
    while(a01)
    {
        freeblock_++;
        a01 <<= 1;
    }

    datastart_= freeblock_ << BlockShift();
    dirstart_ = (ReservedTracks() * SectorsTrack());
    bootblock_.append("/");
    bootblock_.append(CPMSYS);
    LOG(LINF) << "dir-blk " << freeblock_ << ", sys-file " << bootblock_ << endl;

    readhost(path);
}

disk::~disk()
{
    unSelect();
    delete [] smap_;
}

unsigned disk::Flush()
{
    LOG(LINF) << "Flush cache from Drive " << path_ << endl;
    map<unsigned long, uint8_t*>::const_iterator citcache;

    vector<cpmdir>::const_iterator itdir;

    for(citcache = cache_.begin(); citcache != cache_.end(); citcache++)
    {
        unsigned long sid = citcache->first;
        uint8_t *data = citcache->second;
        unsigned long block= sid >> BlockShift();
        unsigned      soffs= (sid & BlockMsk()) * 128;

        for(itdir = dir_.begin(); itdir != dir_.end(); itdir++)
        {
            cpmdir dir = *itdir;

            if(dir.GetStatus() == cpmdir::UNUSED)
                continue;

            unsigned blkoffset = dir.FindBlockIndex(block,BlockPtrType());
            if(blkoffset--)
            {
                string uname = cpm2unix_.at(dir.GetName());
                unsigned long offset = blkoffset * BlockSize() + soffs;
                WriteHost(uname, offset, data);
                delete [] data;
                cache_.erase(sid);
            }
        }
    }

    LOG(LINF) << cache_.size() << " Sectors left on cache for drive " << path_ << endl;
    return 0;
}

unsigned disk::WriteHost(const string& hfile, unsigned long pos, const uint8_t *data128)
{
    unsigned res = 1;

    LOG(LINF) << "Write file " << hfile << " offset " << pos << endl;
    //HexDump(LOG(LINF), data128, 128);

    fstream ofs(hfile);

    if(ofs)
    {
        if(ofs.seekp(pos))
        {
            if( !ofs.write((const char*) data128, 128))
                LOG(LINF) << "ERROR write";
            else
                res = 0;
        }
        else
            LOG(LINF) << "ERROR seek";
        ofs.close();
    }
    else
        LOG(LINF) << "ERROR open";

    LOG(LINF) << endl;

    return res;
}

unsigned disk::ReadHost(const string& hfile, unsigned long pos, uint8_t *data128)
{
    unsigned res = 1;

    LOG(LINF) << "Read file " << hfile << " offset " << pos << endl;
    ifstream ifs(hfile);

    if(ifs)
    {
        if(ifs.seekg(pos))
        {
            if( !ifs.read((char*) data128, 128) && !ifs.eof())
                LOG(LINF) << "ERROR read";
            else
            {
                res = 0;
                //HexDump(LOG(LINF), data128, 128);
            }
        }
        else
            LOG(LINF) << "ERROR seek";
        ifs.close();
    }
    else
        LOG(LINF) << "ERROR open";

    LOG(LINF) << endl;

    return res;
}

unsigned disk::Write(unsigned track, unsigned sector, const uint8_t *dma)
{
    unsigned res = 0;
    string   hash;

    map<unsigned,uint8_t*>::const_iterator cache;

    LOG(LINF) << "Write: " << path_
         << ", track=" << dec << setw(4) << setfill(' ') << track
         << ", sector=" << setw(2) << sector << ", ";

    unsigned long absector = GetAbsSector(track,sector);

    if( absector < dirstart_)
    {
        LOG(LINF) << ", System track " << bootblock_ << endl;
        //HexDump(LOG(LINF), dma, 128);
        if(WriteHost(bootblock_, absector << 7, dma))
            throw;
    }
    else
    {
        absector -= dirstart_;      // Block 0
        if(absector < datastart_)
        {
            unsigned extno = absector << 2; // first extent of abs sector
            LOG(LINF) << ", Directory " << endl;

            for( unsigned idx = 0; idx < 4; idx++)  // 4 extents pro sector
            {
                cpmdir &extold = dir_.at(extno+idx);// existing extend
                cpmdir extnew(dma+idx*32, BlockPtrType());// newextend on sector

                LOG(LINF) << "old " << extold << endl
                     << "new " << extnew << endl;

                if(extold == extnew)    // nothing changed on this extent
                    continue;

                if(extnew.GetStatus() != cpmdir::UNUSED || extold.GetStatus() != cpmdir::UNUSED)
                {
                    if(extnew.GetStatus() == cpmdir::UNUSED && extold.GetStatus() != cpmdir::UNUSED)
                    {
                        if(!extold.GetExtNumber())
                        {
                            // delete host file
                            string uname = cpm2unix_.at(extold.GetName());
                            LOG(LINF) << "Remove host file " << uname;
                            if(remove(uname.c_str()))
                            {
                                LOG(LINF) << " ERROR remove." << endl;
                                throw;
                            }
                            cpm2unix_.erase(extold.GetName());
                        }
                    }
                    else if( extnew.GetStatus() != cpmdir::UNUSED && extold.GetStatus() == cpmdir::UNUSED)
                    {
                        if( !extnew.GetExtNumber())
                        {
                            // create host file
                            string uname(mkhostname(extnew.GetName()));
                            LOG(LINF) << "Create file on host " << uname;
                            ofstream ois(uname);
                            if(!ois)
                            {
                                LOG(LINF) << " ERROR create." << endl;
                                throw;
                            }
                            ois.flush();
                            ois.close();
                            cpm2unix_[extnew.GetName()] = uname;
                        }
                    }
                    else if( extnew.GetStatus() != cpmdir::UNUSED && extold.GetStatus() != cpmdir::UNUSED
                             && extnew.GetName() != extold.GetName())
                    {
                        if(!extold.GetExtNumber() && !extnew.GetExtNumber())
                        {
                            // rename host file
                            string unameold(cpm2unix_.at(extold.GetName()));
                            string unamenew(mkhostname(extnew.GetName()));
                            LOG(LINF) << ", rename a file host " << unameold << " to " << unamenew;
                            if(rename(unameold.c_str(), unamenew.c_str()))
                            {
                                LOG(LINF) << " ERROR rename." << endl;
                                throw;
                            }
                            cpm2unix_.erase(extold.GetName());
                            cpm2unix_[ extnew.GetName()] =unamenew;
                        }
                    }

                }
                extold = extnew;
            }
            res = Flush();
        }
        else
        {
            // data sector
            LOG(LINF) << ", data" << endl;
            map<unsigned long, uint8_t*>::iterator it = cache_.find(absector);

            uint8_t *data = (it == cache_.end())? new uint8_t[128] : it->second;
            memcpy(data, dma, 128);
            cache_[absector] = data;
            //HexDump(LOG(LINF), dma, 128);
        }
    }
    LOG(LINF) << endl;
    return res;
}

unsigned disk::Read(unsigned track, unsigned sector, uint8_t *dma)
{
    unsigned        res = 0;
    string          hash;
    map<unsigned,uint8_t*>::const_iterator cache;

    LOG(LINF) << "Read: " << path_
         << ", track=" << dec << setw(4) << setfill(' ') << track
         << ", sector=" << setw(2) << sector << ", ";

    unsigned long absector = GetAbsSector(track,sector);

    if( absector < dirstart_)
    {
        LOG(LINF) << ", System track " << bootblock_ << endl;
        if(ReadHost(bootblock_, absector << 7, dma))
            throw;
        HexDump(LOG(LINF), dma, 128);
    }
    else
    {
        absector -= dirstart_;
        if(absector < datastart_)
        {
            unsigned extno = absector << 2; // first extent index of sector
            LOG(LINF) << ", Directory " << endl;
            for ( unsigned idx = 0; idx < 4; idx++)
            {
                cpmdir &ext = dir_.at(extno+idx);    // existing extend
                ext.read(dma + idx*32);
                LOG(LINF) << ext << endl;
            }
        }
        else
        {
            LOG(LINF) << ", data" << endl;
            // data sector
            map<unsigned long, uint8_t*>::const_iterator cit = cache_.find(absector);
            if(cit != cache_.end())
            {
                memcpy(dma, cit->second, 128);
            }
            else
            {
                unsigned long block= absector >> BlockShift();
                unsigned      soffs= (absector & BlockMsk()) * 128;

                for(vector<cpmdir>::const_iterator itdir = dir_.begin(); itdir != dir_.end(); itdir++)
                {
                    cpmdir dir = *itdir;
                    if(dir.GetStatus() != cpmdir::UNUSED)
                    {
                        unsigned blkoffset = dir.FindBlockIndex(block,BlockPtrType());
                        if(blkoffset--)
                        {
                            string uname;
                            string dirname = dir.GetName();
                            try {
                            uname = cpm2unix_.at(dir.GetName());
                            } catch( ... )
                            {
                                dir.SetStatus(cpmdir::UNUSED);
                            }

                            unsigned long offset = blkoffset * BlockSize() + soffs;
                            ReadHost(uname, offset, dma);
                        }
                    }
                }
            }
        }
    }
    LOG(LINF) << endl;
    return res;
}

int simdrive::Select()
{
    if(selected_ == -1)
    {
        if(-1 == (selected_ = open(path_,O_RDWR)))
        {
            perror("open");
           throw;
        }
        if(flock(selected_, LOCK_EX) == -1)
        {
            const int error = errno;
            close(selected_);
            selected_ = -1;
            throw std::system_error(error, std::generic_category(), "flock");
        }
    }
    return 0;
}

int simdrive::unSelect()
{
    if(selected_ != -1)
    {
        Flush();

        if(flock(selected_,LOCK_UN))
        {
            perror("flock");
           throw;
        }
        close(selected_);
        selected_ = -1;
    }
    return 0;
}



dskimage::dskimage(const char* path, ostype_t os, const dpb &dpb, const uint8_t *stbl)
    :simdrive(path, os, dpb, stbl)
    ,image_(path)
{
    if(!image_)
        throw;

}

dskimage::~dskimage()
{
    if(image_.is_open())
        image_.close();
}

unsigned dskimage::Read(unsigned track, unsigned sector, uint8_t *dma)
{
    unsigned long offset = 128 * (track * SectorsTrack() + sector -1);
    LOG(LINF) << "Read: " << path_
         << ", track=" << dec << setw(4) << setfill(' ') << track
         << ", sector=" << setw(2) << sector << endl;

    return (image_.seekg(offset) && image_.read((char*)dma,128)) ? 0:1;
}

unsigned dskimage::Write(unsigned track, unsigned sector, const uint8_t *dma)
{
    unsigned long offset = 128 * ( track * SectorsTrack() + sector -1);
    LOG(LINF) << "Write:" << path_
         << ", track=" << dec << setw(4) << setfill(' ') << track
         << ", sector=" << setw(2) << sector << endl;
    return (image_.seekp(offset) && image_.write((const char*)dma,128)) ? 0:1;
}



cpmdir::cpmdir(unsigned pointersize)
    :St_(UNUSED)
    ,Fx_{' ',' ',' ',' ',' ',' ',' ',' ',}
    ,Ex_{' ',' ',' '}
    ,Xl_(0)
    ,Bc_(0)
    ,Xh_(0)
    ,Rc_(0)
    ,Al_{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
    ,pointersize_(pointersize)
{
}

cpmdir::cpmdir(const uint8_t* dd, unsigned pointersize)
    :St_(dd[0])
    ,Xl_(dd[12])
    ,Bc_(dd[13])
    ,Xh_(dd[14])
    ,Rc_(dd[15])
    ,pointersize_(pointersize)
{
    memcpy(Fx_,dd+1,8);
    memcpy(Ex_,dd+9,3);
    memcpy(Al_.b,dd+16,16);
}

cpmdir::~cpmdir()
{

}

unsigned cpmdir::FindBlockIndex(unsigned block,unsigned bps)
{
    if(GetStatus() != UNUSED)
    {
        for(unsigned b = 0; b < bps; b++)
        {
            unsigned blkno;

            if(bps == 16)
                blkno = GetALByte(b);
            else
                blkno = GetALWord(b);

            if(blkno == block)
            {
                return b + 1 + GetExtNumber() * bps;
            }
        }
    }
    return 0;
}

string  cpmdir::GetName() const
{
    string s("");

    stringstream ss;
    ss << hex << setw(2) << setfill('0') << GetStatus() << '_';
    s = ss.str();
    s.append((const char*)Fx_,8);
    if(strncmp((const char*)Ex_,"   ",3))
        s.append(".");
    s.append((const char*)Ex_,3);

    return s;
}

void cpmdir::SetName(const string& s)
{
    if(s.length() != 12)
        throw;

    const char* n = s.c_str();
    memcpy(Fx_,n,8);
    memcpy(Ex_,n+9,3);
}

dpb::dpb(const dpb_t& data)
    :dpb_(data)
{

}

dpb::~dpb()
{

}
