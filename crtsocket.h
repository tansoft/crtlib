#pragma once

#include "crtlib.h"
#include "crtsystem.h"
#ifdef _WIN32
	#include <iphlpapi.h>
	#pragma comment(lib,"Iphlpapi.lib")
#else
	#include <sys/types.h>
	#include <ifaddrs.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

namespace crtfun {
	typedef struct _crtipv4{
		uint32_t ip;
		uint16_t port;
		_crtipv4(){
			ip=0;
			port=0;
		}
		_crtipv4(sockaddr_in *addr){
			ip=addr->sin_addr.s_addr;
			port=addr->sin_port;
		}
		_crtipv4(uint32_t nip,uint16_t nport){
			ip=nip;
			port=nport;
		}
		bool operator < (const _crtipv4 info) const{
			if (ip==info.ip) return port < info.port;
			return ip < info.ip;
		}
	}crtipv4;
	static int init_deamon() {
	#ifndef _WIN32
		int pid=fork();
		//kill parent process
		if (pid) exit(0);
		//fork error,exit
		else if(pid<0) exit(1);
		//the frist child process,process become the new session and process group leader
		setsid();
		//leave from the terminal
		//kill the frist child process
		pid=fork();
		if (pid) exit(0);
		//fork error,exit
		else if(pid< 0) exit(1);
		//the secord child process is not the session group leader
		//close all open file handle
		for(int i=0;i<NOFILE;++i) close(i);
		//reset the file creation mask
		umask(0);
	#endif
		return 0;
	}
	static int init_socket() {
	#ifdef _WIN32
		WSADATA wsaData;
		return WSAStartup(MAKEWORD(2,2), &wsaData);
	#else
		signal(SIGPIPE, SIG_IGN);
		signal(SIGUSR1, SIG_IGN);
		signal(SIGUSR2, SIG_IGN);
		signal(SIGCHLD, SIG_IGN);
		signal(SIGALRM, SIG_IGN);
		/*sigset_t signal_mask;
		sigemptyset (&signal_mask);
		sigaddset (&signal_mask, SIGPIPE);
		return pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);*/
		return 0;
	#endif
	}
	static void set_tcp_socket_nodelay_behavior(SOCKET s,bool nodelay=true){
#if defined(_WIN32) && !defined(_WIN32_WCE)
		BOOL nodelayval = nodelay?TRUE:FALSE;
		setsockopt(s,IPPROTO_TCP,TCP_NODELAY,(const char *)&nodelayval,sizeof(BOOL));
#endif
	}
	static void set_udp_socket_autoclose_behavior(SOCKET s,int bautoclose=0) {
	#ifdef _WIN32
		#ifndef SIO_UDP_CONNRESET
			#define	SIO_UDP_CONNRESET	_WSAIOW(IOC_VENDOR,12)
		#endif
		if (crtosiswinnt()) {
			DWORD dwBytesReturned=0;
			WSAIoctl(s,SIO_UDP_CONNRESET,&bautoclose,sizeof(bautoclose),NULL,0,&dwBytesReturned,NULL,NULL);
		}
	#endif
	}
	static bool is_ip_string(const char *ip) {
		size_t len = strlen(ip);
		if (len == 0)
			return false;
		for (size_t i = 0; i < len; i++) {
			char b = *(ip + i);
			if ((b < '0' || b > '9') && b != '.')
				return false;
		}
		return true;
	}
	static uint32_t get_ip_by_name(const char *host) {
		struct hostent *p;
		if (is_ip_string(host)) return (uint32_t) inet_addr(host);
		p = gethostbyname(host);
		if (!p) return 0;
		return (uint32_t)((struct in_addr *) p->h_addr)->s_addr;
	}
	//ip network order,port host order
	static uint32_t get_ip_port_by_name(const char *host, uint16_t *port) {
		const char *tmp;
		char buf[4096];
		tmp = strstr(host, ":");
		if (port) *port = 0;
		if (tmp) {
			if (port) *port = (uint16_t) atoi(tmp + 1);
			memcpy(buf, host, tmp - host);
			buf[tmp - host] = '\0';
			tmp = buf;
		} else
			tmp = host;
		return get_ip_by_name(tmp);
	}
	static string inet_ntoa_safe_u32n(uint32_t ip){
		char dat[20];
		unsigned char *buf = (unsigned char *) &ip;
		sprintf(dat,"%u.%u.%u.%u",(unsigned int)buf[0],(unsigned int)buf[1],(unsigned int)buf[2],(unsigned int)buf[3]);
		return dat;
	}
	//因为系统的存在全局buffer问题
	static string inet_ntoa_safe(sockaddr_in *addr){
		char dat[20];
		unsigned char *buf = (unsigned char *) &addr->sin_addr;
		sprintf(dat,"%u.%u.%u.%u",(unsigned int)buf[0],(unsigned int)buf[1],(unsigned int)buf[2],(unsigned int)buf[3]);
		return dat;
	}
	//ip and port
	static string print_host(sockaddr_in *addr){
		char dat[25];
		unsigned char *buf = (unsigned char *) &addr->sin_addr;
		sprintf(dat,"%u.%u.%u.%u:%u",(unsigned int)buf[0],(unsigned int)buf[1],
			(unsigned int)buf[2],(unsigned int)buf[3],(unsigned int)ntohs(addr->sin_port));
		return dat;
	}
	static string print_host(crtipv4 ip){
		char dat[25];
		unsigned char *buf = (unsigned char *) &ip.ip;
		sprintf(dat,"%u.%u.%u.%u:%u",(unsigned int)buf[0],(unsigned int)buf[1],
			(unsigned int)buf[2],(unsigned int)buf[3],(unsigned int)ntohs(ip.port));
		return dat;
	}
	//ip is network order,port is local order
	static string print_host(uint32_t ip,uint16_t port){
		char dat[25];
		unsigned char *buf = (unsigned char *) &ip;
		sprintf(dat,"%u.%u.%u.%u:%u",(unsigned int)buf[0],(unsigned int)buf[1],
			(unsigned int)buf[2],(unsigned int)buf[3],port);
		return dat;
	}
	//listenaddr为网络序，listenport为本地序
	static SOCKET create_listen_socket(uint16_t listenport, uint32_t listenaddr = 0, bool reuse = false, bool udp = false) {
		sockaddr_in addr;
		SOCKET s = socket(AF_INET, udp?SOCK_DGRAM:SOCK_STREAM, udp?IPPROTO_UDP:IPPROTO_TCP);
		if (s == INVALID_SOCKET) return INVALID_SOCKET;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = listenaddr;
		addr.sin_port = htons(listenport);
		if (bind(s, (sockaddr *) &addr, sizeof(addr)) != 0) {
			closesocket(s);
			return INVALID_SOCKET;
		}
		if (reuse) {
			int on = 1;
			if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on))!=0) {
				closesocket(s);
				return INVALID_SOCKET;
			}
		}
		if (udp) set_udp_socket_autoclose_behavior(s);
		return s;
	}
	static bool set_socket_buffer_size(SOCKET s, int sendbufsize=81920, int recvbufsize=81920) {
		if (setsockopt(s,SOL_SOCKET,SO_SNDBUF,(char *)&sendbufsize,sizeof(sendbufsize))==0 &&
			setsockopt(s,SOL_SOCKET,SO_RCVBUF,(char *)&recvbufsize,sizeof(recvbufsize))==0)
			return true;
		return false;
	}
#ifdef _WIN32
	#define IsWouldBlock(errorno)			(errorno==WSAEWOULDBLOCK || errorno==WSAENOTCONN)//connecting
#else
	#define IsWouldBlock(errorno)			(errorno==EINPROGRESS || errorno==EWOULDBLOCK || errorno==EINTR || errorno==EAGAIN || errorno==ENOTCONN)
#endif
	//0 for success, other for error
	static int set_non_block(SOCKET fd, int block = 0) {
	#ifdef _WIN32
		int iMode = block? 0:1;
		return ioctlsocket(fd, FIONBIO, (u_long FAR*) &iMode);
	#else
		int flags = fcntl(fd, F_GETFL, 0);
		if (flags < 0)
			return flags;
		if (block)
			flags &= ~O_NONBLOCK;
		else
			flags |= O_NONBLOCK;
		if (fcntl(fd, F_SETFL, flags) < 0)
			return -1;
		return 0;
	#endif
	}
	#define CRTSOCKETEVENTREAD	0x1
	#define CRTSOCKETEVENTWRITE 0x2
	#define CRTSOCKETEVENTERROR 0x4
	//return 0 have event -1 socket error 1 timeout
	static int socket_select(SOCKET s,int timeout=0,int mode=CRTSOCKETEVENTREAD|CRTSOCKETEVENTWRITE|CRTSOCKETEVENTERROR) {
		struct timeval tm,*tmp;
		fd_set set;
		fd_set *r,*w,*e;
		int ret;
		FD_ZERO(&set);
		FD_SET(s,&set);
		if (timeout==0) tmp=NULL;
		else {
			tm.tv_sec = timeout/1000;
			tm.tv_usec = (timeout%1000)*1000;
			tmp=&tm;
		}
		r=(mode & CRTSOCKETEVENTREAD)?&set:NULL;
		w=(mode & CRTSOCKETEVENTWRITE)?&set:NULL;
		e=(mode & CRTSOCKETEVENTERROR)?&set:NULL;
		ret=select((int)s+1,r,w,e,tmp);
		if (ret==0) return 1;
		else if (ret==-1) return -1;
		return 0;
	}
	//0 for success,!= for error -1 socket error, 1 timeout
	static int wait_connect(SOCKET s,int timeout=0) {
		int ret=socket_select(s,timeout,CRTSOCKETEVENTWRITE);
		//recheck if socket is ok
		if (ret==0) {
			socklen_t ret1=sizeof(int);
			getsockopt(s,SOL_SOCKET,SO_ERROR,(char *)&ret,&ret1);
		}
		return ret;
	}
	static SOCKET create_connect_socket(sockaddr_in *addr, int block = 0) {
		SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (s == INVALID_SOCKET) return INVALID_SOCKET;
		if (set_non_block(s, block)!=0) {
			closesocket(s);
			return INVALID_SOCKET;
		}
		if (connect(s, (sockaddr *) addr, sizeof(sockaddr_in)) != 0) {
			if (IsWouldBlock(GetLastError()))
				return s;
			closesocket(s);
			return INVALID_SOCKET;
		}
		return s;
	}
	//ip network order,port local order
	static SOCKET create_connect_socket_by_ip(uint32_t ip,uint16_t port,int block=0)
	{
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = ip;
		addr.sin_port = htons(port);
		return create_connect_socket(&addr, block);
	}
	static int recv_timeout(SOCKET s, char *buf, int len, int flag, int timeout=0) {
		int l=recv(s, buf, len, flag);
		if (l <= 0) {
			if (l<0 && IsWouldBlock(GetLastError())) {
				l = socket_select(s,timeout,CRTSOCKETEVENTREAD);
				if (l<0 || l==1) return -1;
				return recv(s, buf, len, flag);
			}
			return l;
		}
		return l;
	}
	static int ensure_send(SOCKET s, const char *buf, int len) {
		int sed = 0, l;
		while (sed < len) {
			l = min(len - sed, (int) 4096);
			l = send(s, buf + sed, l, 0);
			if (l <= 0) {
				if (l<0 && IsWouldBlock(GetLastError())) {
					l = socket_select(s,0,CRTSOCKETEVENTWRITE);
					if (l<0) return l;
					continue;
				}
				return l;
			}
			sed += l;
		}
		return sed;
	}
	static int ensure_send_str(SOCKET s, const char *buf) {return ensure_send(s, buf, (int) strlen(buf));}
	class crtadapterinfo{
	public:
		crtadapterinfo(){
			m_cnt=0;
		#ifdef _WIN32
			DWORD dwRetVal = 0;
			m_adapterinfo = (IP_ADAPTER_INFO *) new char[sizeof(IP_ADAPTER_INFO)];
			ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
			if (m_adapterinfo) {
				if (GetAdaptersInfo( m_adapterinfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
					delete [] (char *)m_adapterinfo;
					m_adapterinfo = (IP_ADAPTER_INFO *) new char[ulOutBufLen]; 
				}
			}
			if (m_adapterinfo) {
				if ((dwRetVal = GetAdaptersInfo( m_adapterinfo, &ulOutBufLen)) != NO_ERROR) {
					delete [] (char *)m_adapterinfo;
					m_adapterinfo=NULL;
				} else {
					PIP_ADAPTER_INFO pAdapter = m_adapterinfo;
					while (pAdapter) {
						pAdapter=pAdapter->Next;
						m_cnt++;
					}
				}
			}
		#else
			struct ifaddrs * ifAddrStruct=NULL;
			void * tmpAddrPtr=NULL;
			getifaddrs(&ifAddrStruct);
			while (ifAddrStruct!=NULL) {
				if (ifAddrStruct->ifa_addr->sa_family==AF_INET) { // check it is IP4
					// is a valid IP4 Address
					char addressBuffer[INET_ADDRSTRLEN];
					struct _adapterinfo info;
					tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
					inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
					info.ip=addressBuffer;
					char *end=strrchr(addressBuffer,'.');
					if (end) {
						*(end+1)='1';
						*(end+2)='\0';
						info.gw=addressBuffer;//fake gw, only guess it
					} else
						info.gw="0.0.0.0";
					tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_netmask)->sin_addr;
					inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
					info.mask=addressBuffer;
					info.name=ifAddrStruct->ifa_name;
					m_adapterinfo.push_back(info);
					//printf("%s IP Address %s\n", ifAddrStruct->ifa_name, addressBuffer); 
					m_cnt++;
				}/* else if (ifAddrStruct->ifa_addr->sa_family==AF_INET6) { // check it is IP6
					// is a valid IP6 Address
					tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
					char addressBuffer[INET6_ADDRSTRLEN];
					inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
					printf("%s IP Address %s\n", ifAddrStruct->ifa_name, addressBuffer); 
				}*/
				ifAddrStruct=ifAddrStruct->ifa_next;
			}
		#endif
		}
		virtual ~crtadapterinfo(){
		#ifdef _WIN32
			if (m_adapterinfo)
				delete [] (char*)m_adapterinfo;
		#endif
		}
		inline unsigned int count(){return m_cnt;}
		inline string getipstring(int idx) {
		#ifdef _WIN32
			PIP_ADAPTER_INFO p = adapterinfo(idx);
			if (p) return p->IpAddressList.IpAddress.String;
			return "";
		#else
			return m_adapterinfo[idx].ip;
		#endif
		}
		inline string getipmask(int idx) {
		#ifdef _WIN32
			PIP_ADAPTER_INFO p = adapterinfo(idx);
			if (p) return p->IpAddressList.IpMask.String;
			return "";
		#else
			return m_adapterinfo[idx].mask;
		#endif
		}
		inline string getgwstring(int idx)
		{
		#ifdef _WIN32
			PIP_ADAPTER_INFO p = adapterinfo(idx);
			if (p) return p->GatewayList.IpAddress.String;
			return "";
		#else
			return m_adapterinfo[idx].gw;
		#endif
		}
		inline string getname(int idx)
		{
		#ifdef _WIN32
			PIP_ADAPTER_INFO p = adapterinfo(idx);
			if (p) return p->AdapterName;
			return "";
		#else
			return m_adapterinfo[idx].name;
		#endif
		}
		typedef enum _crtadaptertype{
			adaptertype_ethernet,
			adaptertype_ppp,
			adaptertype_loopback,
			adaptertype_other
		}crtadaptertype;
		/*MIB_IF_TYPE_XXX*/
		crtadaptertype adaptertype(int idx){
		#ifdef _WIN32
			PIP_ADAPTER_INFO p = adapterinfo(idx);
			if (p) {
				if (p->Type==MIB_IF_TYPE_ETHERNET) return adaptertype_ethernet;
				else if (p->Type==MIB_IF_TYPE_PPP) return adaptertype_ppp;
				else if (p->Type==MIB_IF_TYPE_LOOPBACK) return adaptertype_loopback;
				return adaptertype_other;
			}
			return adaptertype_other;
		#else
			return adaptertype_ethernet;
		#endif
		}
		static uint32_t getfitip(){
			crtadapterinfo info;
			uint32_t intip=0;
			for(unsigned int i=0;i<info.count();i++) {
				if (get_ip_by_name(info.getgwstring(i).c_str())!=0) {
					//the out going adapter
					intip=get_ip_by_name(info.getipstring(i).c_str());
					if (intip!=0) break;
				} else {
					if (info.adaptertype(i)==adaptertype_ethernet) {
						//other ethernet adapter,temp to use
						uint32_t tmpip=get_ip_by_name(info.getipstring(i).c_str());
						if (tmpip!=0) intip=tmpip;
					}
				}
			}
			return intip;
		}
		static string getfitipstring(){
			crtadapterinfo info;
			string intip="0.0.0.0";
			for(unsigned int i=0;i<info.count();i++) {
				if (get_ip_by_name(info.getgwstring(i).c_str())!=0) {
					//the out going adapter
					intip=info.getipstring(i);
					if (get_ip_by_name(intip.c_str())!=0) break;
				} else {
					if (info.adaptertype(i)==adaptertype_ppp || info.adaptertype(i)==adaptertype_ethernet) {
						//other ethernet adapter,temp to use
						uint32_t tmpip=get_ip_by_name(info.getipstring(i).c_str());
						if (tmpip!=0) intip=info.getipstring(i);
					}
				}
			}
			return intip;
		}
	protected:
	#ifdef _WIN32
		PIP_ADAPTER_INFO adapterinfo(int idx) {
			PIP_ADAPTER_INFO p = m_adapterinfo;
			while (p && idx>0) {
				p=p->Next;
				idx--;
			}
			return p;
		}
		PIP_ADAPTER_INFO m_adapterinfo;
	#else
		struct _adapterinfo{
			string ip;
			string mask;
			string gw;
			string name;
		};
		vector<struct _adapterinfo> m_adapterinfo;
	#endif
		unsigned int m_cnt;
	};
};
