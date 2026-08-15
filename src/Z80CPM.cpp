//============================================================================
// Name        : Z80CPM.cpp
// Author      : extended by j.sievers
// Version     :
// Copyright   : copyright by everyone who are like to have it
// Description : Hello World in C++, Ansi-style
//============================================================================

#include "cpm.h"
#include "monitor.h"
#include "zilogz80.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include <getopt.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>

std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems)
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string& s, char delim)
{
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

// Console control.
pthread_t telnet(long portno);
void stoptelnet();
bool IsConnected();

using namespace std;

static IntelCPU cpu;

MSGLEVEL verbosity = LOFF;
MSGLEVEL debug = LOFF;
ofstream nullstream("/dev/null");

static string console("/usr/bin/plink -fg green -bg black -T \"CP/M 2.2\" -load CPM22");
static string host("localhost");
#ifdef CPM22_DEFAULT_DISKPATH
static string disks(CPM22_DEFAULT_DISKPATH);
#else
static string disks(string(getenv("HOME")) + string("/.cpm"));
#endif
static string rcfile(string(getenv("HOME")) + string("/.cpmrc"));
static string procname;
static int port = 1234;
static double z80mhz=0.0;
static struct option long_options[] = {
    {"verbos", optional_argument, 0, 'l'},   {"debug", optional_argument, 0, 'g'},
    {"diskpath", required_argument, 0, 'd'}, {"console", required_argument, 0, 'c'},
    {"host", required_argument, 0, 'h'},     {"port", required_argument, 0, 'p'},
    {"rcfile", required_argument, 0, 'r'},   {"z80mhz", required_argument, 0, 'z'},
    {"help", no_argument, 0, '?'},           {0, 0, 0, 0}};
static const char* short_options = "l:g:d:c:h:p:r:z:?";

int main(int argc, char** argv)
{
    int op;

    char* tmp = strrchr(argv[0], '/');

    procname = tmp ? ++tmp : argv[0];

    while (1)
    {
        int option_index = 0;
        op = getopt_long(argc, argv, short_options, long_options, &option_index);

        if (op == -1)
            break;

        switch (op)
        {
        case 0:
            if (optarg)
                *long_options[option_index].flag = atoi(optarg);
            break;

        case 'l': // log level
            verbosity = (MSGLEVEL)atoi(optarg);
            break;

        case 'g': // debug level
            debug = (MSGLEVEL)atoi(optarg);
            break;

        case 'd': // disk path
            disks = optarg;
            break;

        case 'c': // console
            console = optarg;
            console.replace(console.begin(), console.end(), ',', ' ');
            break;

        case 'h':
            host = optarg;
            break;

        case 'p':
            if (!isdigit(*optarg))
            {
                struct servent* servinfo = getservbyname(optarg, "tcp");
                if (servinfo)
                    port = ntohs(servinfo->s_port);
                else
                {
                    cerr << "Console-port --" << long_options[option_index].name
                         << "not on /etc/services." << endl;
                    return -1;
                }
            }
            else
                port = atoi(optarg);
            break;
        case 'r':
            rcfile = optarg;
            break;
        case 'z':
            z80mhz = atof(optarg);
            break;
        case '?':
            cout << "Use:" << procname
                 << "\t[--verbos [level]]              Verbosity-level 0=off .. 3=max." << endl
                 << "\t[--debug  [level]]              Debug-level 0=off .. 3=max." << endl
                 << "\t[--diskpath | -d directory]     Set base of CP/M disk-images." << endl
                 << "\t[--console | -c prog[,arg,...]] Set console terminal program and arguments."
                 << endl
                 << "\t[--host | -h hostname|ip]       Set hostname or ip-address." << endl
                 << "\t[--port | number|service-name]  Set portnumber or servie name." << endl
                 << "\t[--rcfile | -r filename]        Set configuration file." << endl
                 << "\t[--z80mhz | -z mhz]             Set Z80 Tackt." << endl
                 << "\t[--help | -?]                   Disply this help text." << endl;
            break;

        default:
            cerr << "Unknown option " << (char)op << endl;
        }
    }

    LOG(LINF) << "Proc:" << procname << endl
              << " CP/M 2.2 Emulation (C) 2010-2015 GPL by J.Sievers <JSievers@NadiSoft.de>" << endl
              << "   Z80 Emulator " << cpu.version() << " Copyright " << cpu.copyright() << endl;

    LOG(LINF) << "verbosity=" << verbosity << endl
              << "debug=" << debug << endl
              << "console" << console << endl
              << "host" << host << ':' << port << endl
              << "disks" << disks << endl
              << "rcfile" << rcfile << endl
              << "z80mhz" << z80mhz << endl;

    if (chdir(disks.c_str()))
    {
        perror("Can't change to cpm base directory.");
        return -1;
    }

    monitor moni(cpu);

    telnet(port);

    int pid = fork();
    if (pid < 0)
    {
        perror("fork");
        exit(-1);
    }
    if (!pid)
    {
        setsid();

        if (execl("/usr/bin/putty", "/usr/bin/putty", "-load", "CPM22", (char*)NULL))
        {
            perror("execl");
            exit(-1);
        }
    }

    while (!IsConnected())
        usleep(300);

    cpu.setMHz(0.0);
    cpu.powercycle();

    moni.run();

    cerr << endl << "EXIT" << endl;

    cpu.setPower(false);
    stoptelnet();
    return 0;
}
