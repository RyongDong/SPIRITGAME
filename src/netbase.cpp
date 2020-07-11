// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2011-2012 Litecoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "netbase.h"
#include "util.h"

#ifndef WIN32
#include <sys/fcntl.h>
#endif

#include "strlcpy.h"
#include <boost/algorithm/string/case_conv.hpp> // for to_lower()

using namespace std;

// Settings
typedef std::pair<CService, int> proxyType;
static proxyType proxyInfo[NET_MAX];
static proxyType nameproxyInfo;
int nConnectTimeout = 5000;
bool fNameLookup = false;

static const unsigned char pchIPv4[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };

enum Network ParseNetwork(std::string net) {
    boost::to_lower(net);
    if (net == "ipv4") return NET_IPV4;
    if (net == "ipv6") return NET_IPV6;
    if (net == "tor")  return NET_TOR;
    if (net == "i2p")  return NET_I2P;
    return NET_UNROUTABLE;
}

void SplitHostPort(std::string in, int &portOut, std::string &hostOut) {
    size_t colon = in.find_last_of(':');
    // if a : is found, and it either follows a [...], or no other : is in the string, treat it as port separator
    bool fHaveColon = colon != in.npos;
    bool fBracketed = fHaveColon && (in[0]=='[' && in[colon-1]==']'); // if there is a colon, and in[0]=='[', colon is not 0, so in[colon-1] is safe
    bool fMultiColon = fHaveColon && (in.find_last_of(':',colon-1) != in.npos);
    if (fHaveColon && (colon==0 || fBracketed || !fMultiColon)) {
        char *endp = NULL;
        int n = strtol(in.c_str() + colon + 1, &endp, 10);
        if (endp && *endp == 0 && n >= 0) {
            in = in.substr(0, colon);
            if (n > 0 && n < 0x10000)
                portOut = n;
        }
    }
    if (in.size()>0 && in[0] == '[' && in[in.size()-1] == ']')
        hostOut = in.substr(1, in.size()-2);
    else
        hostOut = in;
}

bool static LookupIntern(const char *pszName, std::vector<CNetAddr>& vIP, unsigned int nMaxSolutions, bool fAllowLookup)
{
    vIP.clear();

    {
        CNetAddr addr;
        if (addr.SetSpecial(std::string(pszName))) {
            vIP.push_back(addr);
            return true;
        }
    }

    struct addrinfo aiHint;
    memset(&aiHint, 0, sizeof(struct addrinfo));

    aiHint.ai_socktype = SOCK_STREAM;
    aiHint.ai_protocol = IPPROTO_TCP;
#ifdef WIN32
#  ifdef USE_IPV6
    aiHint.ai_family = AF_UNSPEC;
#  else
    aiHint.ai_family = AF_INET;
#  endif
    aiHint.ai_flags = fAllowLookup ? 0 : AI_NUMERICHOST;
#else
#  ifdef USE_IPV6
    aiHint.ai_family = AF_UNSPEC;
#  else
    aiHint.ai_family = AF_INET;
#  endif
    aiHint.ai_flags = fAllowLookup ? AI_ADDRCONFIG : AI_NUMERICHOST;
#endif
    struct addrinfo *aiRes = NULL;
    int nErr = getaddrinfo(pszName, NULL, &aiHint, &aiRes);
    if (nErr)
        return false;

    struct addrinfo *aiTrav = aiRes;
    while (aiTrav != NULL && (nMaxSolutions == 0 || vIP.size() < nMaxSolutions))
    {
        if (aiTrav->ai_family == AF_INET)
        {
            assert(aiTrav->ai_addrlen >= sizeof(sockaddr_in));
            vIP.push_back(CNetAddr(((struct sockaddr_in*)(aiTrav->ai_addr))->sin_addr));
        }

#ifdef USE_IPV6
        if (aiTrav->ai_family == AF_INET6)
        {
            assert(aiTrav->ai_addrlen >= sizeof(sockaddr_in6));
            vIP.push_back(CNetAddr(((struct sockaddr_in6*)(aiTrav->ai_addr))->sin6_addr));
        }
#endif

        aiTrav = aiTrav->ai_next;
    }

    freeaddrinfo(aiRes);

    return (vIP.size() > 0);
}

bool LookupHost(const char *pszName, std::vector<CNetAddr>& vIP, unsigned int nMaxSolutions, bool fAllowLookup)
{
    if (pszName[0] == 0)
        return false;
    char psz[256];
    char *pszHost = psz;
    strlcpy(psz, pszName, sizeof(psz));
    if (psz[0] == '[' && psz[strlen(psz)-1] == ']')
    {
        pszHost = psz+1;
        psz[strlen(psz)-1] = 0;
    }

    return LookupIntern(pszHost, vIP, nMaxSolutions, fAllowLookup);
}

bool LookupHostNumeric(const char *pszName, std::vector<CNetAddr>& vIP, unsigned int nMaxSolutions)
{
    return LookupHost(pszName, vIP, nMaxSolutions, false);
}

bool Lookup(const char *pszName, std::vector<CService>& vAddr, int portDefault, bool fAllowLookup, unsigned int nMaxSolutions)
{
    if (pszName[0] == 0)
        return false;
    int port = portDefault;
    std::string hostname = "";
    SplitHostPort(std::string(pszName), port, hostname);

    std::vector<CNetAddr> vIP;
    bool fRet = LookupIntern(hostname.c_str(), vIP, nMaxSolutions, fAllowLookup);
    if (!fRet)
        return false;
    vAddr.resize(vIP.size());
    for (unsigned int i = 0; i < vIP.size(); i++)
        vAddr[i] = CService(vIP[i], port);
    return true;
}

bool Lookup(const char *pszName, CService& addr, int portDefault, bool fAllowLookup)
{
    std::vector<CService> vService;
    bool fRet = Lookup(pszName, vService, portDefault, fAllowLookup, 1);
    if (!fRet)
        return false;
    addr = vService[0];
    return true;
}

bool LookupNumeric(const char *pszName, CService& addr, int portDefault)
{
    return Lookup(pszName, addr, portDefault, false);
}

bool static Socks4(const CService &addrDest, SOCKET& hSocket)
{
    printf("SOCKS4 connecting %s\n", addrDest.ToString().c_str());
    if (!addrDest.IsIPv4())
    {
        closesocket(hSocket);
        return error("Proxy destination is not IPv4");
    }
    char pszSocks4IP[] = "\4\1\0\0\0\0\0\0user";
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (!addrDest.GetSockAddr((struct sockaddr*)&addr, &len) || addr.sin_family != AF_INET)
    {
        closesocket(hSocket);
        return error("Cannot get proxy destination address");
    }
    memcpy(pszSocks4IP + 2, &addr.sin_port, 2);
    memcpy(pszSocks4IP + 4, &addr.sin_addr, 4);
    char* pszSocks4 = pszSocks4IP;
    int nSize = sizeof(pszSocks4IP);

    int ret = send(hSocket, pszSocks4, nSize, MSG_NOSIGNAL);
    if (ret != nSize)
    {
        closesocket(hSocket);
        return error("Error sending to proxy");
    }
    char pchRet[8];
    if (recv(hSocket, pchRet, 8, 0) != 8)
    {
        closesocket(hSocket);
        return error("Error reading proxy response");
    }
    if (pchRet[1] != 0x5a)
    {
        closesocket(hSocket);
        if (pchRet[1] != 0x5b)
            printf("ERROR: Proxy returned error %d\n", pchRet[1]);
        return false;
    }
    printf("SOCKS4 connected %s\n", addrDest.ToString().c_str());
    return true;
}

bool static Socks5(string strDest, int port, SOCKET& hSocket)
{
    printf("SOCKS5 connecting %s\n", strDest.c_str());
    if (strDest.size() > 255)
    {
        closesocket(hSocket);
        return error("Hostname too long");
    }
    char pszSocks5Init[] = "\5\1\0";
    char *pszSocks5 = pszSocks5Init;
    ssize_t nSize = sizeof(pszSocks5Init) - 1;

    ssize_t ret = send(hSocket, pszSocks5, nSize, MSG_NOSIGNAL);
    if (ret != nSize)
    {
        closesocket(hSocket);
        return error("Error sending to proxy");
    }
    char pchRet1[2];
    if (recv(hSocket, pchRet1, 2, 0) != 2)
    {
        closesocket(hSocket);
        return error("Error reading proxy response");
    }
    if (pchRet1[0] != 0x05 || pchRet1[1] != 0x00)
    {
        closesocket(hSocket);
        return error("Proxy failed to initialize");
    }
    string strSocks5("\5\1");
    strSocks5 += '\000'; strSocks5 += '\003';
    strSocks5 += static_cast<char>(std::min((int)strDest.size(), 255));
    strSocks5 += strDest; 
    strSocks5 += static_cast<char>((port >> 8) & 0xFF);
    strSocks5 += static_cast<char>((port >> 0) & 0xFF);
    ret = send(hSocket, strSocks5.c_str(), strSocks5.size(), MSG_NOSIGNAL);
    if (ret != (ssize_t)strSocks5.size())
    {
        closesocket(hSocket);
        return error("Error sending to proxy");
    }
    char pchRet2[4];
    if (recv(hSocket, pchRet2, 4, 0) != 4)
    {
        closesocket(hSocket);
        return error("Error reading proxy response");
    }
    if (pchRet2[0] != 0x05)
    {
        closesocket(hSocket);
        return error("Proxy failed to accept request");
    }
    if (pchRet2[1] != 0x00)
    {
        closesocket(hSocket);
        switch (pchRet2[1])
        {
            case 0x01: return error("Proxy error: general failure");
            case 0x02: return error("Proxy error: connection not allowed");
            case 0x03: return error("Proxy error: network unreachable");
            case 0x04: return error("Proxy error: host unreachable");
            case 0x05: return error("Proxy error: connection refused");
            case 0x06: return error("Proxy error: TTL expired");
            case 0x07: return error("Proxy error: protocol error");
            case 0x08: return error("Proxy error: address type not supported");
            default:   return error("Proxy error: unknown");
        }
    }
    if (pchRet2[2] != 0x00)
    {
        closesocket(hSocket);
        return error("Error: malformed proxy response");
    }
    char pchRet3[256];
    switch (pchRet2[3])
    {
        case 0x01: ret = recv(hSocket, pchRet3, 4, 0) != 4; break;
        case 0x04: ret = recv(hSocket, pchRet3, 16, 0) != 16; break;
        case 0x03:
        {
            ret = recv(hSocket, pchRet3, 1, 0) != 1;
            if (ret)
                return error("Error reading from proxy");
            int nRecv = pchRet3[0];
            ret = recv(hSocket, pchRet3, nRecv, 0) != nRecv;
            break;
        }
        default: closesocket(hSocket); return error("Error: malformed proxy response");
    }
    if (ret)
    {
        closesocket(hSocket);
        return error("Error reading from proxy");
    }
    if (recv(hSocket, pchRet3, 2, 0) != 2)
    {
        closesocket(hSocket);
        return error("Error reading from proxy");
    }
    printf("SOCKS5 connected %s\n", strDest.c_str());
    return true;
}

bool static ConnectSocketDirectly(const CService &addrConnect, SOCKET& hSocketRet, int nTimeout)
{
    hSocketRet = INVALID_SOCKET;

#ifdef USE_IPV6
    struct sockaddr_storage sockaddr;
#else
    struct sockaddr sockaddr;
#endif
    socklen_t len = sizeof(sockaddr);
    if (!addrConnect.GetSockAddr((struct sockaddr*)&sockaddr, &len)) {
        printf("Cannot connect to %s: unsupported network\n", addrConnect.ToString().c_str());
        return false;
    }

    SOCKET hSocket = socket(((struct sockaddr*)&sockaddr)->sa_family, SOCK_STREAM, IPPROTO_TCP);
    if (hSocket == INVALID_SOCKET)
        return false;
#ifdef SO_NOSIGPIPE
    int set = 1;
    setsockopt(hSocket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
#endif

#ifdef WIN32
    u_long fNonblock = 1;
    if (ioctlsocket(hSocket, FIONBIO, &fNonblock) == SOCKET_ERROR)
#else
    int fFlags = fcntl(hSocket, F_GETFL, 0);
    if (fcntl(hSocket, F_SETFL, fFlags | O_NONBLOCK) == -1)
#endif
    {
        closesocket(hSocket);
        return false;
    }

    if (connect(hSocket, (struct sockaddr*)&sockaddr, len) == SOCKET_ERROR)
    {
        // WSAEINVAL is here because some legacy version of winsock uses it
        if (WSAGetLastError() == WSAEINPROGRESS || WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINVAL)
        {
            struct timeval timeout;
            timeout.tv_sec  = nTimeout / 1000;
            timeout.tv_usec = (nTimeout % 1000) * 1000;

            fd_set fdset;
            FD_ZERO(&fdset);
            FD_SET(hSocket, &fdset);
            int nRet = select(hSocket + 1, NULL, &fdset, NULL, &timeout);
            if (nRet == 0)
            {
                printf("connection timeout\n");
                closesocket(hSocket);
                return false;
            }
            if (nRet == SOCKET_ERROR)
            {
                printf("select() for connection failed: %i\n",WSAGetLastError());
                closesocket(hSocket);
                return false;
            }
            socklen_t nRetSize = sizeof(nRet);
#ifdef WIN32
            if (getsockopt(hSocket, SOL_SOCKET, SO_ERROR, (char*)(&nRet), &nRetSize) == SOCKET_ERROR)
#else
            if (getsockopt(hSocket, SOL_SOCKET, SO_ERROR, &nRet, &nRetSize) == SOCKET_ERROR)
#endif
            {
                printf("getsockopt() for connection failed: %i\n",WSAGetLastError());
                closesocket(hSocket);
                return false;
            }
            if (nRet != 0)
            {
                printf("connect() failed after select(): %s\n",strerror(nRet));
                closesocket(hSocket);
                return false;
            }
        }
#ifdef WIN32
        else if (WSAGetLastError() != WSAEISCONN)
#else
        else
#endif
        {
            printf("connect() failed: %i\n",WSAGetLastError());
            closesocket(hSocket);
            return false;
        }
    }

    // this isn't even strictly necessary
    // CNode::ConnectNode immediately turns the socket back to non-blocking
    /