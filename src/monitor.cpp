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

#include "monitor.h"

#include <iostream>
#include <sstream>

monitor::monitor(IntelCPU &cpu)
    :cpu_(cpu)
{

}

int monitor::run()
{
    int ret = 0;
    cout << endl << "CP/M paralell Monitor V 1.0" << endl
         << "enter ? for help." << endl;
    do {
        cout << endl << "#:" << flush;

        stringstream input;
        input.unsetf(std::ios_base::basefield);

        string line;
        getline(cin, line);

        input << line;

        char command;
        input >> std::skipws >> command;

        while(input)
        {
            switch(toupper(command))
            {
                case '?':
                    cout    << endl << "X exit programm."
                            << endl << "D [hex-addr [bytes]]"

                            << endl;
                break;
                case 'X': return 0;
                case 'D':
                {

                    uint16_t param;
                    input >> param;
                    if(input)
                    {
                        pc_ = param;
                        input >> param;
                    }
                    if(!input)
                    {
                        param = 128;
                        input.clear();
                    }
                    stringstream mem;
                    uint16_t cnt = param;

                    cpu_.readmem(mem, pc_, param);

                    string ascii;

                    for( unsigned n = 0; n < cnt; n++ )
                    {
                        if(!(n%16))
                            cout << endl << hex << setw(4) << setfill('0') << pc_+n << ':';
                        else
                            cout << ',';
                        unsigned char c;
                        mem >> c;
                        cout << hex << setw(2) << (unsigned) c;
                        ascii += (char) (isprint(c) ? c:'.');
                        if((n%16) == 15)
                        {
                            cout << " :\'" << ascii << "\'";
                            ascii.clear();
                        }
                    }
                    pc_ += cnt;
                }

            }
            cout << endl;
            input >> std::skipws >> command;
        }

    }while(!ret);
    return ret;
}
