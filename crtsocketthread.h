#pragma once

#include "crtthread.h"
#include "crtbuffer.h"

/**
* @brief 网络线程类
* @author Barry(barrytan@21cn.com,QQ:20962493)
*/
/**<pre>
  使用Sample：
</pre>*/

namespace crtfun{
	class crtsocketthread;
	class icrtsocketnotify
	{
	public:
		virtual void onreceive(SOCKET s,void *addarg,crtmembuf *buf,crtsocketthread *p){}
		virtual void onerror(SOCKET s,void *addarg,int errcode,crtsocketthread *p){}
	};
	class crtsocketthread:public crtthread,public icrtsocketnotify
	{
	public:
		crtsocketthread(){m_p=this;}
		virtual ~crtsocketthread(){}
		void setnotify(icrtsocketnotify *p){m_p=p;}
		//send���첽��
		void send(SOCKET s,crtmembuf *buf);
		void add_transfer_socket(SOCKET s,void *addarg);
		void remove_transfer_socket(SOCKET s);
		//listen socket�����д���accept��accept��socket����transfersocket��
		void add_listen_socket(SOCKET s,void *addarg);
		void remove_listen_socket(SOCKET s);
	protected:
		virtual int run()
		{
		}
		icrtsocketnotify *m_p;
	};
};