#pragma once

#include "crtlib.h"
#include "crtstring.h"

namespace crtfun {
	static unsigned long crtget_pid() {
		#ifdef _WIN32
			return GetCurrentProcessId();
		#else
			return getpid();
		#endif
	}
	static unsigned long crtget_tid() {
		#ifdef _WIN32
			return GetCurrentThreadId();
		#else
			return syscall(SYS_gettid);
		#endif
	}
	typedef enum _crtos{
		crtos_windows,
		crtos_linux,
		crtos_mac
	}crtos;
	typedef enum _crtsubos{
		crtsubos_windows_unknown,
		crtsubos_windowsnt,//nt 3.51,nt 4
		crtsubos_windows98,//win 95,win me
		crtsubos_windows2000,
		crtsubos_windows2003,
		crtsubos_windowsxp,
		crtsubos_windowsvista,
		crtsubos_windows2008,
		crtsubos_windows7,
		//crtsubos_windows8,
		crtsubos_linux_unknown,
		crtsubos_linux_centos,
		crtsubos_linux_ubuntu,
		crtsubos_linux_redhat,
		crtsubos_linux_opensuse,
		crtsubos_linux_debian,
		crtsubos_linux_gentoo,
		crtsubos_mac_unknown,
		crtsubos_mac_10_6,
		crtsubos_mac_10_7,
		crtsubos_mac_10_8
	}crtsubos;
	typedef enum _crtcpu{
		crtcpu_unknown,
		crtcpu_intel,
		crtcpu_amd,
		crtcpu_arm,
		crtcpu_mips
	}crtcpu;
	typedef enum _crtcpubit{
		crtcpubit_unknown,
		crtcpubit_32,
		crtcpubit_64
	}crtcpubit;
	static crtos getos() {
	#ifdef _WIN32
		return crtos_windows;
	#elif defined(__APPLE__)
		return crtos_mac;
	#else
		return crtos_linux;
	#endif
	}
	static crtsubos getsubos() {
	#ifdef _WIN32
		crtsubos subos=crtsubos_windows_unknown;
		OSVERSIONINFOEX os={0};
		os.dwOSVersionInfoSize=sizeof(OSVERSIONINFOEX);
		::GetVersionEx((OSVERSIONINFO *)&os);
		int BuildNumber=os.dwBuildNumber&0xffff;
		if (os.dwMajorVersion==3 || os.dwMajorVersion==4) {
			if (os.dwPlatformId==VER_PLATFORM_WIN32_NT) subos=crtsubos_windowsnt;
			else subos=crtsubos_windows98;
		} else if (os.dwMajorVersion==5) {
			if (os.dwMinorVersion==0) subos=crtsubos_windows2000;
			else if (os.dwMinorVersion==1) subos=crtsubos_windowsxp;
			else if (os.dwMinorVersion==2) {
				if (os.wProductType==VER_NT_WORKSTATION) subos=crtsubos_windowsxp;
				else subos=crtsubos_windows2003;
			}
		} else if (os.dwMajorVersion==6) {
			if (os.dwMinorVersion==0) {
				if(os.wProductType==VER_NT_WORKSTATION) subos=crtsubos_windowsvista;
				else subos=crtsubos_windows2008;
			} else if (os.dwMinorVersion==1) {
				if (os.wProductType==VER_NT_WORKSTATION) subos=crtsubos_windows7;
				else subos=crtsubos_windows2008;
			}
		}
		return subos;
	#elif defined(__APPLE__)
		return crtsubos_mac_unknown;
	#else
		return crtsubos_linux_unknown;
	#endif
	}
	static string getniceosname() {
	#ifdef _WIN32
		#ifndef SM_SERVERR2
			#define SM_SERVERR2 89
		#endif
		#ifndef VER_SUITE_STORAGE_SERVER
			#define VER_SUITE_STORAGE_SERVER 0x00002000
		#endif
		#ifndef VER_SUITE_COMPUTE_SERVER
			#define VER_SUITE_COMPUTE_SERVER 0x00004000
		#endif
		OSVERSIONINFOEX os={0};
		os.dwOSVersionInfoSize=sizeof(OSVERSIONINFOEX);
		::GetVersionEx((OSVERSIONINFO *)&os);
		string ret="Unknown";
		int BuildNumber=os.dwBuildNumber&0xffff;
		if (os.dwMajorVersion==3 && os.dwMinorVersion==51) ret="Windows NT 3.51";
		else if (os.dwMajorVersion==4) {
			if (os.dwMinorVersion==0) {
				if (os.dwPlatformId==VER_PLATFORM_WIN32_NT) ret="Windows NT 4.0";
				else ret="Windows 95";
			} else if (os.dwMinorVersion==10) {
				ret="Windows 98";
				if (BuildNumber==2222) ret+=" SE";
			} else if (os.dwMinorVersion==90) ret="Windows ME";
		} else if (os.dwMajorVersion==5) {
			if (os.dwMinorVersion==0) {
				ret="Windows 2000";
				if (os.wSuiteMask==VER_SUITE_ENTERPRISE) ret+=" Advanced Server";
			} else if (os.dwMinorVersion==1) {
				ret="Windows XP";
				if (os.wSuiteMask==VER_SUITE_EMBEDDEDNT) ret+=" Embedded";
				else if(os.wSuiteMask==VER_SUITE_PERSONAL) ret+=" Home Edition";
				else ret+=" Professional";
			} else if (os.dwMinorVersion==2) {
				if (os.wProductType==VER_NT_WORKSTATION) {
					ret="Windows XP";
					if (os.wSuiteMask==VER_SUITE_EMBEDDEDNT) ret+=" Embedded";
					else if(os.wSuiteMask==VER_SUITE_PERSONAL) ret+=" Home Edition";
					else ret+=" Professional";
				} else {
					if(GetSystemMetrics(SM_SERVERR2)==0) {
						ret="Windows 2003";
						if (os.wSuiteMask==VER_SUITE_BLADE) ret+=" Web Edition";
						else if (os.wSuiteMask==VER_SUITE_COMPUTE_SERVER) ret+=" Compute Cluster Edition";
						else if (os.wSuiteMask==VER_SUITE_STORAGE_SERVER) ret+=" Storage Server";
						else if (os.wSuiteMask==VER_SUITE_DATACENTER) ret+=" Datacenter Edition";
						else if (os.wSuiteMask==VER_SUITE_ENTERPRISE) ret+=" Enterprise Edition";
					} else {
						ret="Windows Server 2003 R2";
						if (os.wSuiteMask==VER_SUITE_STORAGE_SERVER) ret+="Storage Server";
					}
				}
			}
		} else if (os.dwMajorVersion==6) {
			if (os.dwMinorVersion==0) {
				if(os.wProductType==VER_NT_WORKSTATION) {
					ret="Windows Vista";
					if (os.wSuiteMask==VER_SUITE_PERSONAL) ret+=" Home";
				} else {
					ret="Windows Server 2008";
					if (os.wSuiteMask==VER_SUITE_DATACENTER) ret+=" Datacenter Server";
					else if(os.wSuiteMask==VER_SUITE_ENTERPRISE) ret+=" Enterprise";
				}
			} else if (os.dwMinorVersion==1) {
				if (os.wProductType==VER_NT_WORKSTATION) ret="Windows 7";
				else ret="Windows Server 2008 R2";
			}
		}
		if (strlen(os.szCSDVersion)>0) {
			if (string_compare_nocase(os.szCSDVersion,"A")) ret+=" SE";
			else if (string_compare_nocase(os.szCSDVersion,"C")) ret+=" OSR2";
			else ret+=string(" ")+os.szCSDVersion;
		}
		ret+=string_format("(%d.%d.%d)",os.dwMajorVersion,os.dwMinorVersion,BuildNumber);
		return ret;
	#else
		return "";
	#endif
	}
	static bool crtosiswinnt() {
	#ifdef _WIN32
		return GetVersion()<0x80000000;
	#else
		return false;
	#endif
	}
	static string crthostname() {
	#ifdef _WIN32
		char buf[1024];
		DWORD Size=1024;
		::GetComputerNameA(buf,&Size);
		return buf;
	#else
		return "";
	#endif
	}
	static string crtusername() {
	#ifdef _WIN32
		char buf[1024];
		DWORD Size=1024;
		::GetUserNameA(buf,&Size);
		return buf;
	#else
		return "";
	#endif
	}
	static void crtsrand() {
		unsigned long tid=crtget_tid();
		::srand((unsigned int)(time(NULL)+tid+rand()));
	}
};
