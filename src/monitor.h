#ifndef MONITOR_H
#define MONITOR_H
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
 * Monitor vor CP/M 2.2 emulation system
 * See: https://sourceforge.net/projects/zilogz80/
 *
*/

#include "zilogz80.h"
#include <string>
#include <cstdint>

using namespace std;

class monitor
{
public:
    monitor(IntelCPU &cpu);
    int run();
protected:
private:
    IntelCPU &cpu_;
    uint16_t pc_;
};

#endif // MONITOR_H
