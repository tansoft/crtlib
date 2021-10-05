#pragma once

#include "crtlib.h"
#include "crtstring.h"
#include "deelx.h"

namespace crtfun {
	class crtregex{
	public:
		crtregex(){exp=NULL;}
		crtregex(const char *regex) {init(regex);}
		void init(const char *regex) {
			if (exp) delete exp;
			exp=new CRegexpA(regex);
		}
		virtual ~crtregex(){
			if (exp) {
				delete exp;
				exp=NULL;
			}
		}
		//cached regex object for improve performance
		bool ismatched(const char *text) {
			if (!exp) return false;
			return exp->Match(text).IsMatched()!=0;
		}
		static bool ismatched(const char *text,const char *regex){
			CRegexpA regexp(regex);
			return regexp.Match(text).IsMatched()!=0;
		}
		//cached regex object for improve performance
		bool iswhole_matched(const char *text) {
			if (!exp) return false;
			return exp->MatchExact(text).IsMatched()!=0;
		}
		static bool iswhole_matched(const char *text,const char *regex){
			CRegexpA regexp(regex);
			return regexp.MatchExact(text).IsMatched()!=0;
		}
		static bool ismatched_email(const char *text) {return iswhole_matched(text,"^([0-9a-zA-Z]([-.\\w]*[0-9a-zA-Z])*@(([0-9a-zA-Z])+([-\\w]*[0-9a-zA-Z])*\\.)+[a-zA-Z]{2,9})$");}
		static string match_one(const char *text,const char *regex){
			string ret;
			CRegexpA regexp(regex);
			MatchResult result = regexp.Match(text);
			if(result.IsMatched()) {
				int start = result.GetStart();
				int end = result.GetEnd();
				ret.assign(text+start, end - start);
				//crtdebug("[REGEXP]Matched at %d - %d %s\n",start,end,ret.c_str());
			}
			return ret;
		}
		static vector<string> match_all(const char *text,const char *regex){
			vector<string> ret;
			string tmp;
			CRegexpA regexp(regex);
			MatchResult result = regexp.Match(text);
			while(result.IsMatched()) {
				int start = result.GetStart();
				int end = result.GetEnd();
				tmp.assign(text+start, end - start);
				//crtdebug("[REGEXP][%d]Matched at %d - %d %s\n",(int)ret.size(),start,end,tmp.c_str());
				ret.push_back(tmp);
				result = regexp.Match(text, result.GetEnd());
			}
			return ret;
		}
		static string match_one_bytoken(const char *content,const char *start,const char *end) {
			const char *key1;
			if (!start) key1=content;
			else key1=strstr(content,start);
			string ret;
			if (key1) {
				key1+=start?strlen(start):0;
				if (!end) ret=key1;
				else {
					const char *key2=strstr(key1,end);
					if (key2) ret.assign(key1,key2-key1);
				}
			}
			return ret;
		}
	protected:
		CRegexpA *exp;
	};
	class crthttpuatest{
	public:
		crthttpuatest(){reset();}
		virtual ~crthttpuatest(){}
		void init(const char *useragent) {
			reset();
			string agent=useragent;
			string_tolower(agent);
			version=crtregex::match_one(agent.c_str(),"(?<=.+(rv|it|ra|ie)[\\/: ])([\\d.]+)");
			dversion=atof(version.c_str());
			safari=agent.find("webkit")!=string::npos;
			opera=agent.find("opera")!=string::npos;
			msie=(agent.find("msie")!=string::npos && !opera);
			mozilla=(agent.find("mozilla")!=string::npos && !(agent.find("compatible")!=string::npos || agent.find("webkit")!=string::npos));
			chrome=(agent.find("chrome")!=string::npos);
			if (chrome) {safari=false;mozilla=false;}
			firefox=(agent.find("firefox")!=string::npos);
			if (firefox) {mozilla=false;}
			other=!(msie||safari||opera||mozilla||chrome||firefox);
		}
		void reset(){
			msie=false;
			safari=false;
			opera=false;
			mozilla=false;
			chrome=false;
			firefox=false;
			other=true;
			version.clear();
			dversion=0;
		}
		string tostring() {
			return string_format("ver:%s (%g) msie:%d safari:%d opera:%d mozilla:%d firefox:%d chrome:%d other:%d",
				version.c_str(),dversion,msie,safari,opera,mozilla,firefox,chrome,other);
		}
		inline bool is_support_digest_auth() {return (safari||firefox||chrome||(msie && dversion>6.0));}
		inline bool isie6() {return msie && version=="6.0";}
		string version;
		double dversion;
		bool msie;
		bool safari;
		bool opera;
		bool mozilla;//Netscape
		bool firefox;
		bool chrome;
		bool other;
	};
};
