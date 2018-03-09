/*!
  \file  xSock.h
  \brief xSock.h定义了网络通讯的基本封装

  \version    4.0.1701.0317
  \note       Only For Ring3

  \author     triones
  \date       2011-01-27
*/
#ifndef _XLIB_XSOCK_H_
#define _XLIB_XSOCK_H_

#ifndef FOR_RING0

#ifdef _WIN32

#include <winsock2.h>
#pragma comment (lib, "ws2_32.lib")
#include <ws2tcpip.h>

//! 用于保证sock环境初始化，注意构造失败抛出异常
class xWSA
  {
  public:
    xWSA();
  };

#define xSockErrno      (int)WSAGetLastError()
#define xSockExpt(msg)  throw std::runtime_error(xmsg() << std::string(msg"失败:") << xSockErrno);

#else   // _WIN32

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define SOCKET          int
#define closesocket     close
#define SD_BOTH         SHUT_RDWR
#define ioctlsocket     ioctl
#define xSockErrno      errno
#define xSockExpt(msg)  throw std::runtime_error(std::string(msg"失败:") + std::string(strerror(xSockErrno)));
#define FAR
#define IN

#endif  // _WIN32

#include <string>
#include <string.h>

#include "xlib_base.h"
#include "xline.h"
#include "xlog.h"

//! 由指定IP地址/IP地址串、端口/端口字串，组织sockaddr_in结构
/*!
  解析错误时，抛出runtime_error异常

  \code
    sockaddr_in addr = AddrInfo("127.0.0.1", "4210");
  \endcode
*/
sockaddr_in AddrInfo(const char* host, const char* ports);

#ifdef _WIN32

/*!
  解析错误时，抛出runtime_error异常

  \code
    sockaddr_in addr = AddrInfo(L"127.0.0.1", L"4210");
  \endcode
*/
sockaddr_in AddrInfo(PCWSTR host, PCWSTR ports);

#endif

/*!
  \code
    sockaddr_in addr = AddrInfo(0x7F000001, 4210);
  \endcode
*/
sockaddr_in AddrInfo(const uint32 host, const uint16 ports);

//! 由指定sockaddr_in结构，输出“#.#.#.#:#”串
std::string IpString(const sockaddr_in& addr);

//! 模板，用于指定协议建立socket
template<int TYPES> class xSocket
  {
  public:
    //! 自动创建socket
    xSocket()
      {
      _hSocket = socket(AF_INET, TYPES, 0);
      if(hSocket() <= 0)
        {
        xSockExpt("创建Socket");
        }
      }
    //! 指定接受已经存在的socket
    xSocket(const SOCKET hS)
      :_hSocket(hS)
      {
      if(hSocket() <= 0)
        {
        xSockExpt("初始化Socket");
        }
      }
    virtual ~xSocket()
      {
      if(hSocket() > 0)
        {
        shutdown(hSocket(), SD_BOTH);
        closesocket(hSocket());
        }
      }
    //! 返回SOCKET句柄
    SOCKET hSocket() const
      {
      return _hSocket;
      }
  private:
    xSocket(const xSocket&);
    xSocket& operator=(const xSocket&);
  private:
    SOCKET      _hSocket;
#ifdef _WIN32
    static const xWSA wsa;
#endif
  };

#define xSockTimeOut 50                 //默认收发延时50ms

//! 应用于TCP及UDP的基本通讯
template<int TYPES> class xSock
  {
  public:
    typedef xSock<TYPES>      _Myt;
  public:
    template<typename T> _Myt& operator<<(const T& argvs)
      {
      sendbuf << argvs;
      return *this;
      }
    template<typename T> _Myt& operator>>(T& argvs)
      {
      recvbuf >> argvs;
      return *this;
      }
  public:
    //! 返回SOCKET句柄
    SOCKET hSocket() const
      {
      return _socket.hSocket();
      }
    /*!
      当IP地址为0时，视作建立服务器端。否则视作建立客户端。\n
      建立服务器端将自动绑定本机端口。(注意与127.0.0.1区别)\n
      TCP协议建立服务器端，自动开启监听。\n
      TCP协议建立客户端时，自动连接指定IP。\n
      客户端默认收发延时50ms。服务器端无限制
    */
    xSock(const sockaddr_in& addr)
      :addrto(addr), _socket()
      {
      if(TYPES == SOCK_DGRAM)   recvbuf.reserve(0x400);
      if(addrto.sin_addr.s_addr == INADDR_ANY)
        {
        if(bind(hSocket(), (sockaddr*)&addrto, sizeof(addrto)))
          {
          xSockExpt("xSock绑定");
          }
        if(TYPES == SOCK_STREAM)
          {
          if(listen(hSocket(), 0))
            {
            xSockExpt("xSock监听");
            }
          }
        }
      else
        {
#ifdef _WIN32
        const int timeout = xSockTimeOut;
#else
        const timeval timeout = { xSockTimeOut / 1000, (xSockTimeOut % 1000) * 1000 };
#endif
        opt(SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
        opt(SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
        if(TYPES == SOCK_STREAM)
          {
          if(connect(hSocket(), (sockaddr*)&addrto, sizeof(addrto)))
            {
            xSockExpt("xSock连接");
            }
          }
        }
      }
    //! 指定SOCKET，作为客户端建立xSock
    xSock(const SOCKET hS, const sockaddr_in& addr)
      :addrto(addr), _socket(hS)
      {
#ifdef _WIN32
      const int timeout = xSockTimeOut;
#else
      const timeval timeout = { xSockTimeOut / 1000, (xSockTimeOut % 1000) * 1000 };
#endif
      opt(SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
      opt(SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
      }
    //! 用于服务器端accept
    _Myt* apt()
      {
      socklen_t namelen = sizeof(addrfrom);
      const SOCKET hS = accept(hSocket(), (sockaddr*)&addrfrom, &namelen);
      if(hS <= 0)
        {
        xSockExpt("xSock接受连接");
        }
      return new _Myt(hS, addrfrom);
      }
    //! 用于setsockopt
    void opt(int level, int optname, const char FAR * optval, int optlen)
      {
      if(setsockopt(hSocket(), level, optname, optval, optlen))
        {
        xSockExpt("xSock设置");
        }
      }
    //! 用于基本发送，注意不抛出异常
    bool send(IN const char FAR * buf, IN int len)
      {
      while(len > 0)
        {
        int sendlen;
        if(TYPES == SOCK_STREAM)
          sendlen = ::send(hSocket(), buf, len, 0);
        else
          sendlen = ::sendto(hSocket(), buf, len, 0,
          (const sockaddr *)&addrto, sizeof(addrto));

        if(sendlen <= 0)
          {
          xerr << "xSock send Err:" << xSockErrno;
          return false;
          }
        len -= sendlen;
        buf += sendlen;
        }
      return true;
      }
    //! 用于基本发送，主要应用于结构体
    template<typename T> bool send(T const& v)
      {
      return send((char*)&v, sizeof(T));
      }
    //! 发送数据，注意发送失败，抛出异常。发送成功，清除缓冲
    _Myt& send()
      {
      if(!send((char*)sendbuf.c_str(), (int)sendbuf.size()))
        {
        xSockExpt("xSock发送");
        }
      sendbuf.clear();
      return *this;
      }
    //! 发送数据，会附加数据头。
    _Myt& sendmkhead()
      {
      sendbuf.mkhead();
      return send();
      }
    //! 用于基本接收，注意需要根据返回值自行处理
    int recv(char FAR * buf, IN int len)
      {
      int recvlen;
      if(TYPES == SOCK_STREAM)
        {
        recvlen = ::recv(hSocket(), buf, len, 0);
        }
      else
        {
        socklen_t addrfromlen = sizeof(addrfrom);
        memset(&addrfrom, 0, sizeof(addrfrom));
        recvlen = ::recvfrom(hSocket(), buf, len, 0,
          (sockaddr*)&addrfrom, &addrfromlen);
        }
      return recvlen;
      }
    //! 用于基本接收，主要应用于结构体
    template<typename T> int recv(T& v)
      {
      return recv((char*)&v, sizeof(T));
      }
    //! 接收数据
    int recv()
      {
      int mainrecvlen = 0;
      while(mainrecvlen >= 0)
        {
        if(recvbuf.capacity() - recvbuf.size() == 0)
          recvbuf.reserve(recvbuf.capacity() + 0x40);
        unsigned char* lprecv = const_cast<unsigned char*>(recvbuf.c_str()) + recvbuf.size();
        const int recvlen = recv((char*)lprecv, (int)(recvbuf.capacity() - recvbuf.size()));
        if(recvlen == 0)
          {
          xtrace << "xSock recv close";
          return 0;
          }
        if(recvlen < 0)
          {
          int codes = xSockErrno;
          xtrace << "xSock recv Err:" << codes;
          switch(codes)
            {
            case ETIMEDOUT:  return mainrecvlen;
            case ECONNRESET:  return mainrecvlen;
            case EMSGSIZE:
              //UDP会出现这个错误，此时数据已经被截断
              {
              const int len = (int)(recvbuf.capacity() - recvbuf.size());
              recvbuf.append((const unsigned char*)lprecv, mainrecvlen);
              recvbuf.reserve(recvbuf.capacity() + 0x40);
              return mainrecvlen + len;
              }
            case ENOTCONN:
              xSockExpt("xSock无连接");
            default:
              xSockExpt("xSock接收");
            }
          }
        recvbuf.append((const unsigned char*)lprecv, recvlen);
        mainrecvlen += recvlen;
        if(recvbuf.capacity() - recvbuf.size() != 0)
          break;//如果缓冲被填满，可能还有数据需要接收
        }
      return mainrecvlen;
      }
  public:
    netline     sendbuf;
    netline     recvbuf;
    //因查询的需要，又不愿意写独立的访问函数，故意放置为public
    sockaddr_in addrto;
    sockaddr_in addrfrom;
  private:
    xSocket<TYPES> _socket;
  };

typedef xSock<SOCK_STREAM>    xTCP;
typedef xSock<SOCK_DGRAM>     xUDP;

typedef xUDP& (*xUDP_func)(xUDP&);
typedef xTCP& (*xTCP_func)(xTCP&);

//! 重载使xSock能以流形式操作数据，这个重载无法做在类内部
xUDP& operator<<(xUDP& s, xUDP_func pfn);
xTCP& operator<<(xTCP& s, xTCP_func pfn);

//! 重载使xSock能以流形式发送数据
xUDP& xsend(xUDP& s);
xTCP& xsend(xTCP& s);
xUDP& xxsend(xUDP& s);
xTCP& xxsend(xTCP& s);

#endif  // FOR_RING0

#endif  // _XLIB_XSOCK_H_