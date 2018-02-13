#include "xSock.h"

#ifndef FOR_RING0

#include "xrand.h"

using std::string;

#ifdef _WIN32

class xWSA_base
  {
  private:
    xWSA_base();              //!< 故意做成私有初始，保证不被外部误初始
    ~xWSA_base();
    friend static void xWSA_init();
  };

xWSA_base::xWSA_base()
  {
  WSADATA wsaData;
  if(WSAStartup(MAKEWORD(2, 2), &wsaData))
    {
    xSockExpt("WSAStartup");
    }
  }

xWSA_base::~xWSA_base()
  {
  WSACleanup();
  }

static void xWSA_init()
  {
  static xWSA_base xwb;
  }

xWSA::xWSA()
  {
  xWSA_init();
  }

sockaddr_in AddrInfo(const char* host, const char* ports)
  {
  ADDRINFOA* res;

  ADDRINFOA hints;
  memset(&hints, 0, sizeof(hints));

  xWSA wsa;

  if(GetAddrInfoA(host, ports, &hints, &res))
    {
    xSockExpt("GetAddrInfo");
    }

  size_t count = 0;

  for(auto p = res; p != nullptr; p = p->ai_next)
    {
    ++count;
    }

  count = xrand(count) + 1;

  auto p = res;
  while(--count)
    {
    p = p->ai_next;
    }

  sockaddr_in addrto;
  memcpy(&addrto, p->ai_addr, sizeof(addrto));

  FreeAddrInfoA(res);

  return addrto;
  }

sockaddr_in AddrInfo(PCWSTR host, PCWSTR ports)
  {
  ADDRINFOW* res;

  ADDRINFOW hints;
  memset(&hints, 0, sizeof(hints));

  xWSA wsa;

  if(GetAddrInfoW(host, ports, &hints, &res))
    {
    xSockExpt("GetAddrInfo");
    }

  size_t count = 0;

  for(auto p = res; p != nullptr; p = p->ai_next)
    {
    ++count;
    }

  count = xrand(count) + 1;

  auto p = res;
  while(--count)
    {
    p = p->ai_next;
    }

  sockaddr_in addrto;
  memcpy(&addrto, p->ai_addr, sizeof(addrto));

  FreeAddrInfoW(res);

  return addrto;
  }
#else   // _WIN32

#include <netdb.h>
#include <arpa/inet.h>

sockaddr_in AddrInfo(const char* host, const char* ports)
  {
  addrinfo* res;

  addrinfo hints;
  memset(&hints, 0, sizeof(hints));

  if(getaddrinfo(host, ports, &hints, &res))
    {
    xSockExpt("GetAddrInfo");
    }

  size_t count = 0;

  for(auto p = res; p != nullptr; p = p->ai_next)
    {
    ++count;
    }

  count = xrand(count) + 1;

  auto p = res;
  while(--count)
    {
    p = p->ai_next;
    }

  sockaddr_in addrto;
  memcpy(&addrto, p->ai_addr, sizeof(addrto));

  freeaddrinfo(res);

  return addrto;
  }

#endif  // _WIN32

sockaddr_in AddrInfo(const uint32 host, const uint16 ports)
  {
  sockaddr_in addrto;
  memset(&addrto, 0, sizeof(addrto));

  addrto.sin_family = AF_INET;
  addrto.sin_addr.s_addr = htonl(host);
  addrto.sin_port = htons(ports);

  return addrto;
  }


string IpString(const sockaddr_in& addr)
  {
#if defined(_WIN32) && !defined(_WIN64)
  // XP的ws2_32没有inet_ntop导出
  return  xmsg()
    << (int)addr.sin_addr.s_net   << '.'
    << (int)addr.sin_addr.s_host << '.'
    << (int)addr.sin_addr.s_lh << '.'
    << (int)addr.sin_addr.s_impno << ':'
    << (int)htons(addr.sin_port);
#else
  char ips[0x40];
  if(nullptr == inet_ntop(AF_INET, (void*)&(addr.sin_addr), ips, sizeof(ips)))
    {
    xSockExpt("inet_ntop");
    }
  return xmsg() << ips << ':' << (int)htons(addr.sin_port);
#endif
  }


xUDP& operator<<(xUDP& s, xUDP_func pfn)
  {
  return pfn(s);
  }

xTCP& operator<<(xTCP& s, xTCP_func pfn)
  {
  return pfn(s);
  }

xUDP& xsend(xUDP& s)
  {
  return s.send();
  }

xTCP& xsend(xTCP& s)
  {
  return s.send();
  }

xUDP& xxsend(xUDP& s)
  {
  return s.sendmkhead();
  }

xTCP& xxsend(xTCP& s)
  {
  return s.sendmkhead();
  }

#endif  // FOR_RING0

#ifdef _XLIB_TEST_

ADD_XLIB_TEST(XSOCK)
  {
  SHOW_TEST_INIT;

  auto done = false;

  try
    {
    SHOW_TEST_HEAD("AddrInfo");
    auto addr = AddrInfo("127.0.0.1", "4210");

    char ips[0x40];
    inet_ntop(AF_INET, (void*)&(addr.sin_addr), ips, sizeof(ips));

    done = (0 == memcmp(ips, "127.0.0.1", 10));
    SHOW_TEST_RESULT(done);

    SHOW_TEST_HEAD("IpString");
    done = (string("127.0.0.1:4210") == IpString(addr));
    SHOW_TEST_RESULT(done);

    SHOW_TEST_HEAD("xSocket");
      {
      xSocket<SOCK_STREAM>();
      xSocket<SOCK_DGRAM>();
      }
    SHOW_TEST_RESULT(true);

    SHOW_TEST_HEAD("xUDP");
    xUDP udps(AddrInfo(0, 4210));
    xUDP udpc(AddrInfo("127.0.0.1", "4210"));

    udpc << "4210" << xsend;
    udps.recv();
    done = (0 == memcmp("4210", udps.recvbuf.c_str(), udps.recvbuf.size()));
    SHOW_TEST_RESULT(done);

    SHOW_TEST_HEAD("xTCP");
    xTCP tcps(AddrInfo(0, 4210));
    xTCP tcpc(AddrInfo("127.0.0.1", "4210"));

    auto tcpcc = tcps.apt();
    tcpc << "4210" << xsend;
    tcpcc->recv();
    done = (0 == memcmp("4210", tcpcc->recvbuf.c_str(), tcpcc->recvbuf.size()));
    delete tcpcc;
    SHOW_TEST_RESULT(done);
    }
  catch(const std::runtime_error& e)
    {
    cout << "XSOCK Exception : " << e.what() << endl;
    }
  catch(...)
    {
    cout << "XSOCK Exception!!!"<< endl;
    }
  }

#endif  // _XLIB_TEST_