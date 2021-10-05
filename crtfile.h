#pragma once

#include "crtlib.h"
#include "crtstring.h"
#include "crttime.h"
#include "crtsystem.h"
#include "crtcharset.h"

namespace crtfun {
	/**
	* @brief 文件操作类
	* @author Barry(barrytan@21cn.com,QQ:20962493)
	*/
	/**<pre>
	文件名统一使用utf8，windows函数不用fopen是因为fseeko 2005以上才支持，linux不用fopen是因为ftruncate。 
	使用Sample：
	</pre>*/
	class crtfile{
	public:
		crtfile(){m_handle=INVALID_HANDLE_VALUE;}
		virtual ~crtfile(){close();}
		typedef enum _openmode{
			openmode_read = 1,			///<读 
			openmode_readwrite = 2,		///<读写 
			openmode_shared = 3,		///<读写，允许共享 
			openmode_write = 4,			///<写，总是新建 
		}openmode;
		virtual bool openu8(const char *filename,int openmode){
			close();
#ifdef _WIN32
			m_handle=CreateFileW(crtu82ucs(filename).c_str(),openmode==openmode_read?GENERIC_READ:GENERIC_READ|GENERIC_WRITE,
				openmode==openmode_shared?FILE_SHARE_READ|FILE_SHARE_WRITE:0,NULL,openmode==openmode_write?CREATE_ALWAYS:OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
#else
			int mode;
			switch(openmode) {
				case openmode_write:mode=O_RDWR|O_TRUNC|O_CREAT;break;
				case openmode_shared:
				case openmode_readwrite:
					mode=O_RDWR|O_CREAT;break;
				case openmode_read:
				default:
					mode=O_RDONLY;
			}
			m_handle=open(filename,mode);
#endif
			return m_handle!=INVALID_HANDLE_VALUE;
		}
		virtual void close(){
			if (m_handle!=INVALID_HANDLE_VALUE) {
#ifdef _WIN32
				CloseHandle(m_handle);
#else
				::close(m_handle);
#endif
				m_handle=INVALID_HANDLE_VALUE;
			}
		}
		size_t read(void *buf, size_t len){
#ifdef _WIN32
			DWORD readed=0;
			if (!ReadFile(m_handle, buf, (DWORD)len, &readed, NULL)) return 0;
			return readed;
#else
			ssize_t slen=::read(m_handle,buf,len);
			if (slen<=0) slen=0;
			return slen;
#endif
		}
		bool write(const void *buf, size_t len){
#ifdef _WIN32
			DWORD writed=0;
			if (!WriteFile(m_handle, buf, (DWORD)len, &writed, NULL)) return false;
			if (writed!=len) return false;
			return true;
#else
			return ::write(m_handle,buf,len)==len;
#endif
		}
		bool seek(off_t offset, int whence){
#ifdef _WIN32
			LARGE_INTEGER li;
			li.QuadPart = offset;
			li.LowPart = SetFilePointer(m_handle,li.LowPart,&li.HighPart,whence);
			if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) return false;
			return true;
#else
			return lseek(m_handle,offset,whence)!=-1;
#endif
		}
		off_t tell(){
#ifdef _WIN32
			LARGE_INTEGER li;
			li.QuadPart = 0;
			li.LowPart = SetFilePointer(m_handle,li.LowPart,&li.HighPart,FILE_CURRENT);
			if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) return -1;
			return (off_t)li.QuadPart;
#else
			return lseek(m_handle,0,SEEK_CUR);
#endif
		}
		off_t getsize() {
#ifdef _WIN32
			LARGE_INTEGER li;
			DWORD ret;
			li.LowPart = GetFileSize(m_handle,&ret);
			if (li.LowPart == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) return -1;
			li.HighPart=ret;
			return (off_t)li.QuadPart;
#else
			if (lseek(m_handle,0,SEEK_END)==-1) return -1;
			return lseek(m_handle,0,SEEK_CUR);
#endif
		}
		bool setsize(off_t offest){
#ifdef _WIN32
			return SetFileValidData(m_handle,offest)!=FALSE;
#else
			return ftruncate(m_handle,offest)==0;
#endif
		}
		static bool savestr_to_file(const char *filename,const char *string) {
			crtfile f;
			if (!f.openu8(filename,crtfile::openmode_write)) return false;
			if (!f.write(string,strlen(string))) return false;
			f.close();
			return true;
		}
		static string readstr_from_file(const char *filename) {
			crtfile f;
			string ret;
			if (f.openu8(filename,crtfile::openmode_read)) {
				off_t size=f.getsize();
				if (size!=-1 && f.seek(0,SEEK_SET)) {
					char *buf=new char[size+1];
					if (f.read(buf,size)==size) {
						buf[size]='\0';
						ret=buf;
					}
					delete [] buf;
				}
				f.close();
			}
			return ret;
		}
	protected:
		HANDLE m_handle;
	};
	class crtdirectaceessfile:public crtfile{
	public:
		crtdirectaceessfile(){m_off=-1;}
		virtual ~crtdirectaceessfile(){m_off=-1;}
		virtual bool openu8(const char *filename,int openmode){
			if (crtfile::openu8(filename,openmode)){
				m_off=0;
				return true;
			}
			m_off=-1;
			return false;
		}
		virtual void close(){
			crtfile::close();
			m_off=-1;
		}
		virtual size_t read(void *buf, size_t len){
			size_t readed=crtfile::read(buf,len);
			if (m_off!=-1) m_off+=(off_t)readed;
			return m_off;
		}
		virtual bool write(const void *buf, size_t len){
			if (crtfile::write(buf,len)) {
				if (m_off!=-1) m_off+=(off_t)len;
				return true;
			}
			m_off=-1;
			return false;
		}
		virtual size_t readoff(void *buf,size_t len,off_t off){
			if (off!=m_off) seek(off,SEEK_SET);
			return read(buf,len);
		}
		virtual bool writeoff(const void *buf,size_t len,off_t off){
			if (off!=m_off) seek(off,SEEK_SET);
			return write(buf,len);
		}
		virtual bool seek(off_t offset, int whence){
			if (crtfile::seek(offset,whence)){
				if (whence==SEEK_SET) m_off=offset;
				m_off=-1;
				return true;
			}
			m_off=-1;
			return false;
		}
#ifndef _WIN32
		virtual off_t getsize(){m_off=-1;return crtfile::getsize();}
#endif
		virtual bool setsize(off_t offest){
			m_off=-1;
			return crtfile::setsize(offest);
		}
	protected:
		volatile off_t m_off;
		//fixme 注意这里没有用lock
	};
	static string crtget_temp_path() {
		string str;
		char buf[MAX_PATH];
	#ifdef _WIN32
		::GetTempPath(MAX_PATH,buf);
		str=buf;
	#elif defined(__APPLE__)
		str="/var/tmp";
	#else
		str="/tmp";
	#endif
		return str;
	}
	static string crtget_temp_file(const char *prefix="crttmp") {
		char temp[64];
		string temppath=crtget_temp_path();
		temppath+=CRTPATH_SEPERATOR_STRING;
		temppath+=prefix;
		crtsrand();
		sprintf(temp,"%04u%04u%04u",rand()%65535,rand()%65535,rand()%65535);
		temppath+=temp;
		return temppath;
	}
	static bool crtmake_dir(const char *dir) {
	#ifdef _WIN32
		return ::CreateDirectoryW(crtu82ucs(dir).c_str(),NULL)!=FALSE;
	#else
		return mkdir(dir,S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH)==0;
	#endif
	}
	static bool crtdelete_file(const char *file) {
	#ifdef _WIN32
		return ::DeleteFileW(crtu82ucs(file).c_str())!=FALSE;
	#else
		return unlink(file)==0;
	#endif
	};
	//execute file
	static string crtget_runnable_name() {
	#ifdef _WIN32
		wchar_t buf[MAX_PATH];
		::GetModuleFileNameW(NULL,buf,MAX_PATH);
		return crtucs2u8(buf);
	#elif defined(__APPLE__)
		char buf[MAX_PATH];
		uint32_t usize=MAX_PATH;
		char buf2[2 * PATH_MAX];
		int result = _NSGetExecutablePath(buf, &usize);
		if (result) return "";
		char *fullpath = realpath(buf, buf2);
		if (fullpath == NULL) return "";
		return fullpath;
	#else
		char buf[MAX_PATH];
		char str[64];
		sprintf(str,"/proc/%lu/exe",crtget_pid());
		int count=readlink(str,buf,MAX_PATH);
		if (count>0 && count<MAX_PATH)
		{
			buf[count]='\0';
			return buf;
		}
		return "";
	#endif
	}
	static string crtget_runnable_path(const char *filename=NULL) {
		string str = get_file_path(crtget_runnable_name());
		if (filename) {
			str+=CRTPATH_SEPERATOR_STRING;
			str+=filename;
		}
		return str;
	}
	//execute path
	static string crtget_current_path(const char *filename=NULL) {
		string str;
	#ifdef _WIN32
		wchar_t wbuf[MAX_PATH];
		::GetCurrentDirectoryW(MAX_PATH,wbuf);
		str=crtucs2u8(wbuf);
	#else
		char buf[MAX_PATH];
		char tmp[64];
		sprintf(tmp,"/proc/%lu/cwd",crtget_pid());
		int count=readlink(tmp,buf,MAX_PATH);
		if (count>0 && count<MAX_PATH)
		{
			buf[count]='\0';
			str=buf;
		}
	#endif
		if (filename) {
			str+=CRTPATH_SEPERATOR_STRING;
			str+=filename;
		}
		return str;
	}
	typedef struct _crtfileinfo{
		size_t filesize;
		time_t ctime;
		string ext;
	}crtfileinfo;
	typedef struct _crtfileinfow{
		size_t filesize;
		time_t ctime;
		wstring ext;
	}crtfileinfow;
#ifdef _WIN32
	static bool get_file_listw(const wstring &path,map<wstring,crtfileinfow> &strar,bool recursive=true,const wchar_t *extfilter=NULL) {
		WIN32_FIND_DATAW FindFileData;
		wstring find=path;
		find+=CRTPATH_SEPERATOR_STRINGL;
		find+=L"*.*";
		HANDLE hFind = FindFirstFileW(find.c_str(),&FindFileData);
		if (hFind == INVALID_HANDLE_VALUE) return false;
		BOOL bRet=TRUE;
		crtfileinfow fi;
		while(bRet)
		{
			if (wcscmp(FindFileData.cFileName,L".")!=0 &&
				wcscmp(FindFileData.cFileName,L"..")!=0)
			{
				wstring str=path;
				str+=CRTPATH_SEPERATOR_STRINGL;
				str+=FindFileData.cFileName;
				if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					if (recursive) get_file_listw(str.c_str(),strar,recursive,extfilter);
				} else {
					size_t pos=str.rfind('.');
					if (pos!=string::npos)
					{
						wstring ext=str.substr(pos)+L";";
						if (extfilter==NULL || wcsstr(extfilter,ext.c_str()))
						{
							fi.ctime=filetime2time_t(FindFileData.ftCreationTime);
							fi.filesize=(FindFileData.nFileSizeHigh << 32) + FindFileData.nFileSizeLow;
							fi.ext=ext;
							strar.insert(map<wstring,crtfileinfow>::value_type(str,fi));
						}
					}
				}
			}
			bRet = FindNextFileW(hFind,&FindFileData);
		}
		FindClose(hFind);
		return true;
	}
	//extfilter format e.g.: .flv;.mp4;
	static bool get_file_listu8(const char *path,map<string,crtfileinfo> &strar,bool recursive=true,const char *extfilter=NULL)
	{
		map<wstring,crtfileinfow> ar;
		bool ret=get_file_listw(crtu82ucs(path).c_str(),ar,recursive,crtu82ucs(extfilter).c_str());
		map<wstring,crtfileinfow>::const_iterator it=ar.begin();
		while(it!=ar.end()) {
			it++;
			crtfileinfo w;
			w.ctime=it->second.ctime;
			w.filesize=it->second.filesize;
			w.ext=crtucs2u8(it->second.ext);
			strar.insert(map<string,crtfileinfo>::value_type(crtucs2u8(it->first),w));
		}
		return ret;
	}
	static bool unlink_file_or_directoryw(const wchar_t *path)
	{
		struct _stat s;
		memset(&s,0,sizeof(struct _stat));
		_wstat(path,&s);
		if (!S_ISDIR(s.st_mode)) return _wunlink(path)==0;
		WIN32_FIND_DATAW FindFileData;
		wstring find=path;
		find+=CRTPATH_SEPERATOR_STRINGL;
		find+=L"*.*";
		HANDLE hFind = FindFirstFileW(find.c_str(),&FindFileData);
		if (hFind == INVALID_HANDLE_VALUE) return false;
		BOOL bRet=TRUE;
		while(bRet)
		{
			if (wcscmp(FindFileData.cFileName,L".")!=0 &&
				wcscmp(FindFileData.cFileName,L"..")!=0)
			{
				wstring str=path;
				str+=CRTPATH_SEPERATOR_STRINGL;
				str+=FindFileData.cFileName;
				if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					unlink_file_or_directoryw(str.c_str());
				else
					_wunlink(str.c_str());
			}
			bRet = FindNextFileW(hFind,&FindFileData);
		}
		FindClose(hFind);
		return RemoveDirectoryW(path)!=FALSE;
	}
	static bool unlink_file_or_directoryu8(const char *path) {return unlink_file_or_directoryw(crtu82ucs(path).c_str());}
#else
	//extfilter format e.g.: .flv;.mp4;
	static bool get_file_listu8(const char *path,map<string,crtfileinfo> &strar,bool recursive=true,const char *extfilter=NULL)
	{
		DIR *dp;
		struct dirent *dirp;
		struct stat statbuf;
		crtfileinfo fi;
		if ((dp=opendir(path))!=NULL)
		{
			while((dirp=readdir(dp))!=NULL)
			{
				if(strcmp(dirp->d_name,".")==0 || strcmp(dirp->d_name,"..")==0) continue;
				string str;
				str=path;str+=CRTPATH_SEPERATOR_STRING;
				str+=dirp->d_name;
				if (stat(str.c_str(),&statbuf)==0)
				{
					if (S_ISDIR(statbuf.st_mode)) {
						if (recursive) get_file_listu8(str.c_str(),strar,recursive,extfilter);
					} else {
						size_t pos=str.rfind('.');
						if (pos!=string::npos)
						{
							string ext=str.substr(pos)+";";
							if (extfilter==NULL || strstr(extfilter,ext.c_str()))
							{
								fi.ctime=statbuf.st_ctime;
								fi.filesize=statbuf.st_size;
								fi.ext=ext;
								//printlog("found %u,%u,%s",fi.ctime,fi.filesize,str.c_str());
								strar.insert(map<string,crtfileinfo>::value_type(str,fi));
							}
						}
					}
				}
			}
			closedir(dp);
			return true;
		}
		return false;
	}
	static bool get_file_listw(const wstring &path,map<wstring,crtfileinfow> &strar,bool recursive=true,const wchar_t *extfilter=NULL) {
		map<string,crtfileinfo> ar;
		bool ret=get_file_listu8(crtucs2u8(path).c_str(),ar,recursive,crtucs2u8(extfilter).c_str());
		map<string,crtfileinfo>::const_iterator it=ar.begin();
		while(it!=ar.end()) {
			it++;
			crtfileinfow w;
			w.ctime=it->second.ctime;
			w.filesize=it->second.filesize;
			w.ext=crtu82ucs(it->second.ext);
			strar.insert(map<wstring,crtfileinfow>::value_type(crtu82ucs(it->first),w));
		}
		return ret;
	}
	static bool unlink_file_or_directoryu8(const char *path)
	{
		struct stat s;
		memset(&s,0,sizeof(struct stat));
		stat(path,&s);
		if (!S_ISDIR(s.st_mode)) return unlink(path)==0;
		DIR *dp;
		struct dirent *dirp;
		struct stat statbuf;
		if ((dp=opendir(path))==NULL) return false;
		while((dirp=readdir(dp))!=NULL)
		{
			if(strcmp(dirp->d_name,".")==0 || strcmp(dirp->d_name,"..")==0) continue;
			string str;
			str=path;str+=CRTPATH_SEPERATOR_STRING;
			str+=dirp->d_name;
			if (stat(str.c_str(),&statbuf)==0)
			{
				if (S_ISDIR(statbuf.st_mode))
					unlink_file_or_directoryu8(str.c_str());
				else
					unlink(str.c_str());
			}
		}
		closedir(dp);
		return rmdir(path)==0;
	}
	static bool unlink_file_or_directoryw(const wchar_t *path) {return unlink_file_or_directoryu8(crtucs2u8(path).c_str());}
#endif
	static bool unlink_fileu8(const char *utf8file) {
	#ifdef _WIN32
		return ::DeleteFileW(crtu82ucs(utf8file).c_str())!=FALSE;
	#else
		return unlink(utf8file)==0;
	#endif
	}
};
