#pragma once

#include "crtlib.h"
#include "crtstring.h"
#include "crtthread.h"
#include "crtdebug.h"
#include "libuv/uv.h"

/**
* @brief libuv封装
* @author Barry(barrytan@21cn.com,QQ:20962493)
*/
/**<pre>
  使用Sample：
</pre>*/

//mac use follow link to build: -Ltan.h/crtlib/lib/mac -luv -framework CoreServices
//in main cpp please include crtlibuv.h with define _CRTLIBUV_STATIC_LINKED
#ifdef _CRTLIBUV_STATIC_LINKED
#if _MSC_VER==1310
	#ifdef _DEBUG
		extern "C" unsigned char _BitScanReverse( unsigned long * Index, unsigned long Mask ) {
			*Index=7;
			return 1;
		}
	#endif
#endif
#endif

#ifdef _WIN32
	#pragma comment(lib,"iphlpapi.lib")
	#pragma comment(lib,"psapi.lib")
	#if _MSC_VER==1500 //vc9.0 2008
		#ifdef _DEBUG
			#pragma comment(lib,"libuv2008d.lib")
		#else
			#pragma comment(lib,"libuv2008.lib")
		#endif
	#elif _MSC_VER==1400 //vc8.0 2005
		#ifdef _DEBUG
			#pragma comment(lib,"libuv2005d.lib")
		#else
			#pragma comment(lib,"libuv2005.lib")
		#endif
	#elif _MSC_VER==1310 //vc7.1 2003
		#ifdef _DEBUG
			#pragma comment(lib,"libuv2003d.lib")
		#else
			#pragma comment(lib,"libuv2003.lib")
		#endif
	#elif _MSC_VER==1300 //vc7.0 2002
		#ifdef _DEBUG
			#pragma comment(lib,"libuv2002d.lib")
		#else
			#pragma comment(lib,"libuv2002.lib")
		#endif
	#else //if _MSC_VER=1200 //vc6.0 if _MSC_VER=1100 //vc5.0
		#ifdef _DEBUG
			#pragma comment(lib,"libuv60d.lib")
		#else
			#pragma comment(lib,"libuv60.lib")
		#endif
	#endif
#endif

//helpfile http://nikhilm.github.com/uvbook/
/*
	uv_TYPE_init(uv_TYPE_t*) for all TYPEs init
	loop:
		uv_loop_t *loop = uv_loop_new();
		uv_loop_t *loop = uv_default_loop();
		uv_run(loop);
	Timer:
		uv_timer_t timer_req;
		uv_timer_init(loop, &timer_req);
		//first starts 5 seconds,then repeats every 2 seconds, 0 for non-repeats
		uv_timer_start(&timer_req, callback, 5000, 2000);
		//stop
		uv_timer_stop(&timer_req);
		//change repeat time
		uv_timer_set_repeat(uv_timer_t *timer, int64_t repeat);
			If the timer was non-repeating, the timer has already been stopped. Use uv_timer_start again.
			If the timer is repeating, the next timeout has already been scheduled, 
			so the old repeat interval will be used once more before the timer switches to the new interval.
		//applies only to repeating timers and is equivalent to stopping the timer and then starting it
		with both initial timeout and repeat set to the old repeat value. If the timer hasn’t been started
		it fails (error code UV_EINVAL) and returns -1.
		int uv_timer_again(uv_timer_t *);
	Loading libraries:
		typedef void (*init_plugin_function)();
		uv_lib_t *lib = (uv_lib_t*) malloc(sizeof(uv_lib_t));
		if (uv_dlopen("test.dylib", lib)) {
		    fprintf(stderr, "Error: %s\n", uv_dlerror(lib));
		}
		init_plugin_function init_plugin;
		if (uv_dlsym(lib, "initialize", (void **) &init_plugin)) {
		    fprintf(stderr, "dlsym error: %s\n", uv_dlerror(lib));
		}
	Idle watcher:
		uv_loop_t *loop;
		uv_fs_t stdin_watcher;
		uv_idle_t idler;
		char buffer[1024];		
		int main() {
		    loop = uv_default_loop();
		    uv_idle_init(loop, &idler);
		    uv_fs_read(loop, &stdin_watcher, 1, buffer, 1024, -1, on_type);
		    uv_idle_start(&idler, crunch_away);
		    return uv_run(loop);
		}
	Event loop reference count:如果有任务，uv_loop会一直运行，如果启动了垃圾回收线程gcthread，又想别的任务完成后，退出loop，可以把gcthread的引用减掉
		uv_loop_t *loop;
		uv_timer_t gc_req;
		uv_timer_t fake_job_req;
		int main() {
		    loop = uv_default_loop();
		    uv_timer_init(loop, &gc_req);
		    uv_unref((uv_handle_t*) &gc_req);
		    uv_timer_start(&gc_req, gc, 0, 2000);
		    // could actually be a TCP download or something
		    uv_timer_init(loop, &fake_job_req);
		    uv_timer_start(&fake_job_req, fake_job, 9000, 0);
		    return uv_run(loop);
		}
	Passing data to worker thread：
		uv_work_t req;
		req.data=adddata;
		//queue_work中的函数在线程中运行，因此可以阻塞
		uv_queue_work(loop, &req, work_cb, cleanup_cb);
	Core thread operations:
		int main() {
			int tracklen = 10;
			uv_thread_t hare_id;
			uv_thread_t tortoise_id;
			//create thread, tracklen for adddata
			uv_thread_create(&hare_id, hare, &tracklen);
			uv_thread_create(&tortoise_id, tortoise, &tracklen);
			//wait thread
			uv_thread_join(&hare_id);
			uv_thread_join(&tortoise_id);
			return 0;
		}
		void hare(void *arg) {}
	Mutexes:
		The uv_mutex_init() and uv_mutex_trylock() functions will return 0 on success, -1 on error instead of error codes.
		If libuv has been compiled with debugging enabled, uv_mutex_destroy(), uv_mutex_lock() and uv_mutex_unlock()
		will abort() on error. Similarly uv_mutex_trylock() will abort if the error is anything other than EAGAIN.
		** Recursive mutexes are supported by some platforms, but you should not rely on them. The BSD mutex implementation
		will raise an error if a thread which has locked a mutex attempts to lock it again.
	Inter-thread communication:
		uv_loop_t *loop;
		uv_async_t async;
		int main() {
		    loop = uv_default_loop();
		    uv_work_t req;
		    int size = 10240;
		    req.data = (void*) &size;
		    uv_async_init(loop, &async, print_progress);
		    uv_queue_work(loop, &req, fake_download, after);
		    return uv_run(loop);
		}
		fake_download(uv_work_t *req){
			async.data = (void*) &percentage;
			uv_async_send(&async);
		}
		void print_progress(uv_async_t *handle, int status) {
			double percentage = *((double*) handle->data);
			fprintf(stderr, "Downloaded %.2f%%\n", percentage);
		}
		void after(uv_work_t *req) {
			fprintf(stderr, "Download complete\n");
			uv_close((uv_handle_t*) &async, NULL);
		}
		uv_async_send是异步发送，有可能会合并掉最后的发送
	Tcp Server:
		int main() {
			loop = uv_default_loop();
			uv_tcp_t server;
			uv_tcp_init(loop, &server);
			struct sockaddr_in bind_addr = uv_ip4_addr("0.0.0.0", 7000);
			uv_tcp_bind(&server, bind_addr);
			int r = uv_listen((uv_stream_t*) &server, 128, on_new_connection);
			if (r) {
			    fprintf(stderr, "Listen error %s\n", uv_err_name(uv_last_error(loop)));
			    return 1;
			}
			return uv_run(loop);
		}
		void on_new_connection(uv_stream_t *server, int status) {
		    // error!
			if (status == -1) return;
			uv_tcp_t *client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
			uv_tcp_init(loop, client);
			if (uv_accept(server, (uv_stream_t*) client) == 0)
				uv_read_start((uv_stream_t*) client, alloc_buffer, echo_read);
			else
			    uv_close((uv_handle_t*) client, NULL);
		}
	Tcp Client:
		uv_tcp_t socket;
		uv_tcp_init(loop, &socket);
		uv_connect_t connect;
		struct sockaddr_in dest = uv_ip4_addr("127.0.0.1", 80);
		uv_tcp_connect(&connect, &socket, dest, on_connect);
	Udp Server & Client:
		//The IP address 0.0.0.0 is used to bind to all interfaces.
		//The IP address 255.255.255.255 is a broadcast address meaning that packets
		//will be sent to all interfaces on the subnet.
		//port 0 means that the OS randomly assigns a port.
		//As usual the read and write callbacks will receive a status code of -1 if something went wrong.
		//The TTL of packets sent on the socket can be changed using uv_udp_set_ttl.
		//Multicast:
		//  A socket can (un)subscribe to a multicast group using:
		//  UV_EXTERN int uv_udp_set_membership(uv_udp_t* handle,
		//  where membership is UV_JOIN_GROUP or UV_LEAVE_GROUP.
		//  Local loopback of multicast packets is enabled by default [3], use uv_udp_set_multicast_loop to switch it off.
		//  The packet time-to-live for multicast packets can be changed using uv_udp_set_multicast_ttl.
		int main() {
			uv_loop_t *loop;
			uv_udp_t send_socket;
			uv_udp_t recv_socket;
			loop = uv_default_loop();
			uv_udp_init(loop, &recv_socket);
			struct sockaddr_in recv_addr = uv_ip4_addr("0.0.0.0", 68);
			uv_udp_bind(&recv_socket, recv_addr, 0);
			uv_udp_recv_start(&recv_socket, alloc_buffer, on_read);
			uv_udp_init(loop, &send_socket);
			uv_udp_bind(&send_socket, uv_ip4_addr("0.0.0.0", 0), 0);
			uv_udp_set_broadcast(&send_socket, 1);
			uv_udp_send_t send_req;
			uv_buf_t discover_msg = make_discover_msg(&send_req);
			struct sockaddr_in send_addr = uv_ip4_addr("255.255.255.255", 67);
			uv_udp_send(&send_req, &send_socket, &discover_msg, 1, send_addr, on_send);
			return uv_run(loop);
		}
		//The flags parameter may be UV_UDP_PARTIAL if the buffer provided
		//by your allocator was not large enough to hold the data.
		//In this case the OS will discard the data that could not fit
		void on_read(uv_udp_t *req, ssize_t nread, uv_buf_t buf, struct sockaddr *addr, unsigned flags) {
		    if (nread == -1) {
		        fprintf(stderr, "Read error %s\n", uv_err_name(uv_last_error(loop)));
		        uv_close((uv_handle_t*) req, NULL);
		        free(buf.base);
		        return;
		    }
		    char sender[17] = { 0 };
		    uv_ip4_name((struct sockaddr_in*) addr, sender, 16);
		    fprintf(stderr, "Recv from %s\n", sender);
		    // DHCP specific code
		    free(buf.base);
		    uv_udp_recv_stop(req);
		}
	Query DNS:
		int main() {
			loop = uv_default_loop();
			struct addrinfo hints;
			hints.ai_family = PF_INET;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;
			hints.ai_flags = 0;
			uv_getaddrinfo_t resolver;
			fprintf(stderr, "irc.freenode.net is ");
			//All arguments can be freed immediately after uv_getaddrinfo returns
			int r = uv_getaddrinfo(loop, &resolver, on_resolved, "irc.freenode.net", "6667", &hints);
			if (r) {
			    fprintf(stderr, "getaddrinfo call error %s\n", uv_err_name(uv_last_error(loop)));
			    return 1;
			}
			return uv_run(loop);
		}
		void on_resolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
			if (status == -1) {
			    fprintf(stderr, "getaddrinfo callback error %s\n", uv_err_name(uv_last_error(loop)));
			    return;
			}
			char addr[17] = {'\0'};
			uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);
			fprintf(stderr, "%s\n", addr);
			uv_connect_t *connect_req = (uv_connect_t*) malloc(sizeof(uv_connect_t));
			uv_tcp_t *socket = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
			uv_tcp_init(loop, socket);
			connect_req->data = (void*) socket;
			uv_tcp_connect(connect_req, socket, *(struct sockaddr_in*) res->ai_addr, on_connect);
			uv_freeaddrinfo(res);
		}
	Reading/Writing files:
	A file descriptor is obtained using
	int uv_fs_open(uv_loop_t* loop, uv_fs_t* req, const char* path, int flags, int mode, uv_fs_cb cb);
	uv_fs_open(uv_default_loop(), &open_req, argv[1], O_RDONLY, 0, on_open);
	flags and mode are standard Unix flags. libuv takes care of converting to the appropriate Windows flags. and close with
	int uv_fs_close(uv_loop_t* loop, uv_fs_t* req, uv_file file,
	void on_open(uv_fs_t* req){
		if (req->result != -1)
			uv_fs_read(uv_default_loop(), &read_req, req->result, buffer, sizeof(buffer), -1, on_read);
		else
			fprintf(stderr, "error opening file: %d\n", req->errorno);
		uv_fs_req_cleanup(req);
	}
	void on_read(uv_fs_t *req) {
		uv_fs_req_cleanup(req);
		if (req->result < 0)
			fprintf(stderr, "Read error: %s\n", uv_strerror(uv_last_error(uv_default_loop())));
		else if (req->result == 0) {//for EOF
			uv_fs_t close_req;
			// synchronous
			uv_fs_close(uv_default_loop(), &close_req, open_req.result, NULL);
		}
		else
			uv_fs_write(uv_default_loop(), &write_req, 1, buffer, req->result, -1, on_write);
	}
	//The error usually stored in errno can be accessed from uv_fs_t.errorno,
	//but converted to a standard UV_* error code. There is currently no way to
	//directly extract a string error message from the errorno field.
	//uv_fs_write for write, and callback after complete
	//configured for performance, a write that ‘succeeds’ may not be committed to disk yet. See uv_fs_fsync for stronger guarantees.
	void on_write(uv_fs_t *req) {
		if (req->result < 0)
			fprintf(stderr, "Write error: %s\n", uv_strerror(uv_last_error(uv_default_loop())));
		else
			uv_fs_read(uv_default_loop(), &read_req, open_req.result, buffer, sizeof(buffer), -1, on_read);
		uv_fs_req_cleanup(req);
	}
	Buffers and Streams:
		TCP sockets, UDP sockets, named pipes for file I/O and IPC
		int uv_read_start(uv_stream_t*, uv_alloc_cb alloc_cb, uv_read_cb read_cb);
		int uv_read_stop(uv_stream_t*);
		int uv_write(uv_write_t* req, uv_stream_t* handle, uv_buf_t bufs[], int bufcnt, uv_write_cb cb);
		The discrete unit of data is the buffer – uv_buf_t. This is simply a collection of a pointer to bytes (uv_buf_t.base) 
		and the length (uv_buf_t.len). The uv_buf_t is lightweight and passed around by value. 
		What does require management is the actual bytes, which have to be allocated and freed by the application.
		read on pipe(tee):
		int main(int argc, char **argv) {
			loop = uv_default_loop();
			uv_pipe_init(loop, &stdin_pipe, 0);
			uv_pipe_open(&stdin_pipe, 0);
			uv_pipe_init(loop, &stdout_pipe, 0);
			uv_pipe_open(&stdout_pipe, 1);
			uv_fs_t file_req;
			int fd = uv_fs_open(loop, &file_req, argv[1], O_CREAT | O_RDWR, 0644, NULL);
			uv_pipe_init(loop, &file_pipe, 0);
			uv_pipe_open(&file_pipe, fd);
			uv_read_start((uv_stream_t*)&stdin_pipe, alloc_buffer, read_stdin);
			uv_run(loop);
			return 0;
		}
		uv_buf_t alloc_buffer(uv_handle_t *handle, size_t suggested_size) {
			return uv_buf_init((char*) malloc(suggested_size), suggested_size);
		}
		void read_stdin(uv_stream_t *stream, ssize_t nread, uv_buf_t buf) {
			if (nread == -1) {
				if (uv_last_error(loop).code == UV_EOF) {
					uv_close((uv_handle_t*)&stdin_pipe, NULL);
					uv_close((uv_handle_t*)&stdout_pipe, NULL);
					uv_close((uv_handle_t*)&file_pipe, NULL);
				}
			}
			else {
				if (nread > 0) {
					write_data((uv_stream_t*)&stdout_pipe, nread, buf, on_stdout_write);
					write_data((uv_stream_t*)&file_pipe, nread, buf, on_file_write);
				}
			}
			if (buf.base) free(buf.base);
		}
		typedef struct {
			uv_write_t req;
			uv_buf_t buf;
		} write_req_t;
		void free_write_req(uv_write_t *req) {
			write_req_t *wr = (write_req_t*) req;
			free(wr->buf.base);
			free(wr);
		}
		void on_stdout_write(uv_write_t *req, int status) {
			free_write_req(req);
		}
		void on_file_write(uv_write_t *req, int status) {
			free_write_req(req);
		}
		void write_data(uv_stream_t *dest, size_t size, uv_buf_t buf, uv_write_cb callback) {
			write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
			req->buf = uv_buf_init((char*) malloc(size), size);
			memcpy(req->buf.base, buf.base, size);
			uv_write((uv_write_t*) req, (uv_stream_t*)dest, &req->buf, 1, callback);
		}
	File change events:
		uv_fs_event_init(loop, (uv_fs_event_t*) malloc(sizeof(uv_fs_event_t)), filename, run_command, 0);
		void run_command(uv_fs_event_t *handle, const char *filename, int events, int status) {
			fprintf(stderr, "Change detected in %s: ", handle->filename);
			if (events == UV_RENAME) fprintf(stderr, "renamed");
			if (events == UV_CHANGE) fprintf(stderr, "changed");
			fprintf(stderr, " %s\n", filename ? filename : "");
			system(command);
		}
		The callback will receive the following arguments:
			uv_fs_event_t *handle - The watcher. The filename field of the watcher is the file on which the watch was set.
			const char *filename - If a directory is being monitored, this is the file which was changed. Only non-null on Linux and Windows. May be null even on those platforms.
			int flags - one of UV_RENAME or UV_CHANGE.
			int status - Currently 0.
	other:
		//file operation of rm,stat,
		UV_FS_FSYNC,UV_FS_FDATASYNC,UV_FS_UNLINK,UV_FS_RMDIR,
		UV_FS_MKDIR,UV_FS_RENAME,UV_FS_READDIR,UV_FS_LINK,
		UV_FS_SYMLINK,UV_FS_READLINK,UV_FS_CHOWN,UV_FS_FCHOWN
		UV_EXTERN void uv_fs_req_cleanup(uv_fs_t* req);
		UV_EXTERN int uv_fs_close(uv_loop_t* loop, uv_fs_t* req, uv_file file, uv_fs_cb cb);
		UV_EXTERN int uv_fs_open(uv_loop_t* loop, uv_fs_t* req, const char* path, int flags, int mode, uv_fs_cb cb);
		UV_EXTERN int uv_fs_read(uv_loop_t* loop, uv_fs_t* req, uv_file file, void* buf, size_t length, int64_t offset, uv_fs_cb cb);
		UV_EXTERN int uv_fs_unlink(uv_loop_t* loop, uv_fs_t* req, const char* path, uv_fs_cb cb);
		UV_EXTERN int uv_fs_write(uv_loop_t* loop, uv_fs_t* req, uv_file file, void* buf, size_t length, int64_t offset, uv_fs_cb cb);
		UV_EXTERN int uv_fs_mkdir(uv_loop_t* loop, uv_fs_t* req, const char* path, int mode, uv_fs_cb cb);
		UV_EXTERN int uv_fs_rmdir(uv_loop_t* loop, uv_fs_t* req, const char* path, uv_fs_cb cb);
		UV_EXTERN int uv_fs_readdir(uv_loop_t* loop, uv_fs_t* req, const char* path, int flags, uv_fs_cb cb);
		UV_EXTERN int uv_fs_stat(uv_loop_t* loop, uv_fs_t* req, const char* path, uv_fs_cb cb);
		UV_EXTERN int uv_fs_fstat(uv_loop_t* loop, uv_fs_t* req, uv_file file, uv_fs_cb cb);
		UV_EXTERN int uv_fs_rename(uv_loop_t* loop, uv_fs_t* req, const char* path, const char* new_path, uv_fs_cb cb);
		UV_EXTERN int uv_fs_fsync(uv_loop_t* loop, uv_fs_t* req, uv_file file, uv_fs_cb cb);
		UV_EXTERN int uv_fs_fdatasync(uv_loop_t* loop, uv_fs_t* req, uv_file file, uv_fs_cb cb);
		UV_EXTERN int uv_fs_ftruncate(uv_loop_t* loop, uv_fs_t* req, uv_file file, int64_t offset, uv_fs_cb cb);
		UV_EXTERN int uv_fs_sendfile(uv_loop_t* loop, uv_fs_t* req, uv_file out_fd, uv_file in_fd, int64_t in_offset, size_t length, uv_fs_cb cb);
		UV_EXTERN int uv_fs_chmod(uv_loop_t* loop, uv_fs_t* req, const char* path, int mode, uv_fs_cb cb);
		//print ipaddress
		int uv_ip4_name(struct sockaddr_in* src, char* dst, size_t size)
*/

typedef uv_handle_t* crtlibuvobj;

namespace crtfun {
	//事件回调函数，默认线程和新建线程模型分开调用保证效率 
	typedef void (*_crtlibuvcallback)(crtlibuvobj obj, int status);
	typedef void (*_crtlibuvcallback1)(crtlibuvobj obj);
	static void _crtlibuvdefcallback(crtlibuvobj obj, int status);
	static void _crtlibuvnewcallback(crtlibuvobj obj, int status);
	static void _crtlibuvdefreqcallback(crtlibuvobj obj, int status);
	static void _crtlibuvnewreqcallback(crtlibuvobj obj, int status);
	//读事件回调函数，默认线程和新建线程模型分开调用保证效率 
	typedef void (*_crtlibuvcallbackread)(uv_stream_t *stream, ssize_t nread, uv_buf_t buf);
	static void _crtlibuvdefcallbackread(uv_stream_t *stream, ssize_t nread, uv_buf_t buf);
	static void _crtlibuvnewcallbackread(uv_stream_t *stream, ssize_t nread, uv_buf_t buf);
	typedef void (*_crtlibuvcallbackreadudp)(uv_udp_t *stream, ssize_t nread, uv_buf_t buf, struct sockaddr* addr,unsigned flags);
	static void _crtlibuvdefcallbackreadudp(uv_udp_t *stream, ssize_t nread, uv_buf_t buf, struct sockaddr* addr,unsigned flags);
	static void _crtlibuvnewcallbackreadudp(uv_udp_t *stream, ssize_t nread, uv_buf_t buf, struct sockaddr* addr,unsigned flags);
	//默认buffer管理函数
	static void _crtlibuvfreebuffer(uv_handle_t *handle, uv_buf_t buf);
	static uv_buf_t _crtlibuvallocbuffer(uv_handle_t *handle, size_t suggested_size);
	static void _crtlibuvshutdownclose(uv_handle_t* stream);
	//run_with_new_thread 使用的回调函数
	static void _crtlibuvnewthreadloopcallback(void *arg);
	class crtlibuv;
	class icrtevent{
	protected:
		friend class crtlibuv;
		virtual void ontimer(crtlibuv *uv,crtlibuvobj obj,int status) {crtdebug("[LIBUV]unhandled timer event: %d\n",status);}
		virtual void onidle(crtlibuv *uv,crtlibuvobj obj,int status) {}
		virtual void onclose(crtlibuv *uv,crtlibuvobj obj) {crtdebug("[LIBUV]socket %p closed.\n",obj);}
		//return 0 that means need the buffer for next read, not need to free it
		virtual int onreceive(crtlibuv *uv,crtlibuvobj obj, uv_buf_t buf, ssize_t nread) {
			crtdebug("[LIBUV]socket %p receive %d data.\n",obj,nread);
			return 1;
		}
		//return 0 that means need the buffer for next read, not need to free it
		virtual int onreceiveudp(crtlibuv *uv,crtlibuvobj obj, uv_buf_t buf, ssize_t nread, struct sockaddr* addr) {
			//crtdebug("[LIBUV]socket %p receive %d data from %s.\n",obj,nread,socket_ip4nameport(addr).c_str());
			crtdebug("[LIBUV]socket %p receive %d data.\n",obj,nread);
			return 1;
		}
		virtual void onaccept(crtlibuv *uv, crtlibuvobj server, crtlibuvobj client) {crtdebug("[LIBUV]socket %p accepted.\n",client);}
		virtual void onsended(crtlibuv *uv, crtlibuvobj sock) {crtdebug("[LIBUV]socket %p sended.\n",sock);}
	};
	class crtlibuv {
		typedef void (crtlibuv::*libuv_timer_cb)(uv_timer_t* handle, int status);
	private:
		crtlibuv(){loop=NULL;eventer=NULL;}
	public:
		//-------------------------base functions-----------------------
		//用主线程来循环uv线程 
		static crtlibuv *getinstance() {
			static crtlibuv *uv=NULL;
			if (!uv) {
				uv=new crtlibuv();
				uv->loop=uv->mainloop=uv_default_loop();
			}
			return uv;
		}
		//新建线程来循环uv线程 
		static crtlibuv *getnewloop() {
			crtlibuv *uv=new crtlibuv();
			uv->loop=uv_loop_new();
			uv->mainloop=uv_default_loop();
			return uv;
		}
		//该函数慎用，释放前确认callback等都释放了 
		static void freeloop(crtlibuv *lib) {delete lib;}
		//开始uv消息循环，如果没有指定消息，会马上退出 
		void run() {uv_run(loop);}
		void run_with_new_thread() {crtthread::startwithfunction(_crtlibuvnewthreadloopcallback,this);}
		static void freeobj(crtlibuvobj obj){if (obj) delete obj;}
		void unref_obj(crtlibuvobj obj){uv_unref(obj);}
		int getlasterror() {return (int)uv_last_error(loop).code;}
		const char* getlaststrerror() {return uv_strerror(uv_last_error(loop));}
		const char* getlasterrorname() {return uv_err_name(uv_last_error(loop));}
		//----------------event handler functions-----------------------
		void seteventcallback(icrtevent *ev) {eventer=ev;}
		//------------------------timer functions-----------------------
		//repeatdelay=0 for non-repeats
		crtlibuvobj timer_start(int firstdelay,int repeatdelay=0) {
			uv_timer_t *t=new uv_timer_t;
			uv_timer_init(loop, t);
			_crtlibuvcallback cb;
			if (mainloop!=loop) {
				getinstance()->setupmapping((crtlibuvobj)t,this);
				cb=_crtlibuvnewcallback;
			} else
				cb=_crtlibuvdefcallback;
			uv_timer_start(t, (uv_timer_cb)cb, firstdelay, repeatdelay);
			return (crtlibuvobj)t;
		}
		void timer_stop(crtlibuvobj obj,bool bfree=true) {
			if (obj) {
				uv_timer_stop((uv_timer_t *)obj);
				if (bfree) {
					if (mainloop!=loop) getinstance()->releasemapping(obj);
					freeobj(obj);
				}
			}
		}
		void timer_restart(crtlibuvobj obj) {if (obj) uv_timer_again((uv_timer_t *)obj);}
		void timer_reset(crtlibuvobj obj,int repeatdelay) {if (obj) uv_timer_set_repeat((uv_timer_t *)obj, repeatdelay);}
		//------------------------load library functions-----------------------
		static crtlibuvobj library_open(const char *filename) {
			uv_lib_t *obj = new uv_lib_t;
			if (uv_dlopen("test.dylib", obj)) {
				freeobj((crtlibuvobj)obj);
				return NULL;
			}
			return (crtlibuvobj)obj;
		}
		static bool library_findfunction(crtlibuvobj obj, const char *fnname, void **fnpointer) {
			return uv_dlsym((uv_lib_t *)obj, fnname, fnpointer)==0;
		}
		static void library_close(crtlibuvobj obj, bool bfree=true) {
			if (obj) {
				uv_dlclose((uv_lib_t *)obj);
				if (bfree) freeobj(obj);
			}
		}
		//------------------------idle run functions-----------------------
		crtlibuvobj idle_start() {
			uv_idle_t *t=new uv_idle_t;
			uv_idle_init(loop, t);
			_crtlibuvcallback cb;
			if (mainloop!=loop) {
				getinstance()->setupmapping((crtlibuvobj)t,this);
				cb=_crtlibuvnewcallback;
			} else
				cb=_crtlibuvdefcallback;
			uv_idle_start(t, (uv_idle_cb)cb);
			return (crtlibuvobj)t;
		}
		void idle_stop(crtlibuvobj obj,bool bfree=true) {
			if (obj) {
				uv_idle_stop((uv_idle_t *)obj);
				if (bfree) {
					if (mainloop!=loop) getinstance()->releasemapping(obj);
					freeobj(obj);
				}
			}
		}
		//------------------------work thread functions-----------------------
		//uv_thread_create uv_queue_work uv_async_init
		//----------------------------socket functions------------------------
		static struct sockaddr_in socket_ip4addr(const char *ip, unsigned short port) {return uv_ip4_addr(ip, port);}
		static string socket_ip4name(struct sockaddr_in* in) {
			char buf[64];
			if (uv_ip4_name(in, buf, 64)==0) return buf;
			return "";
		}
		static string socket_ip4nameport(struct sockaddr_in* in) {
			char buf[70];
			sprintf(buf,"%s:%d",socket_ip4name(in).c_str(),ntohs(in->sin_port));
			return buf;
		}
		crtlibuvobj socket_tcpserver_start(const struct sockaddr_in &addr, int pendingqueue=128) {
			uv_tcp_t *t=new uv_tcp_t;
			uv_tcp_init(loop, t);
			if (uv_tcp_bind(t, addr)) {
				freeobj((crtlibuvobj)t);
				return NULL;
			}
			_crtlibuvcallback cb;
			if (mainloop!=loop) {
				getinstance()->setupmapping((crtlibuvobj)t,this);
				cb=_crtlibuvnewcallback;
			} else
				cb=_crtlibuvdefcallback;
			if (uv_listen((uv_stream_t*)t, pendingqueue, (uv_connection_cb)cb)) {
				freeobj((crtlibuvobj)t);
				return NULL;
			}
			return (crtlibuvobj)t;
		}
		void socket_tcpserver_stop(crtlibuvobj obj) {
			if (mainloop!=loop) getinstance()->releasemapping(obj);
			socket_shutdown(obj);
		}
		crtlibuvobj socket_udpserver_start(const struct sockaddr_in &addr) {
			uv_udp_t *t=new uv_udp_t;
			uv_udp_init(loop, t);
			if (uv_udp_bind(t, addr, 0)) {
				freeobj((crtlibuvobj)t);
				return NULL;
			}
			_crtlibuvcallbackreadudp cb;
			if (mainloop!=loop) {
				getinstance()->setupmapping((crtlibuvobj)t,this);
				cb=_crtlibuvnewcallbackreadudp;
			} else
				cb=_crtlibuvdefcallbackreadudp;
			if (uv_udp_recv_start(t, _crtlibuvallocbuffer, cb)) {
				freeobj((crtlibuvobj)t);
				return NULL;
			}
			return (crtlibuvobj)t;
		}
		static void socket_tcp_nodelay(crtlibuvobj obj, int enable) {uv_tcp_nodelay((uv_tcp_t *)obj, enable);}
		static void socket_tcp_keepalive(crtlibuvobj obj, int delaysec) {uv_tcp_keepalive((uv_tcp_t *)obj, 1, delaysec);}
		static void socket_tcp_nokeepalive(crtlibuvobj obj) {uv_tcp_keepalive((uv_tcp_t *)obj, 0, 0);}
		static bool socket_get_sock_name(crtlibuvobj obj, struct sockaddr* name, int* namelen) {
			if (obj->type==UV_TCP) return uv_tcp_getsockname((uv_tcp_t *)obj, name, namelen)==0;
			else if (obj->type==UV_UDP) return uv_udp_getsockname((uv_udp_t *)obj, name, namelen)==0;
			return false;
		}
		static uint32_t socket_tcp_get_peer_ip4_n(crtlibuvobj obj) {
			struct sockaddr_in addr;
			int socklen=sizeof(sockaddr_in);
			if (socket_tcp_get_peer_name(obj, (sockaddr *)&addr, &socklen))
				return addr.sin_addr.s_addr;
			return 0;
		}
		static bool socket_tcp_get_peer_name(crtlibuvobj obj, struct sockaddr* name, int* namelen) {
			return uv_tcp_getpeername((uv_tcp_t *)obj,name,namelen)==0;
		}
		static bool socket_udp_set_broadcast(crtlibuvobj obj, int on) {return uv_udp_set_broadcast((uv_udp_t *)obj, on)==0;}
		static bool socket_udp_set_ttl(crtlibuvobj obj, int ttl) {return uv_udp_set_ttl((uv_udp_t *)obj, ttl)==0;}
		bool socket_tcp_send(crtlibuvobj obj, const char *buf, int buflen, bool bshutdown) {
			uv_buf_t uvbuf=buffer_alloc(buflen);
			memcpy(uvbuf.base,buf,buflen);
			return socket_tcp_send(obj, uvbuf, bshutdown);
		}
		bool socket_tcp_send(crtlibuvobj obj, uv_buf_t buf, bool bshutdown) {
			writepair *pair=new writepair();
			pair->stream=(uv_stream_t *)obj;
			pair->buf=buf;
			pair->bshutdown=bshutdown;
			_crtlibuvcallback cb;
			if (mainloop!=loop) {
				getinstance()->setupmapping((crtlibuvobj)pair,this);
				cb=_crtlibuvnewreqcallback;
			} else
				cb=_crtlibuvdefreqcallback;
			return uv_write(&pair->u.req, (uv_stream_t *)obj, &buf, 1, (uv_write_cb)cb)==0;
		}
		bool socket_udp_send(crtlibuvobj obj, const char *buf, int buflen, struct sockaddr* addr) {
			uv_buf_t uvbuf=buffer_alloc(buflen);
			memcpy(uvbuf.base,buf,buflen);
			return socket_udp_send(obj, uvbuf, addr);
		}
		bool socket_udp_send(crtlibuvobj obj, uv_buf_t buf, struct sockaddr* addr) {
			writepair *pair=new writepair();
			pair->stream=(uv_stream_t *)obj;
			pair->buf=buf;
			pair->bshutdown=false;
			_crtlibuvcallback cb;
			if (mainloop!=loop) {
				getinstance()->setupmapping((crtlibuvobj)pair,this);
				cb=_crtlibuvnewreqcallback;
			} else
				cb=_crtlibuvdefreqcallback;
			return uv_udp_send(&pair->u.udp, (uv_udp_t *)obj, &buf, 1, *(struct sockaddr_in *)addr, (uv_udp_send_cb)cb)==0;
		}
		void socket_shutdown(crtlibuvobj stream) {
			if (stream->type==UV_TCP) {
				shutdownpair *shutdown=new shutdownpair;
				shutdown->stream=(uv_stream_t *)stream;
				_crtlibuvcallback cb;
				if (mainloop!=loop) {
					getinstance()->setupmapping((crtlibuvobj)shutdown,this);
					cb=_crtlibuvnewreqcallback;
				} else
					cb=_crtlibuvdefreqcallback;
				uv_shutdown(&shutdown->req, (uv_stream_t *)stream, (uv_shutdown_cb)cb);
			} else if (stream->type==UV_UDP) {
				socket_close((uv_handle_t *)stream);
			}
		}
		//----------------------buffer functions----------------------
		static void buffer_free(uv_buf_t buf) {
			if (buf.base) delete [] buf.base;
		}
		static uv_buf_t buffer_alloc(size_t size) {
			return uv_buf_init(new char[size], (unsigned int)size);
		}
		//----------------------utils functions-----------------------
		static uint64_t utils_get_tick_count_ns() {return uv_hrtime();}
		static uint64_t utils_get_free_memory() {return uv_get_free_memory();}
		static uint64_t utils_get_total_memory() {return uv_get_total_memory();}
	protected:
		friend uv_buf_t _crtlibuvallocbuffer(uv_handle_t *handle, size_t suggested_size);
		friend void _crtlibuvdefcallback(crtlibuvobj obj, int status);
		friend void _crtlibuvnewcallback(crtlibuvobj obj, int status);
		friend void _crtlibuvdefreqcallback(crtlibuvobj obj, int status);
		friend void _crtlibuvnewreqcallback(crtlibuvobj obj, int status);
		friend void _crtlibuvdefcallbackread(uv_stream_t *stream, ssize_t nread, uv_buf_t buf);
		friend void _crtlibuvnewcallbackread(uv_stream_t *stream, ssize_t nread, uv_buf_t buf);
		friend void _crtlibuvdefcallbackreadudp(uv_udp_t *stream, ssize_t nread, uv_buf_t buf, struct sockaddr* addr,unsigned flags);
		friend void _crtlibuvnewcallbackreadudp(uv_udp_t *stream, ssize_t nread, uv_buf_t buf, struct sockaddr* addr,unsigned flags);
		//该函数只在主线程对象有用 
		inline void dispatchevent(crtlibuvobj obj, int status){
			//assert(mainloop==loop);
			callbackmapping[obj]->onevent(obj, status);
		}
		inline void dispatchrequest(crtlibuvobj obj, int status){
			//assert(mainloop==loop);
			callbackmapping[obj]->onrequest(obj, status);
		}
		//所有libuv的回调都会在onevent收到并转发
		virtual void onevent(crtlibuvobj obj, int status) {
			if (!eventer && obj->type!=UV_IDLE) crtdebug("[LIBUV]event handler not set, eventtype: %d\n",obj->type);
			switch(obj->type) {
				/*case UV_ASYNC:break;
				case UV_CHECK:break;
				case UV_FS_EVENT:break;
				case UV_FS_POLL:break;*/
				case UV_IDLE:if (eventer) eventer->onidle(this, obj, status);break;
				/*case UV_NAMED_PIPE:break;
				case UV_POLL:break;
				case UV_PREPARE:break;
				case UV_PROCESS:break;*/
				case UV_TCP:ontcp(obj, status);break;
				case UV_TIMER:if (eventer) eventer->ontimer(this, obj, status);break;
				/*case UV_TTY:break;
				case UV_UDP:break;
				case UV_SIGNAL:break;*/
				default:
					crtdebug("[LIBUV]unhandled event: %d\n",obj->type);
			}
		}
		virtual void onrequest(crtlibuvobj uvobj, int status) {
			uv_req_t *obj=(uv_req_t *)uvobj;
			if (!eventer) crtdebug("[LIBUV]event handler not set, requesttype: %d\n",obj->type);
			switch(obj->type) {
				case UV_WRITE:
				{
					writepair *pair=(writepair *)uvobj;
					buffer_free(pair->buf);
					if (pair->bshutdown) socket_shutdown((crtlibuvobj)pair->stream);
					else if (eventer) eventer->onsended(this, (crtlibuvobj)pair->stream);
					delete pair;
				}
					break;
				case UV_UDP_SEND:
				{
					writepair *pair=(writepair *)uvobj;
					buffer_free(pair->buf);
					delete pair;
				}
					break;
				case UV_SHUTDOWN:
				{
					uv_stream_t **stream=(uv_stream_t **)(((uv_shutdown_t *)uvobj)+1);
					uv_handle_t *handle=(uv_handle_t *)*stream;
					delete (shutdownpair *)uvobj;
					socket_close(handle);
				}
					break;
				default:
					crtdebug("[LIBUV]unhandled request: %d\n",obj->type);
			}
		}
		inline void dispatchread(uv_stream_t *stream, ssize_t nread, uv_buf_t buf) {
			callbackmapping[(crtlibuvobj)stream]->onread(stream, nread, buf);
		}
		virtual void onread(uv_stream_t *stream, ssize_t nread, uv_buf_t buf) {
			if (nread == -1) {
				//if (uv_last_error(loop).code == UV_EOF)
				socket_shutdown((uv_handle_t *)stream);
			} else if (nread>0 && eventer) {
				//return 0 to save the buffer for next read
				if (eventer->onreceive(this, (crtlibuvobj)stream, buf, nread)==0) return;
			}
			_crtlibuvfreebuffer((uv_handle_t *)stream, buf);
		}
		inline void dispatchreadudp(uv_udp_t *stream, ssize_t nread, uv_buf_t buf, struct sockaddr* addr,unsigned flags) {
			callbackmapping[(crtlibuvobj)stream]->onreadudp(stream, nread, buf, addr, flags);
		}
		virtual void onreadudp(uv_udp_t *stream, ssize_t nread, uv_buf_t buf, struct sockaddr* addr,unsigned flags) {
			if (nread <= 0) return;
			//return 0 to save the buffer for next read
			if (eventer && eventer->onreceiveudp(this, (crtlibuvobj)stream, buf, nread, addr)==0) return;
			_crtlibuvfreebuffer((uv_handle_t *)stream, buf);
		}
		void socket_close(uv_handle_t *stream) {
			if (eventer) eventer->onclose(this, stream);
			if (mainloop!=loop) getinstance()->releasemapping((crtlibuvobj)stream);
			uv_close(stream,_crtlibuvshutdownclose);
		}
		virtual void ontcp(crtlibuvobj obj,int status){
			if (status == -1) return;
			uv_tcp_t *client = new uv_tcp_t;
			uv_tcp_init(loop, client);
			_crtlibuvcallbackread cb;
			if (uv_accept((uv_stream_t*)obj, (uv_stream_t*)client) == 0) {
				if (mainloop!=loop) {
					getinstance()->setupmapping((crtlibuvobj)client,this);
					cb=_crtlibuvnewcallbackread;
				} else
					cb=_crtlibuvdefcallbackread;
				uv_read_start((uv_stream_t*)client, _crtlibuvallocbuffer, cb);
				if (eventer) eventer->onaccept(this, obj, (crtlibuvobj)client);
			} else
			    socket_close((uv_handle_t*)client);
		}
		typedef struct _writepair{
			union {
				uv_write_t req;
				uv_udp_send_t udp;
			}u;
			uv_buf_t buf;
			uv_stream_t *stream;
			bool bshutdown;
		}writepair;
		typedef struct _shutdownpair{
			uv_shutdown_t req;
			uv_stream_t *stream;
		}shutdownpair;
		//--------------------------以下函数只在主线程对象有用-------------------------------
		inline void setupmapping(crtlibuvobj obj, crtlibuv *t) {
			mtx.lock();
			callbackmapping[(crtlibuvobj)obj]=t;
			mtx.unlock();
		}
		inline void releasemapping(crtlibuvobj obj) {
			mtx.lock();
			callbackmapping.erase(obj);
			mtx.unlock();
		}
		//用于新建线程中的static callback回调到对应类中
		map<crtlibuvobj,crtlibuv *> callbackmapping;
		crtmutex mtx;
		//------------------------------------------------------------------------------
		uv_loop_t *mainloop;
		uv_loop_t *loop;
		icrtevent *eventer;
	};
	static void _crtlibuvdefreqcallback(crtlibuvobj obj, int status) {
		crtlibuv::getinstance()->onrequest(obj, status);
	}
	static void _crtlibuvnewreqcallback(crtlibuvobj obj, int status) {
		crtlibuv::getinstance()->dispatchrequest(obj, status);
	}
	static void _crtlibuvshutdownclose(uv_handle_t* stream) {
		delete stream;
	}
	static void _crtlibuvfreebuffer(uv_handle_t *handle, uv_buf_t buf) {
		crtlibuv::buffer_free(buf);
	}
	static uv_buf_t _crtlibuvallocbuffer(uv_handle_t *handle, size_t suggested_size) {
		return crtlibuv::buffer_alloc(suggested_size);
	}
	static void _crtlibuvdefcallback(crtlibuvobj obj, int status) {
		crtlibuv::getinstance()->onevent(obj, status);
	}
	static void _crtlibuvnewcallback(crtlibuvobj obj, int status) {
		crtlibuv::getinstance()->dispatchevent(obj, status);
	}
	static void _crtlibuvdefcallbackread(uv_stream_t *stream, ssize_t nread, uv_buf_t buf) {
		crtlibuv::getinstance()->onread(stream, nread, buf);
	}
	static void _crtlibuvnewcallbackread(uv_stream_t *stream, ssize_t nread, uv_buf_t buf) {
		crtlibuv::getinstance()->dispatchread(stream, nread, buf);
	}
	static void _crtlibuvdefcallbackreadudp(uv_udp_t *stream, ssize_t nread, uv_buf_t buf, struct sockaddr* addr,unsigned flags) {
		crtlibuv::getinstance()->onreadudp(stream, nread, buf, addr, flags);
	}
	static void _crtlibuvnewcallbackreadudp(uv_udp_t *stream, ssize_t nread, uv_buf_t buf, struct sockaddr* addr,unsigned flags) {
		crtlibuv::getinstance()->dispatchreadudp(stream, nread, buf, addr, flags);
	}
	static void _crtlibuvnewthreadloopcallback(void *arg) {
		//if not have event setted, run() will exit, so while(1) call it
		while(1) {
			((crtlibuv *)arg)->run();
			ms_sleep(100);
		}
	}
	class crtlibuv_rwlock {
	public:
		crtlibuv_rwlock(){uv_rwlock_init(&rwlock);}
		virtual ~crtlibuv_rwlock(){uv_rwlock_destroy(&rwlock);}
		void readlock(){uv_rwlock_rdlock(&rwlock);}
		bool tryreadlock(){return uv_rwlock_tryrdlock(&rwlock)==0;}
		void unreadlock(){uv_rwlock_rdunlock(&rwlock);}
		void writelock(){uv_rwlock_wrlock(&rwlock);}
		bool trywritelock(){return uv_rwlock_trywrlock(&rwlock)==0;}
		void unwritelock(){uv_rwlock_wrunlock(&rwlock);}
	protected:
		uv_rwlock_t rwlock;
	};
};
