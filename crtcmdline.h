#pragma once
#include "crtlib.h"
#include "crtstring.h"

namespace crtfun {
	#define crtcmdline_hasarg	0x1	//has value behind argv
	#define crtcmdline_need		0x2	//the option must be set
	#define crtcmdline_bool		0x4	//opt is exist or not
	#define crtcmdline_int		0x8
	#define crtcmdline_float	0x10
	#define crtcmdline_string	0x20
	#define crtcmdline_func		0x40
	#define crtcmdline_exit		0x80	//if the option is set,call function and exit(0) e.g. help list
	#define crtcmdline_nbool	0x100	//if opt exist set to false or not
	typedef string (*crtcmdline_callbackfunc)(const char *opt, const char *arg); //return text if failed to parse arg
	typedef struct _crtcmdlineinfo{
		const char *shortname;	// start with - or / , "" for default cmdline parser
		const char *longname;	// start with -- , can be null
		int flags;
		union {
			void *any_arg;
			bool *bool_arg;
			int *int_arg;
			char **str_arg;	//need free if option crtcmdline_string is set
			float *float_arg;
			crtcmdline_callbackfunc func_arg;
		} u;
		const char *help;		//help info for help(), can be null
	}crtcmdlineinfo;
	static crtcmdlineinfo* crtcmdline_findopt(crtcmdlineinfo *infos, const char *key, bool blong) {
		if (!key) return NULL;
		crtcmdlineinfo *ret=infos;
		while(ret->shortname) {
			if (blong && ret->longname && strcmp(ret->longname,key)==0) return ret;
			if (!blong && ret->shortname && strcmp(ret->shortname,key)==0) return ret;
			ret++;
		}
		return NULL;
	}
	//return empty if parse ok, return failed msg if have error
	static string crtcmdline_parse(int argc,const char *argv[],crtcmdlineinfo *infos) {
		int optindex = 1;
		const char *opt;
		const char *arg;
		crtcmdlineinfo *info;
		crtcmdlineinfo *def=crtcmdline_findopt(infos,"",false);
		set<crtcmdlineinfo *> needkey;
		info=infos;
		while(info->shortname) {
			if (info->flags & crtcmdline_need) needkey.insert(info);
			info++;
		}
		while (optindex < argc) {
			bool blong=false;
			opt = argv[optindex++];
			if (*opt=='-' || *opt=='/') {
				if (*opt=='-' && *(opt+1)=='-') {
					blong=true;
					opt++;
				}
				opt++;
				info=crtcmdline_findopt(infos,opt,blong);
				if (info) needkey.erase(info); 
			}
			if (!info && def) info=def;
			if (!info) return string_format("unrecognized option '%s'.\n",opt);
			arg = NULL;
			if (info->flags & crtcmdline_hasarg) {
				arg = argv[optindex++];
				if (!arg) return string_format("missing argument for option '%s'.\n",opt);
			}
			if (info->flags & crtcmdline_bool) {
				*info->u.bool_arg = true;
			} else if (info->flags & crtcmdline_nbool) {
				*info->u.bool_arg = false;
			} else if (info->flags & crtcmdline_string) {
				if (!arg) return string_format("string option '%s' but not set hasarg flag.\n",opt);
				*info->u.str_arg = strdup(arg);
			} else if (info->flags & crtcmdline_int) {
				if (!arg) return string_format("int option '%s' but not set hasarg flag.\n",opt);
				*info->u.int_arg = atoi(arg);
			} else if (info->flags & crtcmdline_float) {
				if (!arg) return string_format("float option '%s' but not set hasarg flag.\n",opt);
				*info->u.float_arg = (float)atof(arg);
			} else if (info->flags & crtcmdline_func){
				string ret=info->u.func_arg(opt,arg);
				if (!ret.empty()) return ret;
			}
			if (info->flags & crtcmdline_exit) exit(0);
		}
		if (needkey.size()>0)
			return string_format("option '%s' must be set.\n",(*(needkey.begin()))->shortname);
		return "";
	}
	static string crtcmdline_printinfo(crtcmdlineinfo *infos) {
		string sret;
		crtcmdlineinfo *ret=infos;
		char buf[64],buf2[65535];
	#ifdef _WIN32
		#define CRTCMDLINE_OPTKEY "/"
	#else
		#define CRTCMDLINE_OPTKEY "-"
	#endif
		while(ret->shortname) {
			if (ret->shortname[0]!='\0') {//default parser not need to print
				sprintf(buf," " CRTCMDLINE_OPTKEY "%s ",ret->shortname);
				if (ret->longname) sprintf(buf+strlen(buf),", --%s",ret->longname);
				sprintf(buf2,"%-20s %s\n",buf,ret->help?ret->help:"");
				sret+=buf2;
			}
			ret++;
		}
		return sret;
	}
	/*
	    ffmpegrun("ffmpeg.exe -ewweh");
	    ffmpegrun("ffmpeg.exe -ewweh －l");
	    ffmpegrun("ffmpeg.exe -eeweweh \"ew\"");
	    ffmpegrun("ffmpeg.exe -eeweweh \"ew\" ");
	    ffmpegrun("ffmpeg.exe -eeweweh \"ew\" \"\"");
	    ffmpegrun("ffmpeg.exe -eeweweh \"e'w\" aa");
	    ffmpegrun("ffmpeg.exe -eweweh  \"\"  ''  \"dew\" dew");
	    ffmpegrun("ffmpeg.exe -ewdewrweh  \"ewd\"  'dew\"dew'  dew");
		cmdline: 2  cmdline[0]:ffmpeg.exe  cmdline[1]:-ewweh
		cmdline: 3  cmdline[0]:ffmpeg.exe  cmdline[1]:-ewweh  cmdline[2]:－l
		cmdline: 3  cmdline[0]:ffmpeg.exe  cmdline[1]:-eeweweh  cmdline[2]:ew
		cmdline: 3  cmdline[0]:ffmpeg.exe  cmdline[1]:-eeweweh  cmdline[2]:ew
		cmdline: 4  cmdline[0]:ffmpeg.exe  cmdline[1]:-eeweweh  cmdline[2]:ew  cmdline[3]:
		cmdline: 4  cmdline[0]:ffmpeg.exe  cmdline[1]:-eeweweh  cmdline[2]:e'w  cmdline[3]:aa
		cmdline: 6  cmdline[0]:ffmpeg.exe  cmdline[1]:-eweweh  cmdline[2]:  cmdline[3]:  cmdline[4]:dew  cmdline[5]:dew
		cmdline: 5  cmdline[0]:ffmpeg.exe  cmdline[1]:-ewdewrweh  cmdline[2]:ewd  cmdline[3]:dew"dew  cmdline[4]:dew
	*/
	static char** crtcmdline_cmdline2ar(const char *cmdline,int &argc)
	{
		if (!cmdline) {argc=0;return NULL;}
		char *pcmdline=strdup(cmdline);
		char *p=pcmdline;
		char **argv;
		int i=0;
		argc=1;
		char sp=0;
		while(*p!='\0') {
			if (sp) {
				if (*p==sp) {
					sp=0;
					*p='\0';
					p++;
					if (*p=='\0') break;
					if (*p==' '){
						while(*p==' ') p++;
						if (*p=='\0') break;
						p--;
					}
					argc++;
				}
			} else {
				if (*p==' ') {
					*p='\0';
					p++;
					if (*p=='\0') break;
					while(*p==' ') p++;
					if (*p=='\0') break;
					p--;
					argc++;
				} else if (*p=='\'' || *p=='"') {
					sp=*p;
				}
			}
			p++;
		}
		argv=(char **)malloc(sizeof(char *)*(argc+1));
		p=pcmdline;
		//the first object use for free
		argv[0]=pcmdline;
		argv++;
		while(i<argc) {
			while (*p=='\'' || *p=='"' || *p==' ') p++;
			argv[i++]=p;
			p+=strlen(p)+1;
		}
		return argv;
	}
	static void crtcmdline_cmdarfree(char **argv)
	{
		if (argv) {
			argv--;
			if (argv[0]) free(argv[0]);
			free(argv);
		}
	}
};
