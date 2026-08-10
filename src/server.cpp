/* A simple server in the internet domain using TCP
 The port number is passed as an argument */

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>
#include <iostream>

#include <cstdint>
#include <deque>
#include <mutex>

using namespace std;

char *hostaddrp; /* dotted decimal host addr string */

static int newsockfd = -1;
static int sockfd = -1;

#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)

static void error(const char *msg)
{
    perror(msg);
    exit(1);
}


int ConOut(int, char c)
{
    int n = 0;
    if(newsockfd == -1)
        sleep(1);
    if(newsockfd != -1)
    do
    {
        n = write(newsockfd, &c, 1);
        if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            continue;
    } while (n != 1);
    return n;
}

namespace
{
std::deque<unsigned char> consoleBuffer;
std::mutex consoleBufferMutex;

void enqueueConsoleByte(unsigned char c)
{
    std::lock_guard<std::mutex> lock(consoleBufferMutex);
    consoleBuffer.push_back(c);
}

bool dequeueConsoleByte(unsigned char &c)
{
    std::lock_guard<std::mutex> lock(consoleBufferMutex);
    if(consoleBuffer.empty())
        return false;

    c = consoleBuffer.front();
    consoleBuffer.pop_front();
    return true;
}

bool consoleByteAvailable()
{
    std::lock_guard<std::mutex> lock(consoleBufferMutex);
    return !consoleBuffer.empty();
}
}

bool IsConnected()
{
    return newsockfd != -1;
}

// Console status, return 0ffh if a character is ready, 00h if not.
int ConChk(int)
{
    return consoleByteAvailable() ? 0xff : 0;
}

char ConIn(int)
{
    unsigned char c = 0;
    if(!dequeueConsoleByte(c))
        return 0;

    if(c == 0x11)
        pthread_exit(nullptr);

    return static_cast<char>(c);
}

int console(int s)
{
    enum class TelnetState
    {
        Data,
        Iac,
        Option,
        Subnegotiation,
        SubnegotiationIac
    };

    TelnetState telnetState = TelnetState::Data;
    bool previousWasCr = false;

    for(;;)
    {
        unsigned char c = 0;
        const ssize_t n = recv(s, &c, 1, 0);

        if(n == 0)
            return 0;

        if(n < 0)
        {
            if(errno == EINTR)
                continue;
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                usleep(500);
                continue;
            }
            return errno;
        }

        // Minimal TELNET protocol filtering.  The server historically advertises
        // itself for telnet use, so negotiation bytes must never reach CP/M.
        switch(telnetState)
        {
            case TelnetState::Iac:
                if(c == 0xff)               // IAC IAC -> literal 0xff
                {
                    telnetState = TelnetState::Data;
                }
                else if(c >= 0xfb && c <= 0xfe) // WILL/WONT/DO/DONT
                {
                    telnetState = TelnetState::Option;
                    continue;
                }
                else if(c == 0xfa)          // SB
                {
                    telnetState = TelnetState::Subnegotiation;
                    continue;
                }
                else
                {
                    telnetState = TelnetState::Data;
                    continue;
                }
                break;

            case TelnetState::Option:
                telnetState = TelnetState::Data;
                continue;

            case TelnetState::Subnegotiation:
                if(c == 0xff)
                    telnetState = TelnetState::SubnegotiationIac;
                continue;

            case TelnetState::SubnegotiationIac:
                telnetState = (c == 0xf0) ? TelnetState::Data
                                           : TelnetState::Subnegotiation;
                continue;

            case TelnetState::Data:
                if(c == 0xff)
                {
                    telnetState = TelnetState::Iac;
                    continue;
                }
                break;
        }

        // CP/M expects a single carriage return.  Modern terminal clients often
        // send CR/LF (or TELNET CR/NUL).  Leaving the second byte queued makes
        // CCP's DIR command see a pending key in CHKCON and abort after one file.
        if(previousWasCr)
        {
            previousWasCr = false;
            if(c == '\n' || c == '\0')
                continue;
        }

        if(c == '\r')
        {
            enqueueConsoleByte('\r');
            previousWasCr = true;
        }
        else if(c == '\n')
        {
            enqueueConsoleByte('\r');
        }
        else
        {
            enqueueConsoleByte(static_cast<unsigned char>(c & 0x7f));
        }
    }
}

void cleanupHandler(void *)
{
    if(newsockfd != -1) close(newsockfd);
    if(sockfd != -1) close(sockfd);
}

void* server(void *portno)
{
    int optval = 1;
    socklen_t clien;
    struct sockaddr_in serv_addr, cli_addr;
    struct hostent *hostp; /* client host info */

    pthread_cleanup_push(cleanupHandler,0);

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,0);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,0);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons((long) portno);

    /* setsockopt: Handy debugging trick that lets
     * us rerun the server immediately after we kill it;
     * otherwise we have to wait about 20 secs.
     * Eliminates "ERROR on binding: Address already in use" error.
     */
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int));

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    if (listen(sockfd, 5) < 0) /* allow 5 requests to queue up */
            error("ERROR on listen");
    clien = sizeof(cli_addr);

    do
    {
        cout << "Listen on Port " << dec << (long) portno << " for console." << endl
             << "Use telnet localhost " << dec << (long) portno << endl
             << "to attach console to the CP/M system on localhost." << endl;

        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clien);
        if (newsockfd < 0)
            error("ERROR on accept");
        /*
         * gethostbyaddr: determine who sent the message
         */
        hostp = gethostbyaddr((const char *) &cli_addr.sin_addr.s_addr,
                sizeof(cli_addr.sin_addr.s_addr), AF_INET);

        if (hostp == NULL)
            error("ERROR on gethostbyaddr");

        hostaddrp = inet_ntoa(cli_addr.sin_addr);
        if (hostaddrp == NULL)
            error("ERROR on inet_ntoa\n");

        cout << "Connect console " << hostp->h_name << ':' << hostaddrp << endl;

        console(newsockfd);

        close(newsockfd);
    } while(1);
    close(sockfd);
    pthread_cleanup_pop(1);
    return 0;
}

static pthread_t pthread_con;
static pthread_attr_t attr_con;

pthread_t telnet(long portno)
{
    pthread_attr_init(&attr_con);
    pthread_create(&pthread_con, &attr_con, server, (void*) portno);
    return pthread_con;
}

void stoptelnet()
{
    pthread_cancel(pthread_con);
}

