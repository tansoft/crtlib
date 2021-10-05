#pragma once

#include "crtsocket.h"
#include "crtbuffer.h"
#include "crtobjmap.h"
#if !defined(_WIN32) && !defined(__APPLE__)
	#include <sys/epoll.h>
#endif

/**
* @brief 网络模型类
* @author Barry(barrytan@21cn.com,QQ:20962493)
*/
/**<pre>
  使用Sample：
</pre>*/

namespace crtfun{
	class crtsocketmodel;
	class icrtsocketlayer{
	public:
		icrtsocketlayer(){}
		virtual ~icrtsocketlayer(){}
		virtual string getname()=0;
		virtual SOCKET socket_route(int ad,int type,int protocol)=0;
		virtual int connect_route(SOCKET s,const sockaddr* addr,int addrsize)=0;
		virtual SOCKET accept_route(SOCKET s,sockaddr* addr,int *addrlen)=0;
		virtual int bind_route(SOCKET s,const sockaddr *addr, int addrlen)=0;
		virtual int listen_route(SOCKET s,int backlog)=0;
		virtual int select_route(int nfds,fd_set *readfds,fd_set *writefds,fd_set *exceptfds,const timeval *timeout)=0;
		virtual int recvfrom_route(SOCKET s,char *buf,int len,int flags,sockaddr* addr,int *addrlen)=0;
		virtual int recv_route(SOCKET s,char *buf,int len,int flags)=0;
		virtual int sendto_route(SOCKET s,const char *buf,int len,int flags,const sockaddr* to,int tolen)=0;
		virtual int send_route(SOCKET s,const char *buf,int len,int flags)=0;
		virtual int closesocket_route(SOCKET s)=0;
	};
	class crtsocketlayerbase: public icrtsocketlayer{
	public:
		virtual string getname(){return "baselayer";}
		virtual SOCKET socket_route(int ad,int type,int protocol){return ::socket(ad,type,protocol);}
		virtual int connect_route(SOCKET s,const sockaddr* addr,int addrsize){return ::connect(s,addr,addrsize);}
		virtual SOCKET accept_route(SOCKET s,sockaddr* addr,int *addrlen){return ::accept(s,addr,addrlen);}
		virtual int bind_route(SOCKET s,const sockaddr *addr, int addrlen){return ::bind(s,addr,addrlen);}
		virtual int listen_route(SOCKET s,int backlog){return ::listen(s,backlog);}
		virtual int select_route(int nfds,fd_set *readfds,fd_set *writefds,fd_set *exceptfds,const timeval *timeout)
			{return ::select(nfds,readfds,writefds,exceptfds,timeout);}
		virtual int recvfrom_route(SOCKET s,char *buf,int len,int flags,sockaddr* addr,int *addrlen)
			{return ::recvfrom(s,buf,len,flags,addr,addrlen);}
		virtual int recv_route(SOCKET s,char *buf,int len,int flags){return ::recv(s,buf,len,flags);}
		virtual int sendto_route(SOCKET s,const char *buf,int len,int flags,const sockaddr* to,int tolen){
			crtdebugln("send to %s:%u len:%u",crtfun::inet_ntoa_safe_u32n(((const sockaddr_in *)to)->sin_addr.s_addr).c_str()
				,ntohs(((const sockaddr_in *)to)->sin_port),len);
			return ::sendto(s,buf,len,flags,to,tolen);
		}
		virtual int send_route(SOCKET s,const char *buf,int len,int flags){return ::send(s,buf,len,flags);}
		virtual int closesocket_route(SOCKET s){return ::closesocket(s);}
	};
	class icrtsocketevent{
		public:
		/*
		* param model socket model pointer
		* param fd socket's fd
		* param state fd's private array to save it's state
		* return true if accept,false if want to close it
		*/
		virtual bool onconnected(crtsocketmodel *model,SOCKET fd,crtobjmap<int> *state){return true;}
		/*
		* param model socket model pointer
		* param fd socket's fd
		* param state fd's private array to save it's state
		* return true if accept,false if want to close it
		*/
		virtual bool onaccepted(crtsocketmodel *model,SOCKET svr,SOCKET fd,crtobjmap<int> *state){return true;}
		/*
		* param model socket model pointer
		* param fd socket's fd
		* param state fd's private array to save it's state
		* return true if accept,false if want to close it
		*/
		virtual bool oncansend(crtsocketmodel *model,SOCKET fd,crtobjmap<int> *state){return true;}
		/*
		* param model socket model pointer
		* param fd socket's fd
		* param sended buffer sended
		* param bufferflag buffer's flag
		* param peer peer's socketinfo,only use in udp,tcp is null
		* param state fd's private array to save it's state
		* return true if accept,false if want to close it
		*/
		virtual bool onsended(crtsocketmodel *model,SOCKET fd,crtblockbuffer *sended,uint32_t bufferflag
			,sockaddr_in *peer,crtobjmap<int> *state){return true;}
		/*
		* param model socket model pointer
		* param fd socket's fd
		* param buf buffer received
		* param ret return the buffer chain remain not parse
		* param peer peer's socketinfo,only use in udp,tcp is null
		* param state fd's private array to save it's state
		* return -1 for close it,
		*  0 for continue check for next time,
		*  >0 for parsed sth
		* usually do things below:
		*  1.used crtblockbuffer in buf chain call model->releasebuf to release it
		*  2.not parsed crtblockbuffer chain must be set to **ret for system update the recvbuf chain
		*  3.return 1 if change the buf chain
		*/
		virtual int onreceive(crtsocketmodel *model,SOCKET fd,crtblockbuffer *buf,crtblockbuffer **ret
			,sockaddr_in *peer,crtobjmap<int> *state)=0;
		/*{
			model->releasebuf(buf);
			*ret=NULL;
			return 1;
		}*/
		/*
		* param model socket model pointer
		* param fd socket's fd
		* param state fd's private array to save it's state
		* no return
		*/
		virtual void onclose(crtsocketmodel *model,SOCKET fd,crtobjmap<int> *state){}
	};
	#define socketevent_read	0x1
	#define socketevent_write	0x2
	#define socketevent_error	0x4
	class icrtsocketmonitor{
	public:
		icrtsocketmonitor(){m_layer=NULL;}
		virtual ~icrtsocketmonitor(){}
		virtual void setsocketlayer(icrtsocketlayer *layer){m_layer=layer;}
		virtual bool add(SOCKET fd,int mode=socketevent_read|socketevent_write|socketevent_error){
			if (m_fds.find(fd)!=m_fds.end()) return false;
			m_fds.insert(fd);
			return true;
		}
		virtual bool del(SOCKET fd){
			set<SOCKET>::iterator it=m_fds.find(fd);
			if (it==m_fds.end()) return false;
			m_fds.erase(it);
			return true;
		}
		virtual bool mod(SOCKET fd,int mode=socketevent_read|socketevent_write|socketevent_error)=0;
		virtual set<SOCKET> *allfd(){return &m_fds;}
		//-1 for error,0 for no event,>0 for event
		virtual int select(unsigned int timeoutms)=0;
		virtual SOCKET fd(int n)=0;
		virtual bool issend(int n)=0;
		virtual bool isrecv(int n)=0;
		virtual bool iserr(int n)=0;
		virtual size_t size(){return m_fds.size();}
	protected:
		set<SOCKET> m_fds;
		icrtsocketlayer *m_layer;
	};
	class crtsocketmonitorselect : public icrtsocketmonitor{
	public:
		crtsocketmonitorselect(){
			FD_ZERO(&orgset);
			FD_ZERO(&sorgset);
			FD_ZERO(&rorgset);
			FD_ZERO(&eorgset);
			max=0;
		#ifndef _WIN32
			memset(sets,0,sizeof(sets));
		#endif
		}
		virtual ~crtsocketmonitorselect(){}
		virtual bool add(SOCKET fd,int mode=socketevent_read|socketevent_write|socketevent_error){
			if (!icrtsocketmonitor::add(fd)) return false;
			FD_SET(fd,&orgset);
			if (mode&socketevent_read) FD_SET(fd,&rorgset);
			if (mode&socketevent_write) FD_SET(fd,&sorgset);
			if (mode&socketevent_error) FD_SET(fd,&eorgset);
		#ifndef _WIN32
			if (fd>max) max=fd;
		#endif
			return true;
		}
		virtual bool del(SOCKET fd){
			if (!icrtsocketmonitor::del(fd)) return false;
			FD_CLR(fd,&orgset);
			FD_CLR(fd,&rorgset);
			FD_CLR(fd,&sorgset);
			FD_CLR(fd,&eorgset);
		#ifndef _WIN32
			if (m_fds.size()==0) max=0;
			else max=*m_fds.rbegin();
		#endif
			return true;
		}
		virtual bool mod(SOCKET fd,int mode=socketevent_read|socketevent_write|socketevent_error){
			if (!FD_ISSET(fd,&orgset)) return false;
			if (mode&socketevent_read) FD_SET(fd,&rorgset); else FD_CLR(fd,&rorgset);
			if (mode&socketevent_write) FD_SET(fd,&sorgset); else FD_CLR(fd,&sorgset);
			if (mode&socketevent_error) FD_SET(fd,&eorgset); else FD_CLR(fd,&eorgset);
			return true;
		}
#ifdef _WIN32
#define FD_COPY(set,orgset)	{set.fd_count=orgset.fd_count;memcpy(set.fd_array,orgset.fd_array,orgset.fd_count*sizeof(SOCKET));}
#else
#define FD_COPY(set,orgset) memcpy(&set,&orgset,sizeof(fd_set))
#endif
		virtual int select(unsigned int timeoutms){
			FD_COPY(rset,rorgset);
			FD_COPY(sset,sorgset);
			FD_COPY(eset,eorgset);
			timeval ti;
			if (timeoutms==0) {ti.tv_sec=0;ti.tv_usec=0;}
			else {
				ti.tv_sec=timeoutms/1000;
				ti.tv_usec=timeoutms*1000;
			}
			int ret=m_layer->select_route(max+1,&rset,&sset,&eset,&ti);
		#ifdef _WIN32
			//����win32fd�����飬��setsΪ��set������������set
			if (ret>0) {
				FD_COPY(set,rset);
				memcpy(&set,&rset,sizeof(fd_set));
				unsigned int i;
				for(i=0;i<sset.fd_count;i++) {
					if (!FD_ISSET(sset.fd_array[i],&set)) FD_SET(sset.fd_array[i],&set);
				}
				for(i=0;i<eset.fd_count;i++) {
					if (!FD_ISSET(eset.fd_array[i],&set)) FD_SET(eset.fd_array[i],&set);
				}
				ret=set.fd_count;
			}
		#else
			if (ret>0){
				ret=0;
				set<SOCKET>::iterator it=m_fds.begin();
				SOCKET fd;
				while(it!=m_fds.end()) {
					fd=*it;
					if (FD_ISSET(fd,rset) || FD_ISSET(fd,sset) || FD_ISSET(fd,eset)) sets[ret++]=fd;
					it++;
				}
			}
		#endif
			return ret;
		}
		virtual SOCKET fd(int n){
		#ifdef _WIN32
			return set.fd_array[n];
		#else
			return sets[n];
		#endif
		}
		virtual bool issend(int n){
		#ifdef _WIN32
			return FD_ISSET(set.fd_array[n],&sset)!=FALSE;
		#else
			return FD_ISSET(sets[n],&sset)!=FALSE;
		#endif
		}
		virtual bool isrecv(int n){
		#ifdef _WIN32
			return FD_ISSET(set.fd_array[n],&rset)!=FALSE;
		#else
			return FD_ISSET(sets[n],&rset)!=FALSE;
		#endif
		}
		virtual bool iserr(int n){
		#ifdef _WIN32
			return FD_ISSET(set.fd_array[n],&eset)!=FALSE;
		#else
			return FD_ISSET(sets[n],&eset)!=FALSE;
		#endif
		}
	protected:
	#ifndef _WIN32
		SOCKET sets[FD_SETSIZE];
	#endif
		int max;
		fd_set orgset;//ÿ��selectʱ��orgset���ƣ����fd�ź�ʱ��ʹ��set����ʱ�������add��del��Ҳ����Ӱ�����
		fd_set rorgset;
		fd_set sorgset;
		fd_set eorgset;
		fd_set set;
		fd_set rset;
		fd_set sset;
		fd_set eset;
	};
#if !defined(_WIN32) && !defined(__APPLE__)
	//fixme how to support icrtsocketlayer in epoll ?
	class crtsocketmonitorepoll : public icrtsocketmonitor{
		#define MAX_EPOLL	128
	public:
		crtsocketmonitorepoll(){
			epfd=epoll_create(MAX_EPOLL);
			if (epfd!=-1) {
				pevent=new epoll_event[MAX_EPOLL];
				memset(pevent , 0, sizeof(struct epoll_event)*MAX_EPOLL);
			}
			else
				pevent=NULL;
		}
		virtual ~crtsocketmonitorepoll(){
			if (epfd!=-1) {
				delete [] pevent;
				close(epfd);
			}
		}
		virtual bool add(SOCKET fd, int mode=socketevent_read|socketevent_write|socketevent_error){
			if (!icrtsocketmonitor::add(fd)) return false;
			struct epoll_event ep;
			memset(&ep, 0, sizeof(ep));
			if (mode&socketevent_read) ep.events|=EPOLLIN;
			if (mode&socketevent_write) ep.events|=EPOOLOUT;
			if (mode&socketevent_error) ep.events|=EPOLLERR|EPOLLHUP;
			ep.data.fd = fd;
			return epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ep)!=-1;
		}
		virtual bool del(SOCKET fd){
			if (!icrtsocketmonitor::del(fd)) return false;
			struct epoll_event ep;
			// In kernel versions before 2.6.9, the EPOLL_CTL_DEL operation required a non-NULL pointer in event
			return epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ep)!=-1;
		}
		virtual bool mod(SOCKET fd, int mode=socketevent_read|socketevent_write|socketevent_error){
			struct epoll_event ep;
			memset(&ep, 0, sizeof(ep));
			if (mode&socketevent_read) ep.events|=EPOLLIN;
			if (mode&socketevent_write) ep.events|=EPOOLOUT;
			if (mode&socketevent_error) ep.events|=EPOLLERR|EPOLLHUP;
			ep.data.fd = fd;
			return epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ep)!=-1;
		}
		virtual int select(unsigned int timeoutms){
			return epoll_wait(epfd, pevent, MAX_EPOLL, timeout_ms);
		}
		virtual SOCKET fd(int n){
			if (MAX_EPOLL <= n) return INVALID_SOCKET;
			return pevent[n].data.fd;
		}
		virtual bool issend(int n){
			if (MAX_EPOLL <= n) return false;
			return (pevent[n].events&EPOLLOUT)!=0;
		}
		virtual bool isrecv(int n){
			if (MAX_EPOLL <= n) return false;
			return (pevent[n].events&EPOLLIN)!=0;
		}
		virtual bool iserr(int n){
			if (MAX_EPOLL <= n) return false;
			return (pevent[n].events&(EPOLLERR|EPOLLHUP))!=0;
		}
	protected:
		int epfd;
		struct epoll_event *pevent;
	};
#endif
#if defined(_WIN32) && !defined(__APPLE__)
	#define crtsocketmonitorrecommend crtsocketmonitorselect
#else
	#define crtsocketmonitorrecommend crtsocketmonitorepoll
#endif
	#define crtsocketbufferflag_udp			0x1
	#define crtsocketbufferflag_udpretry	0x2
	class crtsocketmodel{
		#define objectmap_lastti			1
		#define objectmap_host				2
		#define objectmap_port				3
		#define objectmap_sendbuf			4
		#define objectmap_recvbuf			5
		#define objectmap_portn				6
		#define objectmap_userdata			32
	public:
		crtsocketmodel(){
			serverpool = new crtsocketmonitorrecommend();
			connectpool = new crtsocketmonitorrecommend();
			selectpool = new crtsocketmonitorrecommend();
			udpselectpool = new crtsocketmonitorrecommend();
			sendpool = new crtsocketmonitorrecommend();
			setup(NULL);
			lastti = 0;
		}
		virtual ~crtsocketmodel(){
			eventcb=NULL;
			clear();
			delete serverpool;
			delete connectpool;
			delete selectpool;
			delete udpselectpool;
			delete sendpool;
		}
		virtual void clear(){
			opermtx.lock();
			_clear(serverpool);
			_clear(connectpool);
			_clear(selectpool);
			_clear(udpselectpool);
			_clear(sendpool);
			opermtx.unlock();
		}
		/*
		* setup
		* timeout, 0 for unlimit
		* speed limited in B,-1 for unlimit
		* timesinsec will be call function pump() how many times pre sec
		*    more times for improve response's performance and speed limit more smooth
		*/
		virtual void setup(icrtsocketevent *cb,icrtsocketlayer *layer=NULL,unsigned int contout=60,unsigned int idletout=60,
			unsigned int sendspeed=(unsigned int)-1,unsigned int recvspeed=(unsigned int)-1,
			unsigned int maxpacket=65536,
			unsigned int timesinsec=25){
			socketlayer=layer?layer:&baselayer;
			serverpool->setsocketlayer(socketlayer);
			connectpool->setsocketlayer(socketlayer);
			selectpool->setsocketlayer(socketlayer);
			udpselectpool->setsocketlayer(socketlayer);
			sendpool->setsocketlayer(socketlayer);
			eventcb=cb;
			connecttimeout=contout;
			idletimeout=idletout;
			limitsendspeed=sendspeed/timesinsec;
			limitrecvspeed=recvspeed/timesinsec;
			maxpacketlen=maxpacket;
		}
		/*
		* host connect host, e.g. "127.0.0.1:80"
		* return socket's fd
		*/
		virtual SOCKET connect_socket(string host) {
			uint16_t port;
			uint32_t ip=get_ip_port_by_name(host.c_str(), &port);
			return connect_socket(ip,port);
		}
		//ip network order,port local order
		virtual SOCKET connect_socket(uint32_t ip,uint16_t port) {
			//fixme setsockopt fnctl function will not support isocketlayer
			SOCKET s = socketlayer->socket_route(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (s == INVALID_SOCKET) return INVALID_SOCKET;
			if (set_non_block(s)!=0) {
				socketlayer->closesocket_route(s);
				return INVALID_SOCKET;
			}
			sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = ip;
			addr.sin_port = htons(port);
			if (socketlayer->connect_route(s, (sockaddr *) &addr, sizeof(sockaddr_in)) != 0) {
				if (!IsWouldBlock(GetLastError())) {
					socketlayer->closesocket_route(s);
					return INVALID_SOCKET;
				}
			}
			opermtx.lock();
			fdstate.setuint(s,objectmap_lastti,(unsigned int)time(NULL));
			fdstate.setuint(s,objectmap_host,ip);
			fdstate.setuint(s,objectmap_port,port);
			fdstate.setuint(s,objectmap_portn,htons(port));
			connectpool->add(s,socketevent_read|socketevent_write|socketevent_error);
			opermtx.unlock();
			crtdebugln("[socketmodel]connect socket %u %s",s,print_host(ip,port).c_str());
			return s;
		}
		/*
		* ip network order,port local order
		* return server socket's fd
		*/
		virtual SOCKET start_listen(uint16_t port,uint32_t host=0,bool udp=false,int maxconn=SOMAXCONN)
		{
			//fixme setsockopt fnctl function will not support isocketlayer
			SOCKET s = socketlayer->socket_route(AF_INET, udp?SOCK_DGRAM:SOCK_STREAM, udp?IPPROTO_UDP:IPPROTO_TCP);
			if (s == INVALID_SOCKET) return INVALID_SOCKET;
			sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = host;
			addr.sin_port = htons(port);
			if (socketlayer->bind_route(s, (sockaddr *) &addr, sizeof(addr)) != 0) {
				socketlayer->closesocket_route(s);
				return INVALID_SOCKET;
			}
			int on = 1;
			if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on))!=0) {
				socketlayer->closesocket_route(s);
				return INVALID_SOCKET;
			}
			if (udp) set_udp_socket_autoclose_behavior(s);
			if (s==INVALID_SOCKET) return s;
			if (set_non_block(s)!=0) {
				socketlayer->closesocket_route(s);
				return INVALID_SOCKET;
			}
			opermtx.lock();
			if (!udp){
				if (socketlayer->listen_route(s, maxconn)!=0) {
					socketlayer->closesocket_route(s);
					return INVALID_SOCKET;
				}
				serverpool->add(s,socketevent_read|socketevent_error);
			} else
				udpselectpool->add(s,socketevent_read|socketevent_error);
			fdstate.setuint(s,objectmap_host,host);
			fdstate.setuint(s,objectmap_port,port);
			fdstate.setuint(s,objectmap_portn,htons(port));
			opermtx.unlock();
			crtdebugln("[socketmodel]listen socket %u %s",s,print_host(host,port).c_str());
			return s;
		}
		virtual void stop_listen(SOCKET fd){free_socket(fd);}
		virtual crtblockbuffer *alloctcpbuf(unsigned int size){
			opermtx.lock();
			crtblockbuffer *buf=bufferman.newbuf(size);
			opermtx.unlock();
			ASSERT(buf->flag==0);
			return buf;
		}
		virtual crtblockbuffer *allocudpbuf(unsigned int size,const sockaddr_in *addr){
			opermtx.lock();
			crtblockbuffer *buf=bufferman.newbuf(size+sizeof(sockaddr_in));
			opermtx.unlock();
			if (buf) {
				//udp packet
				buf->flag=crtsocketbufferflag_udp;
				if (addr) memcpy(buf->buf,addr,sizeof(sockaddr_in));
				//else memset(buf->buf,0,sizeof(sockaddr_in));
				buf->buf+=sizeof(sockaddr_in);
				//buf->alloc-=sizeof(sockaddr_in);
			}
			return buf;
		}
		virtual crtblockbuffer *allocudpbufbyip(unsigned int size,uint32_t ip,uint16_t port) {
			sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = ip;
			addr.sin_port = htons(port);
			return allocudpbuf(size,&addr);
		}
		virtual bool updateudpbufferipbyip(crtblockbuffer *buf,uint32_t ip,uint16_t port) {
			sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = ip;
			addr.sin_port = htons(port);
			return updateudpbufferip(buf,&addr);
		}
		virtual bool updateudpbufferip(crtblockbuffer *buf,const sockaddr_in *addr) {
			if (buf && addr) {
				//udp packet
				if (buf->flag & crtsocketbufferflag_udp) {
					memcpy(buf->buf-sizeof(sockaddr_in),addr,sizeof(sockaddr_in));
					return true;
				}
			}
			return false;
		}
		virtual void releasebuf(crtblockbuffer *buf,bool checkref=false) {
			if (!buf) return;
			opermtx.lock();
			crtblockbuffer *next;
			while(buf) {
				next=buf->next;
				if (checkref && (buf->flag & crtsocketbufferflag_udpretry)) {
					//crtdebugln("skip resend check buffer %x",buf);
				} else {
					buf->next=NULL;
					//not need to parse now, buffer class auto fix the buf pointer in reuse
					/*if (buf->flag & crtsocketbufferflag_udp) {
						//udp packet
						//hack for any buffer between buf and buf->buf
						uint8_t *orgbuf=(uint8_t *)(buf+1);
						if (buf->buf>orgbuf) {
							buf->alloc+=(unsigned int)(buf->buf-orgbuf);
							buf->buf=orgbuf;
							buf->adddata=NULL;
						}
					}*/
					bufferman.releasebuf(buf);
				}
				buf=next;
			}
			opermtx.unlock();
		}
		//send is asyn request,you can check sended in callback OnSended
		//buf����ϵͳ�ӹܣ�������Ϻ������Ƿ���ȷ�������ù��� 
		virtual bool send(SOCKET fd,crtblockbuffer *buf)
		{
			if (!buf) return false;
			opermtx.lock();
			if (!fdstate.ishave(fd)) {
				releasebuf(buf);
				opermtx.unlock();
				return false;
			}
			crtblockbuffer *b=(crtblockbuffer *)fdstate.getpointer(fd,objectmap_sendbuf);
			if (b) b->appendbuffer(buf);
			else b=buf;
			fdstate.setpointer(fd,objectmap_sendbuf,b);
			sendpool->add(fd,socketevent_write|socketevent_error);
			opermtx.unlock();
			return true;
		}
		virtual void free_socket(SOCKET fd)
		{
			crtdebugln("[socketmodel]free socket %u",fd);
			int err=GetLastError();
			opermtx.lock();
			crtobjmap<int> *obj=fdstate.findmap(fd);
			if (obj) {
				if (eventcb)
					eventcb->onclose(this,fd,obj);
				serverpool->del(fd);
				connectpool->del(fd);
				selectpool->del(fd);
				udpselectpool->del(fd);
				sendpool->del(fd);
				//fixme �и��������resend����ĳ��buf��Ȼ��ռӵ�sendqueue������ʧ��freesocketʱ��ֱ�ӾͰ�sendqueue��buf�ͷ��ˣ�resend�ٷ��ͻ�������
				releasebuf((crtblockbuffer *)fdstate.getpointer(fd,objectmap_sendbuf),true);
				releasebuf((crtblockbuffer *)fdstate.getpointer(fd,objectmap_recvbuf));
				fdstate.remove(fd);
			}
			opermtx.unlock();
			socketlayer->closesocket_route(fd);
		}
		void setfdstateuint(SOCKET s,int key,unsigned int value) {opermtx.lock();fdstate.setuint(s,key,value);opermtx.unlock();}
		unsigned int getfdstateuint(SOCKET s,int key,unsigned int defvalue=0){
			opermtx.lock();
			defvalue=fdstate.getuint(s,key,defvalue);
			opermtx.unlock();
			return defvalue;
		}

		/*
		* while call this function to do socket operation
		*/
		virtual void pump()
		{
			crtsinglelock l(&opermtx);
			unsigned int curti=(unsigned int)time(NULL);
			if (curti!=lastti){
				lastti=curti;
				//check timeout
				if (connecttimeout!=0) {
					set<SOCKET>*fds=connectpool->allfd();
					set<SOCKET>::iterator it=fds->begin();
					while(it!=fds->end()) {
						//fixme ɾ���Ƿ������set����
						if (lastti - connecttimeout > fdstate.getuint(*it,objectmap_lastti)) {
							crtdebugln("[socketmodel]conn timeout socket %u",*it);
							free_socket(*it++);
						} else it++;
					}
				}
				if (idletimeout!=0) {
					set<SOCKET> *fds=selectpool->allfd();
					set<SOCKET>::iterator it=fds->begin();
					while(it!=fds->end()) {
						if (lastti - idletimeout > fdstate.getuint(*it,objectmap_lastti)) {
							crtdebugln("[socketmodel]idle timeout socket %u",*it);
							free_socket(*it++);
						} else it++;
					}
				}
				currentsend=limitsendspeed;
				currentrecv=limitrecvspeed;
			}
			//server accept check
			if (serverpool->size()>0) {
				int event=serverpool->select(0);
				if (event>0) {
					for(int i=0;i<event;i++) {
						if (serverpool->iserr(i))
							free_socket(serverpool->fd(i));
						else if (serverpool->isrecv(i)){
							struct sockaddr_in addr;
							socklen_t addrlen;
							addrlen=sizeof(struct sockaddr_in);
							SOCKET tmpfd=socketlayer->accept_route(serverpool->fd(i),(struct sockaddr *)&addr,&addrlen);
							if (tmpfd!=INVALID_SOCKET) {
								fdstate.remove(tmpfd);
								fdstate.setuint(tmpfd,objectmap_lastti,lastti);
								fdstate.setuint(tmpfd,objectmap_host,addr.sin_addr.s_addr);
								fdstate.setuint(tmpfd,objectmap_port,ntohs(addr.sin_port));
								fdstate.setuint(tmpfd,objectmap_portn,addr.sin_port);
								selectpool->add(tmpfd,socketevent_read|socketevent_error);
								crtdebugln("[socketmodel]accept socket %u %u",serverpool->fd(i),tmpfd);
								if (eventcb && !eventcb->onaccepted(this,serverpool->fd(i),tmpfd,fdstate.findmap(tmpfd)))
									free_socket(tmpfd);
							}
						}
					}
				}
			}
			//connecting socket check
			SOCKET fd;
			if (connectpool->size()>0) {
				int event=connectpool->select(0);
				if (event>0) {
					for(int i=0;i<event;i++) {
						if (connectpool->iserr(i))
							free_socket(connectpool->fd(i));
						else if (connectpool->issend(i)){
							//connection refused if w and r signal together
							if (connectpool->isrecv(i))
								free_socket(connectpool->fd(i));
							else {
								fd=connectpool->fd(i);
								if (eventcb && !eventcb->onconnected(this,fd,fdstate.findmap(fd))) {
									free_socket(fd);
									continue;
								}
								connectpool->del(fd);
								selectpool->add(fd);
								fdstate.setuint(fd,objectmap_lastti,lastti);
							}
						}
					}
				}
			}
			//normal socket read
			if (selectpool->size()>0) {
				int event=selectpool->select(0);
				if (event>0) {
					for(int i=0;i<event;i++) {
						if (selectpool->iserr(i))
							free_socket(selectpool->fd(i));
						else {
							fd=selectpool->fd(i);
							if (selectpool->isrecv(i)){
								crtblockbuffer *buf=alloctcpbuf(maxpacketlen);
								int ret=socketlayer->recv_route(fd,(char *)buf->buf,maxpacketlen,0);
								if (ret<=0) {
									free_socket(fd);
									releasebuf(buf);
								} else {
									buf->len=ret;
									fdstate.setuint(fd,objectmap_lastti,lastti);
									crtblockbuffer *r=(crtblockbuffer *)fdstate.getpointer(fd,objectmap_recvbuf);
									crtblockbuffer *tmp;
									if (r) {
										//already have buf
										if (r->appendorcombinbuffer(buf)) releasebuf(buf);
										buf=r;
									}
									bool bfreed=false;
									if (eventcb) {
										while(buf) {
											tmp=buf;
											int rret=eventcb->onreceive(this,fd,buf,&tmp,NULL,fdstate.findmap(fd));
											if (rret<0) {
												fdstate.setpointer(fd,objectmap_recvbuf,tmp);
												free_socket(fd);
												bfreed=true;
												break;
											} else if (rret==0) break;
											buf=tmp;
										}
									}
									//bufָ���ѿ�����Щ���������onreceive����0���������޷�������û��eventcb�����ݶ��Ѵ���
									if (!bfreed) fdstate.setpointer(fd,objectmap_recvbuf,buf);
									//speed limit
									//fixme, the select event will still set in next time call?
									if (currentrecv<(unsigned int)ret) break;
									currentrecv-=ret;
								}
							}
						}
					}
				}
			}
			//normal udp socket read
			if (udpselectpool->size()>0) {
				int event=udpselectpool->select(0);
				if (event>0) {
					for(int i=0;i<event;i++) {
						if (udpselectpool->iserr(i))
							free_socket(udpselectpool->fd(i));
						else {
							fd=udpselectpool->fd(i);
							if (udpselectpool->isrecv(i)){
								crtblockbuffer *buf=allocudpbuf(maxpacketlen,NULL);
								int fromlen=sizeof(sockaddr_in);
								int ret=socketlayer->recvfrom_route(fd,(char *)buf->buf,maxpacketlen,0,
									(sockaddr *)(buf->buf-sizeof(sockaddr_in)),&fromlen);
								if (ret<=0) {
									free_socket(fd);
									releasebuf(buf);
								} else {
									buf->len=ret;
									fdstate.setuint(fd,objectmap_lastti,lastti);
									crtblockbuffer *r=(crtblockbuffer *)fdstate.getpointer(fd,objectmap_recvbuf);
									crtblockbuffer *tmp;
									if (r) {
										//already have buf
										r->appendbuffer(buf);
										buf=r;
									}
									bool bfreed=false;
									if (eventcb) {
										while(buf) {
											tmp=buf;
											int rret=eventcb->onreceive(this,fd,buf,&tmp
												,(sockaddr_in *)(buf->buf-sizeof(sockaddr_in)),fdstate.findmap(fd));
											if (rret<0) {
												fdstate.setpointer(fd,objectmap_recvbuf,tmp);
												free_socket(fd);
												bfreed=true;
												break;
											} else if (rret==0) break;
											buf=tmp;
										}
									}
									//bufָ���ѿ�����Щ���������onreceive����0���������޷�������û��eventcb�����ݶ��Ѵ���
									if (!bfreed) fdstate.setpointer(fd,objectmap_recvbuf,buf);
									//speed limit
									//fixme, the select event will still set in next time call?
									if (currentrecv<(unsigned int)ret) break;
									currentrecv-=ret;
								}
							}
						}
					}
				}
			}
			//send pool check
			if (sendpool->size()>0) {
				int event=sendpool->select(0);
				if (event>0) {
					for(int i=0;i<event;i++) {
						if (sendpool->iserr(i))
							free_socket(sendpool->fd(i));
						else if (sendpool->issend(i)){
							fd=sendpool->fd(i);
							crtblockbuffer *s=(crtblockbuffer *)fdstate.getpointer(fd,objectmap_sendbuf);
							crtblockbuffer *tmp=s;
							int ret;
							unsigned int flag;
							while(1) {
								tmp=s->next;
								flag=s->flag;
								if (flag & crtsocketbufferflag_udp) {
									ret=socketlayer->sendto_route(fd,(char *)s->buf,s->len,0,
										(sockaddr *)(s->buf-sizeof(sockaddr_in)),sizeof(sockaddr_in));
								} else
									ret=socketlayer->send_route(fd,(char *)s->buf,s->len,0);
								if (ret==0 || (ret<0 && !IsWouldBlock(GetLastError()))){
									free_socket(fd);
									break;
								} else if (ret<0) {
									//socket blocked
									fdstate.setpointer(fd,objectmap_sendbuf,s);
									break;
								}
								s->next=NULL;
								if (eventcb && !eventcb->onsended(this,fd,s,flag
										,(sockaddr_in *)((flag&crtsocketbufferflag_udp)?NULL:(s->buf-sizeof(sockaddr_in)))
										,fdstate.findmap(fd))) free_socket(fd);
								//hack for resend model
								if (!(flag&crtsocketbufferflag_udpretry)) releasebuf(s);
								if (currentsend>=(unsigned int)ret) currentsend-=ret;
								if (!tmp) {
									//all sended
									sendpool->del(fd);
									fdstate.setpointer(fd,objectmap_sendbuf,NULL);
									if (eventcb && !eventcb->oncansend(this,fd,fdstate.findmap(fd))) free_socket(fd);
									break;
								}
								//speed limit
								//fixme, the select event will still set in next time call?
								if (currentsend<(unsigned int)ret) {
									fdstate.setpointer(fd,objectmap_sendbuf,tmp);
									break;
								}
								s=tmp;
							}
							//speed limit double check if send limited
							if (currentsend<(unsigned int)ret) break;
						}
					}
				}
			}
		}
	protected:
		icrtsocketevent *eventcb;			///<socket event handler
		crtsocketlayerbase baselayer;		///<base layer for direct use
		icrtsocketlayer *socketlayer;		///<socket layer
		icrtsocketmonitor *serverpool;		///<server fd for listen accept,value is fd
		icrtsocketmonitor *connectpool;		///<connect fd for connecting socket,value is fd
		icrtsocketmonitor *selectpool;		///<client fd for read event,value is fd,only tcp socket
		icrtsocketmonitor *udpselectpool;	///<client fd for read event,value is fd,only udp socket
		icrtsocketmonitor *sendpool;		///<client fd for write event,value is fd,tcp udp compatible
		crtobjmapmanager<SOCKET,int> fdstate;
											///<fd's state
		unsigned int connecttimeout;		///<connect timeout, 0 for unlimit
		unsigned int idletimeout;			///<idle timeout, 0 for unlimit
		unsigned int limitsendspeed;		///<send speed limited in B,-1 for unlimit
		unsigned int limitrecvspeed;		///<recv speed limited in B,-1 for unlimit
		unsigned int lastti;				///<last check timeout ts
		unsigned int currentsend;			///<current bytes can send in sec
		unsigned int currentrecv;			///<current bytes can recv in sec
		unsigned int maxpacketlen;			///<max packet buffer use by recv
		crtblockbuffermanager bufferman;	///<send and receive buffer manager
		crtmutex opermtx;					///<send,alloc opermtx
		void _clear(icrtsocketmonitor *monitor){
			set<SOCKET> *fds=monitor->allfd();
			set<SOCKET>::iterator it=fds->begin();
			while(it!=fds->end())
				free_socket(*it++);
		}
	};
	class crtsocketresendmodel;
	class icrtsocketretryevent{
	public:
		//return false to cancel the retry
		virtual bool onresendtry(crtsocketresendmodel *cls,SOCKET s,crtblockbuffer *buffer,uint64_t key){return true;}
		//if buffer is NULL, send have error
		virtual void onresendfailed(crtsocketresendmodel *cls,SOCKET s,crtblockbuffer *buffer,uint64_t key){}
		virtual void onretrynotify(crtsocketresendmodel *cls,uint32_t key,void *adddata){};
	};
	class crtsocketresendmodel : public crtsocketmodel{
	public:
		crtsocketresendmodel(){
			m_notify=NULL;
		}
		virtual ~crtsocketresendmodel(){
			removeallseq();
		}
		virtual void setup(icrtsocketretryevent *notify,icrtsocketevent *cb,icrtsocketlayer *layer=NULL,
			unsigned int contout=60,unsigned int idletout=60,
			unsigned int sendspeed=(unsigned int)-1,unsigned int recvspeed=(unsigned int)-1,
			unsigned int maxpacket=65536,
			unsigned int timesinsec=25){
			m_notify=notify;
			crtsocketmodel::setup(cb,layer,contout,idletout,sendspeed,recvspeed,maxpacket,timesinsec);
		}
		virtual bool send(SOCKET fd,crtblockbuffer *buf,uint32_t sendtimes=1,
			uint16_t seq=0,uint32_t delayms=2000,uint32_t delayinc=1000){
			if (sendtimes>1) {
				sockaddr_in *in=(sockaddr_in *)(buf->buf-sizeof(sockaddr_in));
				crtdebugln("addseq %s seq:%x len:%u",print_host(in).c_str(),seq,buf->len);
				uint64_t key=crtmakeddword(in->sin_addr.s_addr,crtmakedword(in->sin_port,seq));
				uint32_t msti=get_ms_tick()+delayms;
				m_mtx.lock();
				while(1) {
					if (m_resendqueue.find(msti)==m_resendqueue.end()) break;
					msti++;
				}
				crtsocketresend *send=m_sendobj.new_item();
				send->s=fd;
				send->key=key;
				send->times=sendtimes;
				send->delay=delayms;
				send->delayinc=delayinc;
				send->buf=buf;
				m_resendqueue.insert(map<uint32_t,crtsocketresend *>::value_type(msti,send));
				pair<map<uint64_t,uint32_t>::iterator,bool> ret=m_resendmap.insert(map<uint64_t,uint32_t>::value_type(key,msti));
				ASSERT(ret.second);
				m_mtx.unlock();
				buf->flag|=crtsocketbufferflag_udpretry;
			} else
				buf->flag&=~crtsocketbufferflag_udpretry;
			return crtsocketmodel::send(fd,buf);
		}
		bool addtimeoutjob(uint32_t timeout,uint32_t key,void *adddata){
			uint64_t ukey=crtmakeddword(0,key);
			uint32_t msti=get_ms_tick()+timeout;
			m_mtx.lock();
			while(1) {
				if (m_resendqueue.find(msti)==m_resendqueue.end()) break;
				msti++;
			}
			crtsocketresend *send=m_sendobj.new_item();
			send->s=INVALID_SOCKET;
			send->key=ukey;
			send->times=1;
			send->delay=timeout;
			send->delayinc=0;
			send->buf=(crtblockbuffer *)adddata;
			m_resendqueue.insert(map<uint32_t,crtsocketresend *>::value_type(msti,send));
			m_resendmap.insert(map<uint64_t,uint32_t>::value_type(ukey,msti));
			m_mtx.unlock();
			return true;
		}
		bool removetimeoutjob(uint32_t key){
			uint64_t ukey=crtmakeddword(0,key);
			return _removeseq(ukey);
		}
		//port is host order
		bool removeseq(uint32_t ip,uint16_t port,uint16_t seq)
			{return _removeseq(crtmakeddword(ip,crtmakedword(htons(port),seq)));}
		bool removeseq(const sockaddr_in *addr,uint16_t seq)
			{return _removeseq(crtmakeddword(addr->sin_addr.s_addr,crtmakedword(addr->sin_port,seq)));}
		//port is host order
		bool removeallseq(uint32_t ip,uint16_t port,uint16_t seq=-1){
			crtdebugln("removeallseq %s seq:%x",print_host(ip,port).c_str(),seq);
			m_mtx.lock();
			uint64_t key=crtmakeddword(ip,crtmakedword(htons(port),0));
			uint64_t seq64=seq & 0xFFFF;
			map<uint64_t,uint32_t>::iterator it=m_resendmap.lower_bound(key);
			while (it!=m_resendmap.end()) {
				uint64_t tkey=it->first;
				if ((tkey & key)!=key || (tkey & 0xFFFF)>seq64) break;
				map<uint32_t,crtsocketresend *>::iterator it2=m_resendqueue.find(it->second);
				if (it2!=m_resendqueue.end()) {
					crtsocketresend *send=it2->second;
					if (send->s!=INVALID_SOCKET && send->buf) releasebuf(send->buf);
					m_sendobj.release_item(send);
					m_resendqueue.erase(it2);
				}
				m_resendmap.erase(it++);
			}
			m_mtx.unlock();
			return true;
		}
		//seq if seq=-1 remove all seq which is the ip and port, seq=seq remove all seq only small and equ to seq
		bool removeallseq(const sockaddr_in *addr,uint16_t seq=-1){return removeallseq(addr->sin_addr.s_addr,ntohs(addr->sin_port),seq);}
		bool removeallseq(){
			m_mtx.lock();
			map<uint32_t,crtsocketresend *>::iterator it=m_resendqueue.begin();
			crtsocketresend *send;
			while (it!=m_resendqueue.end()) {
				send=it->second;
				if (send->s!=INVALID_SOCKET && send->buf) releasebuf(send->buf);
				m_sendobj.release_item(send);
				it++;
			}
			m_resendqueue.clear();
			m_resendmap.clear();
			m_mtx.unlock();
			return true;
		}
		virtual void pump(){
			uint32_t msti=get_ms_tick();
			m_mtx.lock();
			map<uint32_t,crtsocketresend *>::iterator it=m_resendqueue.begin();
			while(it!=m_resendqueue.end()) {
				if (it->first>msti) break;
				crtsocketresend *send=it->second;
				send->times--;
				if (send->times!=0 && m_notify && !m_notify->onresendtry(this,send->s,send->buf,send->key))
					send->times=0;
				if (send->times!=0 && !crtsocketmodel::send(send->s,send->buf)) {
					//send have error, release it
					send->buf=NULL;
					send->times=0;
				}
				if (send->times==0) {
					///release it
					if (m_notify) {
						if (send->s==INVALID_SOCKET)
							m_notify->onretrynotify(this,(uint32_t)send->key,send->buf);
						else
							m_notify->onresendfailed(this,send->s,send->buf,send->key);
					}
					if (send->buf) releasebuf(send->buf);
					m_resendmap.erase(send->key);
					m_sendobj.release_item(it->second);
				} else {
					send->delay+=send->delayinc;
					uint32_t kmsti=msti+send->delay;
					while(1) {
						if (m_resendqueue.find(kmsti)==m_resendqueue.end()) break;
						kmsti++;
					}
					m_resendqueue.insert(map<uint32_t,crtsocketresend *>::value_type(kmsti,send));
					m_resendmap[send->key]=kmsti;
				}
				m_resendqueue.erase(it++);
			}
			m_mtx.unlock();
			crtsocketmodel::pump();
		}
		static uint32_t getipfromkey(uint64_t key){return crthighdword(key);}
		//network order
		static uint16_t getportfromkey(uint64_t key){return ntohs(crthighword(crtlowdword(key)));}
		static uint16_t getseqfromkey(uint64_t key){return crtlowword(key);}
	protected:
		bool _removeseq(uint64_t key) {
			crtdebugln("removeseq %s seq:%x",print_host(getipfromkey(key),getportfromkey(key)).c_str(),getseqfromkey(key));
			m_mtx.lock();
			map<uint64_t,uint32_t>::iterator it=m_resendmap.find(key);
			if (it!=m_resendmap.end()) {
				map<uint32_t,crtsocketresend *>::iterator it2=m_resendqueue.find(it->second);
				m_resendmap.erase(it);
				if (it2!=m_resendqueue.end()) {
					crtsocketresend *send=it2->second;
					if (send->s!=INVALID_SOCKET && send->buf) releasebuf(send->buf);
					m_sendobj.release_item(send);
					m_resendqueue.erase(it2);
					m_mtx.unlock();
					return true;
				}
			}
			m_mtx.unlock();
			return false;
		}
		typedef struct _crtsocketresend{
			SOCKET s;
			uint64_t key;
			uint32_t times;
			uint32_t delay;
			uint32_t delayinc;
			crtblockbuffer *buf;
		}crtsocketresend;
		icrtsocketretryevent *m_notify;
		crtmemmanager<crtsocketresend> m_sendobj;
		map<uint32_t,crtsocketresend *> m_resendqueue;
		map<uint64_t,uint32_t> m_resendmap;
		crtmutex m_mtx;
	};
};
