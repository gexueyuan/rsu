/*
  NTRIP client for POSIX.
  $Id: ntripclient.c,v 1.50 2009/06/08 14:07:22 stoecker Exp $
  Copyright (C) 2003-2008 by Dirk St枚cker <soft@dstoecker.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  or read http://www.gnu.org/licenses/gpl.txt
*/

#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "serial.c"
#include "ntrip.h"

  typedef int sockettype;
  #include <signal.h>
  #include <fcntl.h>
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netdb.h>

  #define closesocket(sock)       close(sock)
  #define ALARMTIME   (2*60)
  #define myperror perror

#ifndef COMPILEDATE
#define COMPILEDATE " built " __DATE__
#endif

/* The string, which is send as agent in HTTP request */
#define AGENTSTRING "NTRIP NtripClientPOSIX"
#define TIME_RESOLUTION 125

#define MAXDATASIZE 1000 /* max number of bytes we can get at once */

/* CVS revision and version */
static char revisionstr[] = "$Revision: 1.50 $";
static char datestr[]     = "$Date: 2009/06/08 14:07:22 $";

enum MODE { HTTP = 1, RTSP = 2, NTRIP1 = 3, AUTO = 4, UDP = 5, END };

struct Args
{
  const char *server;
  const char *port;
  const char *user;
  const char *proxyhost;
  const char *proxyport;
  const char *password;
  const char *nmea;
  const char *data;
  int         bitrate;
  int         mode;

  int         udpport;
  int         initudp;
  enum SerialBaud baud;
  enum SerialDatabits databits;
  enum SerialStopbits stopbits;
  enum SerialParity parity;
  enum SerialProtocol protocol;
  const char *serdevice;
  const char *serlogfile;
};

#define GPS_BUFF_SIZE 256
#define GPS_PIPE 5

#ifndef __TRUE
 #define __TRUE         1
#endif
#ifndef __FALSE
 #define __FALSE        0
#endif

int IsLocate = __FALSE;
int Get_GGA = __FALSE;

typedef enum{
    GPS_PACK_UNKNOWN = 0,
    GPS_PACK_GPGGA,
    GPS_PACK_GPGSA,
    GPS_PACK_GPGSV,
    GPS_PACK_GPRMC,
    GPS_PACK_GPVTG
} e_nmea_type;

typedef struct {
	uint8_t Flag;
	uint8_t Buf[GPS_BUFF_SIZE];
	uint32_t Len;
} t_buff;

typedef struct {
	uint8_t Pipe;
	t_buff PpBuf[GPS_PIPE];
}t_gps_buff;

t_gps_buff __GPSBuff;
char tmp;
char gga_buff[GPS_BUFF_SIZE];

/* option parsing */
#ifdef NO_LONG_OPTS
#define LONG_OPT(a)
#else
#define LONG_OPT(a) a
static struct option opts[] = {
{ "bitrate",    no_argument,       0, 'b'},
{ "data",       required_argument, 0, 'd'}, /* compatibility */
{ "mountpoint", required_argument, 0, 'm'},
{ "initudp",    no_argument,       0, 'I'},
{ "udpport",    required_argument, 0, 'P'},
{ "server",     required_argument, 0, 's'},
{ "password",   required_argument, 0, 'p'},
{ "port",       required_argument, 0, 'r'},
{ "proxyport",  required_argument, 0, 'R'},
{ "proxyhost",  required_argument, 0, 'S'},
{ "user",       required_argument, 0, 'u'},
{ "nmea",       required_argument, 0, 'n'},
{ "mode",       required_argument, 0, 'M'},
{ "serdevice",  required_argument, 0, 'D'},
{ "baud",       required_argument, 0, 'B'},
{ "stopbits",   required_argument, 0, 'T'},
{ "protocol",   required_argument, 0, 'C'},
{ "parity",     required_argument, 0, 'Y'},
{ "databits",   required_argument, 0, 'A'},
{ "serlogfile", required_argument, 0, 'l'},
{ "help",       no_argument,       0, 'h'},
{0,0,0,0}};
#endif
#define ARGOPT "-d:m:bhp:r:s:u:n:S:R:M:IP:D:B:T:C:Y:A:l:"

int stop = 0;
int sigstop = 0;
#ifdef __GNUC__
static __attribute__ ((noreturn)) void sighandler_alarm(
int sig __attribute__((__unused__)))
#else /* __GNUC__ */
static void sighandler_alarm(int sig)
#endif /* __GNUC__ */
{
  if(!sigstop)
    fprintf(stderr, "ERROR: more than %d seconds no activity\n", ALARMTIME);
  else
    fprintf(stderr, "ERROR: user break\n");
  exit(1);
}

#ifdef __GNUC__
static void sighandler_int(int sig __attribute__((__unused__)))
#else /* __GNUC__ */
static void sighandler_alarm(int sig)
#endif /* __GNUC__ */
{
  sigstop = 1;
  alarm(2);
  stop = 1;
}


void nmea_parse(uint8_t *buff, uint32_t len)
{
    uint8_t crcCk = 0x00;
    uint8_t crcCp = 0x00;
    uint32_t index = 0;
    float latitude = 0.0;
    float longitude = 0.0;
    float speed = 0.0;
    float heading = 0.0;
    float accu = 0.0;
    char *pStrS = NULL;
    char *pStrE = NULL;
    e_nmea_type CurPackType = GPS_PACK_UNKNOWN;
    int i;

    if (memcmp(buff, "$GPRMC", 6) == 0){
        CurPackType = GPS_PACK_GPRMC;
    }
    else if (memcmp(buff, "$GPGSA", 6) == 0){
        CurPackType = GPS_PACK_GPGSA;
    }
    else if (memcmp(buff, "$GPGGA", 6) == 0){
        CurPackType = GPS_PACK_GPGGA;
    }
    else return;
    //if (len <= 50) return;

    if (buff[len - 5] != '*') {
        //fprintf(stderr,"NMEA->No *\n");
        return;
    }

    //Check CRC
    for (index = 1; index < len -5; index++) {
        crcCk ^= buff[index];
    }

    //Convent
    if (isdigit(buff[len - 4])) {
        crcCp = (buff[len - 4] << 4) & 0xf0;
    }
    else if (isalpha(buff[len - 4])) {
        crcCp = ((buff[len - 4] << 4) + 0x90 ) & 0xf0;
    }
    else {
         fprintf(stderr,"NMEA->Error 0\n");
        return;
    }

    if (isdigit(buff[len - 3])) {
        crcCp |= (buff[len - 3] & 0x0f);
    }
    else if (isalpha(buff[len - 3])) {
        crcCp |= ((buff[len - 3] + 0x09 ) & 0x0f);
    }
    else {
         fprintf(stderr,"NMEA->Error 1\n");
        return;
    }

    //Compare
    if (crcCk != crcCp) {
         fprintf(stderr,"NMEA->数据帧CRC校验错误\n");
        return;
    }

    if (CurPackType == GPS_PACK_GPRMC){
        if (IsLocate != __TRUE) {
            //fprintf(stdout,"NMEA->定位精度不够\n");
            return;
        }

        pStrS = (char *)buff;
        if ((pStrS = strchr((char *)buff, ',')) != NULL) {
            pStrS++;
            if ((pStrS = strchr(pStrS, ',')) != NULL) {
                pStrS++;
            }
            else return;
        }
        else return;

        if ((*pStrS) == 'A') {
            
        }
        else {
             IsLocate = __FALSE;
             //fprintf(stderr,"NMEA->未定位\n");
        }
    }
    else if (CurPackType == GPS_PACK_GPGSA) {
        uint8_t Tmp[16];
        uint8_t i = 0;
        uint8_t IsError = __FALSE;

        //判断定位
        for (pStrS = (char *)buff, i = 0; i < 2; i++) {
            if ((pStrS = strchr(pStrS + 1, ',')) == NULL) {
                IsError = __TRUE;
                break;
            }
        }
        if (IsError == __FALSE) {
            pStrS++;
            if (*(pStrS) == ',') {
                IsLocate = __FALSE;
                //NMEA_DEBUG("NMEA->未定位\n");
                return;
            }
            if ((pStrE = strchr(pStrS, ',')) == NULL) return;
            if (pStrE - pStrS > (sizeof(Tmp) - 1))  return;

            memset(Tmp, 0, sizeof(Tmp));
            memcpy(Tmp, pStrS, pStrE - pStrS);

            //NMEA_DEBUG("NMEA->定位标志:%s\n", Tmp);

            if (atoi((char *)Tmp) == 1) {
                IsLocate = __FALSE;
                //NMEA_DEBUG("NMEA->未定位\n");
                return;
            }
        }
        else {
            return;
        }
        //判断定位精度
        IsError = __FALSE;
        for (pStrS = (char *)buff, i = 0; i < 15; i++) {
            if ((pStrS = strchr(pStrS + 1, ',')) == NULL) {
                IsError = __TRUE;
                break;
            }
        }
        if (IsError == __FALSE) {
            pStrS++;
            if (*(pStrS) == ',') {
                IsLocate = __FALSE;
                //NMEA_DEBUG("NMEA->定位精度不够\n");
                return;
            }
            if ((pStrE = strchr(pStrS, ',')) == NULL) return;
            if (pStrE - pStrS > (sizeof(Tmp) - 1))  return;

            memset(Tmp, 0, sizeof(Tmp));
            memcpy(Tmp, pStrS, pStrE - pStrS);

            //NMEA_DEBUG("NMEA->定位精度:%s\n", Tmp);

            accu = (float)atof((char *)Tmp);

            if ( accu > 49.0f) {
                IsLocate = __FALSE;
                //NMEA_DEBUG("NMEA->定位精度不够\n");
            }
            else {
                IsLocate = __TRUE;
                //lip_update_local(NULL, &accu);
                //NMEA_DEBUG("NMEA->有效定位精度\n");
            }
        }
        else {
            ;
        }
    }
    if((CurPackType == GPS_PACK_GPGSA) && (IsLocate == __TRUE)){

        for(i = 0;buff[i] != '\n';i++)
            gga_buff[i] = buff[i];

        gga_buff[i] = 0;
        
        Get_GGA = __TRUE;
        fprintf(stdout,"GGA string is %s\n",gga_buff);
    }
}

static const char *encodeurl(const char *req)
{
  char *h = "0123456789abcdef";
  static char buf[128];
  char *urlenc = buf;
  char *bufend = buf + sizeof(buf) - 3;

  while(*req && urlenc < bufend)
  {
    if(isalnum(*req)
    || *req == '-' || *req == '_' || *req == '.')
      *urlenc++ = *req++;
    else
    {
      *urlenc++ = '%';
      *urlenc++ = h[*req >> 4];
      *urlenc++ = h[*req & 0x0f];
      req++;
    }
  }
  *urlenc = 0;
  return buf;
}

static const char *geturl(const char *url, struct Args *args)
{
  static char buf[1000];
  static char *Buffer = buf;
  static char *Bufend = buf+sizeof(buf);
  char *h = "0123456789abcdef";

  if(strncmp("ntrip:", url, 6))
    return "URL must start with 'ntrip:'.";
  url += 6; /* skip ntrip: */

  if(*url != '@' && *url != '/')
  {
    /* scan for mountpoint */
    args->data = Buffer;
    if(*url != '?')
    {
       while(*url && *url != '@' &&  *url != ';' && *url != '/' && Buffer != Bufend)
         *(Buffer++) = *(url++);
    }
    else
    {
       while(*url && *url != '@' &&  *url != '/' && Buffer != Bufend)
       {
          if(isalnum(*url) || *url == '-' || *url == '_' || *url == '.')
            *Buffer++ = *url++;
          else
          {
            *Buffer++ = '%';
            *Buffer++ = h[*url >> 4];
            *Buffer++ = h[*url & 0x0f];
            url++;
          }
       }
    }
    if(Buffer == args->data)
      return "Mountpoint required.";
    else if(Buffer >= Bufend-1)
      return "Parsing buffer too short.";
    *(Buffer++) = 0;
  }

  if(*url == '/') /* username and password */
  {
    ++url;
    args->user = Buffer;
    while(*url && *url != '@' && *url != ';' && *url != ':' && Buffer != Bufend)
      *(Buffer++) = *(url++);
    if(Buffer == args->user)
      return "Username cannot be empty.";
    else if(Buffer >= Bufend-1)
      return "Parsing buffer too short.";
    *(Buffer++) = 0;

    if(*url == ':') ++url;

    args->password = Buffer;
    while(*url && *url != '@' && *url != ';' && Buffer != Bufend)
      *(Buffer++) = *(url++);
    if(Buffer == args->password)
      return "Password cannot be empty.";
    else if(Buffer >= Bufend-1)
      return "Parsing buffer too short.";
    *(Buffer++) = 0;
  }

  if(*url == '@') /* server */
  {
    ++url;
    if(*url != '@' && *url != ':')
    {
      args->server = Buffer;
      while(*url && *url != '@' && *url != ':' && *url != ';' && Buffer != Bufend)
        *(Buffer++) = *(url++);
      if(Buffer == args->server)
        return "Servername cannot be empty.";
      else if(Buffer >= Bufend-1)
        return "Parsing buffer too short.";
      *(Buffer++) = 0;
    }

    if(*url == ':')
    {
      ++url;
      args->port = Buffer;
      while(*url && *url != '@' && *url != ';' && Buffer != Bufend)
        *(Buffer++) = *(url++);
      if(Buffer == args->port)
        return "Port cannot be empty.";
      else if(Buffer >= Bufend-1)
        return "Parsing buffer too short.";
      *(Buffer++) = 0;
    }

    if(*url == '@') /* proxy */
    {
      ++url;
      args->proxyhost = Buffer;
      while(*url && *url != ':' && *url != ';' && Buffer != Bufend)
        *(Buffer++) = *(url++);
      if(Buffer == args->proxyhost)
        return "Proxy servername cannot be empty.";
      else if(Buffer >= Bufend-1)
        return "Parsing buffer too short.";
      *(Buffer++) = 0;

      if(*url == ':')
      {
        ++url;
        args->proxyport = Buffer;
        while(*url && *url != ';' && Buffer != Bufend)
          *(Buffer++) = *(url++);
        if(Buffer == args->proxyport)
          return "Proxy port cannot be empty.";
        else if(Buffer >= Bufend-1)
          return "Parsing buffer too short.";
        *(Buffer++) = 0;
      }
    }
  }
  if(*url == ';') /* NMEA */
  {
    args->nmea = ++url;
    while(*url)
      ++url;
  }

  return *url ? "Garbage at end of server string." : 0;
}

static int getargs(struct Args *args)
{
  int res = 1;
  int getoptr;
  char *a;
  int i = 0;

  args->server = "ntrip.qxwz.com";
  args->port = "8001";
  args->user = "carsmart1";
  args->password = "6648ebb";
  args->nmea = "$GPGGA,023056.00,3957.34315,N,11621.53262,E,1,10,1.07,155.5,M,-8.6,M,,*43";//0;
  args->data = "RTCM23_GPS";
  args->bitrate = 0;
  args->proxyhost = 0;
  args->proxyport = "2101";
  args->mode = NTRIP1;
  args->initudp = 0;
  args->udpport = 0;
  args->protocol = SPAPROTOCOL_NONE;
  args->parity = SPAPARITY_NONE;
  args->stopbits = SPASTOPBITS_1;
  args->databits = SPADATABITS_8;
  args->baud = SPABAUD_9600;
  args->serdevice = SERIALDEFAULTDEVICE;
  args->serlogfile = 0;

  for(a = revisionstr+11; *a && *a != ' '; ++a)
    revisionstr[i++] = *a;
  revisionstr[i] = 0;
  datestr[0] = datestr[7];
  datestr[1] = datestr[8];
  datestr[2] = datestr[9];
  datestr[3] = datestr[10];
  datestr[5] = datestr[12];
  datestr[6] = datestr[13];
  datestr[8] = datestr[15];
  datestr[9] = datestr[16];
  datestr[4] = datestr[7] = '-';
  datestr[10] = 0;
 
  return res;
}

static const char encodingTable [64] = {
  'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
  'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
  'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
  'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
};

/* does not buffer overrun, but breaks directly after an error */
/* returns the number of required bytes */
static int encode(char *buf, int size, const char *user, const char *pwd)
{
  unsigned char inbuf[3];
  char *out = buf;
  int i, sep = 0, fill = 0, bytes = 0;

  while(*user || *pwd)
  {
    i = 0;
    while(i < 3 && *user) inbuf[i++] = *(user++);
    if(i < 3 && !sep)    {inbuf[i++] = ':'; ++sep; }
    while(i < 3 && *pwd)  inbuf[i++] = *(pwd++);
    while(i < 3)         {inbuf[i++] = 0; ++fill; }
    if(out-buf < size-1)
      *(out++) = encodingTable[(inbuf [0] & 0xFC) >> 2];
    if(out-buf < size-1)
      *(out++) = encodingTable[((inbuf [0] & 0x03) << 4)
               | ((inbuf [1] & 0xF0) >> 4)];
    if(out-buf < size-1)
    {
      if(fill == 2)
        *(out++) = '=';
      else
        *(out++) = encodingTable[((inbuf [1] & 0x0F) << 2)
                 | ((inbuf [2] & 0xC0) >> 6)];
    }
    if(out-buf < size-1)
    {
      if(fill >= 1)
        *(out++) = '=';
      else
        *(out++) = encodingTable[inbuf [2] & 0x3F];
    }
    bytes += 4;
  }
  if(out-buf < size)
    *out = 0;
  return bytes;
}

int ntrip_main_proc()
{
  struct Args args;

  setbuf(stdout, 0);
  setbuf(stdin, 0);
  setbuf(stderr, 0);
  signal(SIGALRM,sighandler_alarm);
  signal(SIGINT,sighandler_int);
  
  alarm(ALARMTIME);


  if(getargs(&args))
  {
    struct serial sx;
    FILE *ser = 0;
    char nmeabuffer[200] = "$GPGGA,"; /* our start string */
    size_t nmeabufpos = 0;
    size_t nmeastarpos = 0;
    int sleeptime = 0;
    
    do
    {
      int error = 0;
      sockettype sockfd = 0;
      int numbytes;
      char buf[MAXDATASIZE];
      struct sockaddr_in their_addr; /* connector's address information */
      struct hostent *he;
      struct servent *se;
      const char *server, *port, *proxyserver = 0;
      char proxyport[6];
      char *b;
      long i;
      if(sleeptime)
      {

        sleep(sleeptime);
        sleeptime += 2;
      }
      else
      {
        sleeptime = 1;
      }
      alarm(ALARMTIME);
      if(args.proxyhost)
      {
        int p;
        if((i = strtol(args.port, &b, 10)) && (!b || !*b))
          p = i;
        else if(!(se = getservbyname(args.port, 0)))
        {
          fprintf(stderr, "Can't resolve port %s.", args.port);
          stop = 1;
        }
        else
        {
          p = ntohs(se->s_port);
        }
        if(!stop && !error)
        {
          snprintf(proxyport, sizeof(proxyport), "%d", p);
          port = args.proxyport;
          proxyserver = args.server;
          server = args.proxyhost;
        }
      }
      else
      {
        server = args.server;
        port = args.port;
      }
      if(!stop && !error)
      {
        memset(&their_addr, 0, sizeof(struct sockaddr_in));
        if((i = strtol(port, &b, 10)) && (!b || !*b))
          their_addr.sin_port = htons(i);
        else if(!(se = getservbyname(port, 0)))
        {
          fprintf(stderr, "Can't resolve port %s.", port);
          stop = 1;
        }
        else
        {
          their_addr.sin_port = se->s_port;
        }
        if(!stop && !error)
        {
          if(!(he=gethostbyname(server)))
          {
            fprintf(stderr, "Server name lookup failed for '%s'.\n", server);
            error = 1;
          }
          else if((sockfd = socket(AF_INET, (args.mode == UDP ? SOCK_DGRAM :
          SOCK_STREAM), 0)) == -1)
          {
            myperror("socket");
            error = 1;
          }
          else
          {
            their_addr.sin_family = AF_INET;
            their_addr.sin_addr = *((struct in_addr *)he->h_addr);
          }
        }
      }
      if(!stop && !error)
      {
        if(args.mode == UDP)
        {
          unsigned int session;
          int tim, seq, init;
          char rtpbuf[1526];
          int i=12, j;

          init = time(0);
          srand(init);
          session = rand();
          tim = rand();
          seq = rand();

          rtpbuf[0] = (2<<6);
          /* padding, extension, csrc are empty */
          rtpbuf[1] = 97;
          /* marker is empty */
          rtpbuf[2] = (seq>>8)&0xFF;
          rtpbuf[3] = (seq)&0xFF;
          rtpbuf[4] = (tim>>24)&0xFF;
          rtpbuf[5] = (tim>>16)&0xFF;
          rtpbuf[6] = (tim>>8)&0xFF;
          rtpbuf[7] = (tim)&0xFF;
          /* sequence and timestamp are empty */
          rtpbuf[8] = (session>>24)&0xFF;
          rtpbuf[9] = (session>>16)&0xFF;
          rtpbuf[10] = (session>>8)&0xFF;
          rtpbuf[11] = (session)&0xFF;
          ++seq;

          j = snprintf(rtpbuf+i, sizeof(rtpbuf)-i-40, /* leave some space for login */
          "GET /%s HTTP/1.1\r\n"
          "Host: %s\r\n"
          "Ntrip-Version: Ntrip/2.0\r\n"
          "User-Agent: %s/%s\r\n"
          "%s%s%s"
          "Connection: close%s",
          args.data ? args.data : "", args.server, AGENTSTRING, revisionstr,
          args.nmea ? "Ntrip-GGA: " : "", args.nmea ? args.nmea : "",
          args.nmea ? "\r\n" : "",
          (*args.user || *args.password) ? "\r\nAuthorization: Basic " : "");
          i += j;
          if(i > (int)sizeof(rtpbuf)-40 || j < 0) /* second check for old glibc */
          {
            fprintf(stderr, "Requested data too long\n");
            stop = 1;
          }
          else
          {
            i += encode(rtpbuf+i, sizeof(rtpbuf)-i-4, args.user, args.password);
            if(i > (int)sizeof(rtpbuf)-4)
            {
              fprintf(stderr, "Username and/or password too long\n");
              stop = 1;
            }
            else
            {
              struct sockaddr_in local;
              socklen_t len;

              rtpbuf[i++] = '\r';
              rtpbuf[i++] = '\n';
              rtpbuf[i++] = '\r';
              rtpbuf[i++] = '\n';


              /* fill structure with local address information for UDP */
              memset(&local, 0, sizeof(local));
              local.sin_family = AF_INET;
              local.sin_port = htons(args.udpport);
              local.sin_addr.s_addr = htonl(INADDR_ANY);
              len = sizeof(local);

              /* bind() in order to get a random RTP client_port */
              if((bind(sockfd, (struct sockaddr *)&local, len)) < 0)
              {
                myperror("bind");
                error = 1;
              }
              else if(connect(sockfd, (struct sockaddr *)&their_addr,
              sizeof(struct sockaddr)) == -1)
              {
                myperror("connect");
                error = 1;
              }
              else if(send(sockfd, rtpbuf, i, 0) != i)
              {
                myperror("Could not send UDP packet");
                stop = 1;
              }
              else
              {
                if((numbytes=recv(sockfd, rtpbuf, sizeof(rtpbuf)-1, 0)) > 0)
                {
                  int sn = 0x10000, ts=0;
                  /* we don't expect message longer than 1513, so we cut the last
                    byte for security reasons to prevent buffer overrun */
                  rtpbuf[numbytes] = 0;
                  if(numbytes > 17+12 &&
                  (!strncmp(rtpbuf+12, "HTTP/1.1 200 OK\r\n", 17) ||
                  !strncmp(rtpbuf+12, "HTTP/1.0 200 OK\r\n", 17)))
                  {
                    const char *sessioncheck = "session: ";
                    const char *datacheck = "Content-Type: gnss/data\r\n";
                    const char *sourcetablecheck = "Content-Type: gnss/sourcetable\r\n";
                    const char *contentlengthcheck = "Content-Length: ";
                    const char *httpresponseend = "\r\n\r\n";
                    int contentlength = 0, httpresponselength = 0;
                    /* datacheck */
                    int l = strlen(datacheck)-1;
                    int j=0;
                    for(i = 12; j != l && i < numbytes-l; ++i)
                    {
                      for(j = 0; j < l && rtpbuf[i+j] == datacheck[j]; ++j)
                        ;
                    }
                    if(i != numbytes-l)
                    {
                      /* check for Session */
                      l = strlen(sessioncheck)-1;
                      j=0;
                      for(i = 12; j != l && i < numbytes-l; ++i)
                      {
                        for(j = 0; j < l && tolower(rtpbuf[i+j]) == sessioncheck[j]; ++j)
                          ;
                      }
                      if(i != numbytes-l) /* found a session number */
                      {
                        i+=l;
                        session = 0;
                        while(i < numbytes && rtpbuf[i] >= '0' && rtpbuf[i] <= '9')
                          session = session * 10 + rtpbuf[i++]-'0';
                        if(rtpbuf[i] != '\r')
                        {
                          fprintf(stderr, "Could not extract session number\n");
                          stop = 1;
                        }
                      }
                    }
                    else
                    {
                      /* sourcetablecheck */
                      l = strlen(sourcetablecheck)-1;
                      j=0;
                      for(i = 12; j != l && i < numbytes-l; ++i)
                      {
                        for(j = 0; j < l && rtpbuf[i+j] == sourcetablecheck[j]; ++j)
                          ;
                      }
                      if(i == numbytes-l) 
                      {
                        fprintf(stderr, "No 'Content-Type: gnss/data' or"
				" 'Content-Type: gnss/sourcetable' found\n");
                        error = 1;
                      }
                      else 
                      {
                        /* check for http response end */
                        l = strlen(httpresponseend)-1;
                        j=0;
                        for(i = 12; j != l && i < numbytes-l; ++i)
                        {
                          for(j = 0; j < l && rtpbuf[i+j] == httpresponseend[j]; ++j)
                            ;
                        }
                        if(i != numbytes-l) /* found http response end */
                        {
                            httpresponselength = i+3-12;
                        }
                        /* check for content length */
                        l = strlen(contentlengthcheck)-1;
                        j=0;
                        for(i = 12; j != l && i < numbytes-l; ++i)
                        {
                          for(j = 0; j < l && rtpbuf[i+j] == contentlengthcheck[j]; ++j)
                            ;
                        }
                        if(i != numbytes-l) /* found content length */
                        {
                          i+=l;
                          contentlength = 0;
                          while(i < numbytes && rtpbuf[i] >= '0' && rtpbuf[i] <= '9')
                            contentlength = contentlength * 10 + rtpbuf[i++]-'0';
                          if(rtpbuf[i] == '\r')
                          {
		            contentlength += httpresponselength;                     
                            do
                            {
                              fwrite(rtpbuf+12, (size_t)numbytes-12, 1, stdout);
                              if((contentlength -= (numbytes-12)) == 0)
                              {
                                stop = 1;
                              }
                              else
                              {
                                numbytes = recv(sockfd, rtpbuf, sizeof(rtpbuf), 0);
                              }
                            }while((numbytes >12) && (!stop));
                          }
                          else
                          { 
                            fprintf(stderr, "Could not extract content length\n");
                            stop = 1;
                          }
                        }
                      }
                    }
                  }
                  else
                  {
                    int k;
                    fprintf(stderr, "Could not get the requested data: ");
                    for(k = 12; k < numbytes && rtpbuf[k] != '\n' && rtpbuf[k] != '\r'; ++k)
                    {
                      fprintf(stderr, "%c", isprint(rtpbuf[k]) ? rtpbuf[k] : '.');
                    }
                    fprintf(stderr, "\n");
                    error = 1;
                  }
                  while(!stop && !error)
                  {
                    struct timeval tv = {1,0};
                    fd_set fdr;
                    fd_set fde;

                    FD_ZERO(&fdr);
                    FD_ZERO(&fde);
                    FD_SET(sockfd, &fdr);
                    FD_SET(sockfd, &fde);
                    if(select(sockfd+1,&fdr,0,&fde,&tv) < 0)
                    {
                      fprintf(stderr, "Select problem.\n");
                      error = 1;
                      continue;
                    }
                    i = recv(sockfd, rtpbuf, sizeof(rtpbuf), 0);
                    
                    alarm(ALARMTIME);
                    
                    if(i >= 12 && (unsigned char)rtpbuf[0] == (2 << 6)
                    && rtpbuf[1] >= 96 && rtpbuf[1] <= 98)
                    {
                      time_t ct;
                      int u,v;
                      unsigned int w;
                      u = ((unsigned char)rtpbuf[2]<<8)+(unsigned char)rtpbuf[3];
                      v = ((unsigned char)rtpbuf[4]<<24)+((unsigned char)rtpbuf[5]<<16)
                      +((unsigned char)rtpbuf[6]<<8)+(unsigned char)rtpbuf[7];
                      w = ((unsigned char)rtpbuf[8]<<24)+((unsigned char)rtpbuf[9]<<16)
                      +((unsigned char)rtpbuf[10]<<8)+(unsigned char)rtpbuf[11];

                      if(sn == 0x10000) {sn = u-1;ts=v-1;}
                      else if(u < -30000 && sn > 30000) sn -= 0xFFFF;
                      if(session != w || ts > v)
                      {
                        fprintf(stderr, "Illegal UDP data received.\n");
                        continue;
                      }
                      else if(u > sn) /* don't show out-of-order packets */
                      {
                        if(rtpbuf[1] == 98)
                        {
                          fprintf(stderr, "Connection closed.\n");
                          error = 1;
                          continue;
                        }
                        else if((rtpbuf[1] == 96)  && (i>12)) 
                        {
                          fwrite(rtpbuf+12, (size_t)i-12, 1, stdout);
                        }
                      }
                      sn = u; ts = v;
  
                      /* Keep Alive */
                      ct = time(0);
                      if(ct-init > 15)
                      {
                        tim += (ct-init)*1000000/TIME_RESOLUTION;
                        rtpbuf[0] = (2<<6);
                        /* padding, extension, csrc are empty */
                        rtpbuf[1] = 96;
                        /* marker is empty */
                        rtpbuf[2] = (seq>>8)&0xFF;
                        rtpbuf[3] = (seq)&0xFF;
                        rtpbuf[4] = (tim>>24)&0xFF;
                        rtpbuf[5] = (tim>>16)&0xFF;
                        rtpbuf[6] = (tim>>8)&0xFF;
                        rtpbuf[7] = (tim)&0xFF;
                        /* sequence and timestamp are empty */
                        rtpbuf[8] = (session>>24)&0xFF;
                        rtpbuf[9] = (session>>16)&0xFF;
                        rtpbuf[10] = (session>>8)&0xFF;
                        rtpbuf[11] = (session)&0xFF;
                        ++seq;
                        init = ct;

                        if(send(sockfd, rtpbuf, 12, 0) != 12)
                        {
                          myperror("send");
                          error = 1;
                        }
                      }
                    }
                    else if(i >= 0)
                    {
                      fprintf(stderr, "Illegal UDP header.\n");
                      continue;
                    }
                  }
                }
                /* send connection close always to allow nice session closing */
                tim += (time(0)-init)*1000000/TIME_RESOLUTION;
                rtpbuf[0] = (2<<6);
                /* padding, extension, csrc are empty */
                rtpbuf[1] = 98;
                /* marker is empty */
                rtpbuf[2] = (seq>>8)&0xFF;
                rtpbuf[3] = (seq)&0xFF;
                rtpbuf[4] = (tim>>24)&0xFF;
                rtpbuf[5] = (tim>>16)&0xFF;
                rtpbuf[6] = (tim>>8)&0xFF;
                rtpbuf[7] = (tim)&0xFF;
                /* sequence and timestamp are empty */
                rtpbuf[8] = (session>>24)&0xFF;
                rtpbuf[9] = (session>>16)&0xFF;
                rtpbuf[10] = (session>>8)&0xFF;
                rtpbuf[11] = (session)&0xFF;

                send(sockfd, rtpbuf, 12, 0); /* cleanup */
              }
            }
          }
        }
        else if(args.data && *args.data != '%' && args.mode == RTSP)
        {
          struct sockaddr_in local;
          sockettype sockudp = 0;
          int localport;
          int cseq = 1;
          socklen_t len;

          if((sockudp = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
          {
            myperror("socket");
            error = 1;
          }
          if(!stop && !error)
          {
            /* fill structure with local address information for UDP */
            memset(&local, 0, sizeof(local));
            local.sin_family = AF_INET;
            local.sin_port = htons(args.udpport);
            local.sin_addr.s_addr = htonl(INADDR_ANY);
            len = sizeof(local);
            /* bind() in order to get a random RTP client_port */
            if((bind(sockudp, (struct sockaddr *)&local, len)) < 0)
            {
              myperror("bind");
              error = 1;
            }
            else if((getsockname(sockudp, (struct sockaddr*)&local, &len)) == -1)
            {
              myperror("local access failed");
              error = 1;
            }
            else if(connect(sockfd, (struct sockaddr *)&their_addr,
            sizeof(struct sockaddr)) == -1)
            {
              myperror("connect");
              error = 1;
            }
            localport = ntohs(local.sin_port);
          }
          if(!stop && !error)
          {
            i=snprintf(buf, MAXDATASIZE-40, /* leave some space for login */
            "SETUP rtsp://%s%s%s/%s RTSP/1.0\r\n"
            "CSeq: %d\r\n"
            "Ntrip-Version: Ntrip/2.0\r\n"
            "Ntrip-Component: Ntripclient\r\n"
            "User-Agent: %s/%s\r\n"
            "%s%s%s"
            "Transport: RTP/GNSS;unicast;client_port=%u%s",
            args.server, proxyserver ? ":" : "", proxyserver ? args.port : "",
            args.data, cseq++, AGENTSTRING, revisionstr,
            args.nmea ? "Ntrip-GGA: " : "", args.nmea ? args.nmea : "",
            args.nmea ? "\r\n" : "",
            localport,
            (*args.user || *args.password) ? "\r\nAuthorization: Basic " : "");
            if(i > MAXDATASIZE-40 || i < 0) /* second check for old glibc */
            {
              fprintf(stderr, "Requested data too long\n");
              stop = 1;
            }
            i += encode(buf+i, MAXDATASIZE-i-4, args.user, args.password);
            if(i > MAXDATASIZE-4)
            {
              fprintf(stderr, "Username and/or password too long\n");
              stop = 1;
            }
            buf[i++] = '\r';
            buf[i++] = '\n';
            buf[i++] = '\r';
            buf[i++] = '\n';
          }
          if(!stop && !error)
          {
            if(send(sockfd, buf, (size_t)i, 0) != i)
            {
              myperror("send");
              error = 1;
            }
            else if((numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1)
            {
              myperror("recv");
              error = 1;
            }
            else if(numbytes >= 17 && !strncmp(buf, "RTSP/1.0 200 OK\r\n", 17))
            {
              int serverport = 0, session = 0;
              const char *portcheck = "server_port=";
              const char *sessioncheck = "session: ";
              int l = strlen(portcheck)-1;
              int j=0;
              for(i = 0; j != l && i < numbytes-l; ++i)
              {
                for(j = 0; j < l && tolower(buf[i+j]) == portcheck[j]; ++j)
                  ;
              }
              if(i == numbytes-l)
              {
                fprintf(stderr, "No server port number found\n");
                stop = 1;
              }
              else
              {
                i+=l;
                while(i < numbytes && buf[i] >= '0' && buf[i] <= '9')
                  serverport = serverport * 10 + buf[i++]-'0';
                if(buf[i] != '\r' && buf[i] != ';')
                {
                  fprintf(stderr, "Could not extract server port\n");
                  stop = 1;
                }
              }
              if(!stop && !error)
              {
                l = strlen(sessioncheck)-1;
                j=0;
                for(i = 0; j != l && i < numbytes-l; ++i)
                {
                  for(j = 0; j < l && tolower(buf[i+j]) == sessioncheck[j]; ++j)
                    ;
                }
                if(i == numbytes-l)
                {
                  fprintf(stderr, "No session number found\n");
                  stop = 1;
                }
                else
                {
                  i+=l;
                  while(i < numbytes && buf[i] >= '0' && buf[i] <= '9')
                    session = session * 10 + buf[i++]-'0';
                  if(buf[i] != '\r')
                  {
                    fprintf(stderr, "Could not extract session number\n");
                    stop = 1;
                  }
                }
              }
              if(!stop && !error && args.initudp)
              {
                printf("Sending initial UDP packet\n");
                struct sockaddr_in casterRTP;
                char rtpbuffer[12];
                int i;
                rtpbuffer[0] = (2<<6);
                /* padding, extension, csrc are empty */
                rtpbuffer[1] = 96;
                /* marker is empty */
                rtpbuffer[2] = 0;
                rtpbuffer[3] = 0;
                rtpbuffer[4] = 0;
                rtpbuffer[5] = 0;
                rtpbuffer[6] = 0;
                rtpbuffer[7] = 0;
                /* sequence and timestamp are empty */
                rtpbuffer[8] = (session>>24)&0xFF;
                rtpbuffer[9] = (session>>16)&0xFF;
                rtpbuffer[10] = (session>>8)&0xFF;
                rtpbuffer[11] = (session)&0xFF;
                /* fill structure with caster address information for UDP */
                memset(&casterRTP, 0, sizeof(casterRTP));
                casterRTP.sin_family = AF_INET;
                casterRTP.sin_port   = htons(serverport);
                casterRTP.sin_addr   = *((struct in_addr *)he->h_addr);

                if((i = sendto(sockudp, rtpbuffer, 12, 0,
                (struct sockaddr *) &casterRTP, sizeof(casterRTP))) != 12)
                  myperror("WARNING: could not send initial UDP packet");
              }
              if(!stop && !error)
              {
                i = snprintf(buf, MAXDATASIZE,
                "PLAY rtsp://%s%s%s/%s RTSP/1.0\r\n"
                "CSeq: %d\r\n"
                "Session: %u\r\n"
                "\r\n",
                args.server, proxyserver ? ":" : "", proxyserver ? args.port : "",
                args.data, cseq++, session);

                if(i > MAXDATASIZE || i < 0) /* second check for old glibc */
                {
                  fprintf(stderr, "Requested data too long\n");
                  stop=1;
                }
                else if(send(sockfd, buf, (size_t)i, 0) != i)
                {
                  myperror("send");
                  error = 1;
                }
                else if((numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) != -1)
                {
                  if(numbytes >= 17 && !strncmp(buf, "RTSP/1.0 200 OK\r\n", 17))
                  {
                    int ts = 0, sn = 0;
                    time_t init = 0;
                    struct sockaddr_in addrRTP;

                    if(fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0
                    || fcntl(sockudp, F_SETFL, O_NONBLOCK) < 0)
                    {
                      fprintf(stderr, "Could not set nonblocking mode\n");
                      error = 1;
                    }

                    /* fill structure with caster address information for UDP */
                    memset(&addrRTP, 0, sizeof(addrRTP));
                    addrRTP.sin_family = AF_INET;
                    addrRTP.sin_port   = htons(serverport);
                    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
                    len = sizeof(addrRTP);

                    while(!stop && !error)
                    {
                      char rtpbuffer[1526];
                      struct timeval tv = {1,0};
                      fd_set fdr;
                      fd_set fde;
                      int r;

                      FD_ZERO(&fdr);
                      FD_ZERO(&fde);
                      FD_SET(sockudp, &fdr);
                      FD_SET(sockfd, &fdr);
                      FD_SET(sockudp, &fde);
                      FD_SET(sockfd, &fde);
                      if(select((sockudp>sockfd?sockudp:sockfd)+1,
                      &fdr,0,&fde,&tv) < 0)
                      {
                        fprintf(stderr, "Select problem.\n");
                        error = 1;
                        continue;
                      }
                      i = recvfrom(sockudp, rtpbuffer, sizeof(rtpbuffer), 0,
                      (struct sockaddr*) &addrRTP, &len);
                      
                      alarm(ALARMTIME);
                      
                      if(i >= 12+1 && (unsigned char)rtpbuffer[0] == (2 << 6) && rtpbuffer[1] == 0x60)
                      {
                        int u,v,w;
                        u = ((unsigned char)rtpbuffer[2]<<8)+(unsigned char)rtpbuffer[3];
                        v = ((unsigned char)rtpbuffer[4]<<24)+((unsigned char)rtpbuffer[5]<<16)
                        +((unsigned char)rtpbuffer[6]<<8)+(unsigned char)rtpbuffer[7];
                        w = ((unsigned char)rtpbuffer[8]<<24)+((unsigned char)rtpbuffer[9]<<16)
                        +((unsigned char)rtpbuffer[10]<<8)+(unsigned char)rtpbuffer[11];

                        if(init)
                        {
                          time_t ct;
                          if(u < -30000 && sn > 30000) sn -= 0xFFFF;
                          if(session != w || ts > v)
                          {
                            fprintf(stderr, "Illegal UDP data received.\n");
                            continue;
                          }
                          else if(u > sn) /* don't show out-of-order packets */
                            fwrite(rtpbuffer+12, (size_t)i-12, 1, stdout);
                          ct = time(0);
                          if(ct-init > 15)
                          {
                            i = snprintf(buf, MAXDATASIZE,
                            "GET_PARAMETER rtsp://%s%s%s/%s RTSP/1.0\r\n"
                            "CSeq: %d\r\n"
                            "Session: %u\r\n"
                            "\r\n",
                            args.server, proxyserver ? ":" : "", proxyserver
                            ? args.port : "", args.data, cseq++, session);
                            if(i > MAXDATASIZE || i < 0)
                            {
                              fprintf(stderr, "Requested data too long\n");
                              stop = 1;
                            }
                            else if(send(sockfd, buf, (size_t)i, 0) != i)
                            {
                              myperror("send");
                              error = 1;
                            }
                            init = ct;
                          }
                        }
                        else
                        {
                          init = time(0);
                        }
                        sn = u; ts = v;
                      }
                      else if(i >= 0)
                      {
                        fprintf(stderr, "Illegal UDP header.\n");
                        continue;
                      }
                      /* ignore RTSP server replies */
                      if((r=recv(sockfd, buf, MAXDATASIZE-1, 0)) < 0)
                      {

                        if(errno != EAGAIN)
                        {
                          fprintf(stderr, "Control connection closed\n");
                          error = 1;
                        }
                      }
                      else if(!r)
                      {
                        fprintf(stderr, "Control connection read error\n");
                        error = 1;
                      }
                    }
                  }
                  i = snprintf(buf, MAXDATASIZE,
                  "TEARDOWN rtsp://%s%s%s/%s RTSP/1.0\r\n"
                  "CSeq: %d\r\n"
                  "Session: %u\r\n"
                  "\r\n",
                  args.server, proxyserver ? ":" : "", proxyserver ? args.port : "",
                  args.data, cseq++, session);

                  if(i > MAXDATASIZE || i < 0) /* second check for old glibc */
                  {
                    fprintf(stderr, "Requested data too long\n");
                    stop = 1;
                  }
                  else if(send(sockfd, buf, (size_t)i, 0) != i)
                  {
                    myperror("send");
                    error = 1;
                  }
                }
                else
                {
                  fprintf(stderr, "Could not start data stream.\n");
                  error = 1;
                }
              }
            }
            else
            {
              fprintf(stderr, "Could not setup initial control connection.\n");
              error = 1;
            }
            if(sockudp)
              closesocket(sockudp);
          }
        }
        else//ntrip
        {
          //printf("%s,%u\n",__FILE__,__LINE__);
          if(connect(sockfd, (struct sockaddr *)&their_addr,
          sizeof(struct sockaddr)) == -1)
          {
            myperror("connect");
            error = 1;
          }
          if(!stop && !error)
          {
            //printf("%s,%u\n",__FILE__,__LINE__);
            if(!args.data)
            {
              i = snprintf(buf, MAXDATASIZE,
              "GET %s%s%s%s/ HTTP/1.1\r\n"
              "Host: %s\r\n%s"
              "User-Agent: %s/%s\r\n"
              "Connection: close\r\n"
              "\r\n"
              , proxyserver ? "http://" : "", proxyserver ? proxyserver : "",
              proxyserver ? ":" : "", proxyserver ? proxyport : "",
              args.server, args.mode == NTRIP1 ? "" : "Ntrip-Version: Ntrip/2.0\r\n",
              AGENTSTRING, revisionstr);
            }
            else
            {
              const char *nmeahead = (args.nmea && args.mode == HTTP) ? args.nmea : 0;

              i=snprintf(buf, MAXDATASIZE-40, /* leave some space for login */
              "GET %s%s%s%s/%s HTTP/1.1\r\n"
              "Host: %s\r\n%s"
              "User-Agent: %s/%s\r\n"
              "%s%s%s"
              "Connection: close%s"
              , proxyserver ? "http://" : "", proxyserver ? proxyserver : "",
              proxyserver ? ":" : "", proxyserver ? proxyport : "",
              args.data, args.server,
              args.mode == NTRIP1 ? "" : "Ntrip-Version: Ntrip/2.0\r\n",
              AGENTSTRING, revisionstr,
              nmeahead ? "Ntrip-GGA: " : "", nmeahead ? nmeahead : "",
              nmeahead ? "\r\n" : "",
              (*args.user || *args.password) ? "\r\nAuthorization: Basic " : "");
              if(i > MAXDATASIZE-40 || i < 0) /* second check for old glibc */
              {
                fprintf(stderr, "Requested data too long\n");
                stop = 1;
              }
              else
              {
                i += encode(buf+i, MAXDATASIZE-i-4, args.user, args.password);
                if(i > MAXDATASIZE-4)
                {
                  fprintf(stderr, "Username and/or password too long\n");
                  stop = 1;
                }
                else
                {
                  buf[i++] = '\r';
                  buf[i++] = '\n';
                  buf[i++] = '\r';
                  buf[i++] = '\n';
                  if(args.nmea && !nmeahead)
                  {
                    int j = snprintf(buf+i, MAXDATASIZE-i, "%s\r\n", args.nmea);
                    if(j >= 0 && j < MAXDATASIZE-i)
                      i += j;
                    else
                    {
                      fprintf(stderr, "NMEA string too long\n");
                      stop = 1;
                    }
                  }
                }
              }
            }
          }
          if(!stop && !error)
          {
          
          //printf("%s,%u\n",__FILE__,__LINE__);
          //printf("send buf is %s",buf);
            if(send(sockfd, buf, (size_t)i, 0) != i)
            {
              myperror("send");
              error = 1;
              
              //printf("%s,%u",__FILE__,__LINE__);
            }
            else if(args.data && *args.data != '%')
            {
            
            //printf("%s,%u\n",__FILE__,__LINE__);
              int k = 0;
              int chunkymode = 0;
              int starttime = time(0);
              int lastout = starttime;
              int totalbytes = 0;
              int chunksize = 0;

              while(!stop && !error &&
              (numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) > 0)
              {
                alarm(ALARMTIME);
                //printf("%s,%u\n",__FILE__,__LINE__);

                if(!k)
                {
                  //printf("%s,%u\n",__FILE__,__LINE__);
                 // printf("%s%d end\n",buf,numbytes);
                  if( numbytes > 17 &&
                    !strstr(buf, "ICY 200 OK")  &&  /* case 'proxy & ntrip 1.0 caster' */
                    (!strncmp(buf, "POP_GW_Ntrip_1.0_1457434850/1.0\r\n", 17) ||
                    !strncmp(buf, "HTTP/1.0 200 OK\r\n", 17)) )
                  {
                  printf("%s,%u\n",__FILE__,__LINE__);
                    const char *datacheck = "Content-Type: gnss/data\r\n";
                    const char *chunkycheck = "Transfer-Encoding: chunked\r\n";
                    int l = strlen(datacheck)-1;
                    int j=0;
                    for(i = 0; j != l && i < numbytes-l; ++i)
                    {
                      for(j = 0; j < l && buf[i+j] == datacheck[j]; ++j)
                        ;
                    }
                    if(i == numbytes-l)
                    {
                      fprintf(stderr, "No 'Content-Type: gnss/data' found\n");
                      error = 1;
                    }
                    l = strlen(chunkycheck)-1;
                    j=0;
                    for(i = 0; j != l && i < numbytes-l; ++i)
                    {
                      for(j = 0; j < l && buf[i+j] == chunkycheck[j]; ++j)
                        ;
                    }
                    if(i < numbytes-l)
                      chunkymode = 1;
                  }
                  else if(!strstr(buf, "ICY 200 OK"))
                  {
                  //printf("%s,%u\n",__FILE__,__LINE__);
                    fprintf(stderr, "Could not get the requested data: ");
                    for(k = 0; k < numbytes && buf[k] != '\n' && buf[k] != '\r'; ++k)
                    {
                      fprintf(stderr, "%c", isprint(buf[k]) ? buf[k] : '.');
                    }
                    fprintf(stderr, "\n");
                    error = 1;
                  }
                  else if(args.mode != NTRIP1)
                  {
                  //printf("%s,%u\n",__FILE__,__LINE__);
                    fprintf(stderr, "NTRIP version 2 HTTP connection failed%s.\n",
                    args.mode == AUTO ? ", falling back to NTRIP1" : "");
                    if(args.mode == HTTP)
                      stop = 1;
                  }
                  ++k;
                }
                else
                {
                //printf("%s,%u\n",__FILE__,__LINE__);
                  sleeptime = 0;
                  if(chunkymode)
                  {
                    int cstop = 0;
                    int pos = 0;
                    while(!stop && !cstop && !error && pos < numbytes)
                    {
                      switch(chunkymode)
                      {
                      case 1: /* reading number starts */
                        chunksize = 0;
                        ++chunkymode; /* no break */
                      case 2: /* during reading number */
                        i = buf[pos++];
                        if(i >= '0' && i <= '9') chunksize = chunksize*16+i-'0';
                        else if(i >= 'a' && i <= 'f') chunksize = chunksize*16+i-'a'+10;
                        else if(i >= 'A' && i <= 'F') chunksize = chunksize*16+i-'A'+10;
                        else if(i == '\r') ++chunkymode;
                        else if(i == ';') chunkymode = 5;
                        else cstop = 1;
                        break;
                      case 3: /* scanning for return */
                        if(buf[pos++] == '\n') chunkymode = chunksize ? 4 : 1;
                        else cstop = 1;
                        break;
                      case 4: /* output data */
                        i = numbytes-pos;
                        if(i > chunksize) i = chunksize;
                        if(args.serdevice)
                        {
                          int ofs = 0;
                          while(i > ofs && !cstop && !stop && !error)
                          {
                            int j = SerialWrite(&sx, buf+pos+ofs, i-ofs);
                            //printf("char is %c",buf[pos+ofs]);  //debug
                            printf("%s,%u\n",__FILE__,__LINE__);
                            if(j < 0)
                            {
                              fprintf(stderr, "Could not access serial device\n");
                              stop = 1;
                            }
                            else
                              ofs += j;
                          }
                        }
                        else
                          fwrite(buf+pos, (size_t)i, 1, stdout);
                        totalbytes += i;
                        chunksize -= i;
                        pos += i;
                        if(!chunksize)
                          chunkymode = 1;
                        break;
                      case 5:
                        if(i == '\r') chunkymode = 3;
                        break;
                      }
                    }
                    if(cstop)
                    {
                      fprintf(stderr, "Error in chunky transfer encoding\n");
                      error = 1;
                    }
                  }
                  else
                  {
                    totalbytes += numbytes;
                    if(args.serdevice)
                    {
                      int ofs = 0;
                      while(numbytes > ofs && !stop)
                      {
                        //printf("\n\nbuf is %s\n\n",buf);
                        //printf("%s,%u\n",__FILE__,__LINE__);
                        int i = numbytes-ofs;//SerialWrite(&sx, buf+ofs, numbytes-ofs);
                        if(i < 0)
                        {
                          fprintf(stderr, "Could not access serial device\n");
                          stop = 1;
                        }
                        else
                          ofs += i;
                      }
                    }
                    else
                      fwrite(buf, (size_t)numbytes, 1, stdout);
                  }
                  fflush(stdout);
                  if(totalbytes < 0) /* overflow */
                  {
                    totalbytes = 0;
                    starttime = time(0);
                    lastout = starttime;
                  }
                  if(args.serdevice && !stop)
                  {
                    int doloop = 1;
                    while(doloop && !stop)
                    {
                      int i = SerialRead(&sx, buf, 200);
                      if(i < 0)
                      {
                        fprintf(stderr, "Could not access serial device\n");
                        stop = 1;
                      }
                      else
                      {
                        int j = 0;
                        if(i < 200) doloop = 0;
                        fwrite(buf, i, 1, stdout);
                        if(ser)
                          fwrite(buf, i, 1, ser);
                        while(j < i)
                        {
                          if(nmeabufpos < 6)
                          {
                            if(nmeabuffer[nmeabufpos] != buf[j])
                            {
                              if(nmeabufpos) nmeabufpos = 0;
                              else ++j;
                            }
                            else
                            {
                              nmeastarpos = 0;
                              ++j; ++nmeabufpos;
                            }
                          }
                          else if((nmeastarpos && nmeabufpos == nmeastarpos + 3)
                          || buf[j] == '\r' || buf[j] == '\n')
                          {
                            doloop = 0;
                            nmeabuffer[nmeabufpos++] = '\r';
                            nmeabuffer[nmeabufpos++] = '\n';
                            if(send(sockfd, nmeabuffer, nmeabufpos, 0)
                            != (int)nmeabufpos)
                            {
                              fprintf(stderr, "Could not send NMEA\n");
                              error = 1;
                            }
                            nmeabufpos = 0;
                          }
                          else if(nmeabufpos > sizeof(nmeabuffer)-10 ||
                          buf[j] == '$')
                            nmeabufpos = 0;
                          else
                          {
                            if(buf[j] == '*') nmeastarpos = nmeabufpos;
                            nmeabuffer[nmeabufpos++] = buf[j++];
                          }
                        }
                      }
                    }
                  }
                  if(args.bitrate)
                  {
                    int t = time(0);
                    if(t > lastout + 60)
                    {
                      lastout = t;
                      fprintf(stderr, "Bitrate is %dbyte/s (%d seconds accumulated).\n",
                      totalbytes/(t-starttime), t-starttime);
                    }
                  }
                }
                //printf("%s,%u\n",__FILE__,__LINE__);
              }
            }
            else
            {
            
            //printf("%s,%u\n",__FILE__,__LINE__);
              sleeptime = 0;
              while(!stop && (numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) > 0)
              {
                alarm(ALARMTIME);
                fwrite(buf, (size_t)numbytes, 1, stdout);
              }
            }
          }
        }
      }
      if(sockfd)
        closesocket(sockfd);
    } while(args.data && *args.data != '%' && !stop);
    if(args.serdevice)
    {
      SerialFree(&sx);
    }
    if(ser)
      fclose(ser);
  }
  return 0;
}

void * ntrip_thread_entry (void *parameter)
{
    int err;
    sys_msg_t *p_msg;
    vam_envar_t *p_vam = (vam_envar_t *)parameter;
    uint32_t len = 0;
    uint8_t buf[VAM_MQ_MSG_SIZE];
    
    //OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_INFO, "%s: ---->\n", __FUNCTION__);

    p_msg = (sys_msg_t *)buf;
    
	while(1){
        memset(buf, 0, VAM_MQ_MSG_SIZE);
        err = osal_queue_recv(p_vam->queue_vam, buf, &len, OSAL_WAITING_FOREVER);
        if (err == OSAL_STATUS_SUCCESS && len > 0){
            ntrip_main_proc(p_vam, p_msg);
        }
        else{
            OSAL_MODULE_DBGPRT(MODULE_NAME, OSAL_DEBUG_ERROR, "%s: osal_queue_recv error [%d]\n",\
                            __FUNCTION__, err);
        }
	}
}
void ntrip_init(void)
{
    osal_task_t *tid;


	tid = osal_task_create("tk_ntrip",
                           ntrip_thread_entry, NULL,
                           RT_NTRIP_THREAD_STACK_SIZE, RT_NTRIP_THREAD_PRIORITY);
    osal_assert(tid != NULL)
}

