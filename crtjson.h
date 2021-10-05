#pragma once

#include "crtlib.h"
#include "crtstring.h"
#include "crtcharset.h"
#include <float.h>
#include <math.h>

//imp from cJSON library
//how to use in crttestcase.h testcase_crtjson() 

namespace crtfun {
	typedef enum _crtjsontype{
		crtjsontype_false,
		crtjsontype_true,
		crtjsontype_null,
		crtjsontype_number,
		crtjsontype_string,
		crtjsontype_array,
		crtjsontype_object,
		crtjsontype_reference=256
	}crtjsontype;
	typedef struct _crtjson {
		struct _crtjson *next,*prev;
		struct _crtjson *child;
		int type;
		char *valuestring;
		int valueint;
		double valuedouble;
		char *string;		// the object key
	} crtjson;
	class crtjsonparser{
	public:
		crtjsonparser(){ep=NULL;crtjson_malloc=::malloc;crtjson_free=::free;}
		virtual ~crtjsonparser(){}
		crtjson *parse(const char *value){
			crtjson *c=new_json();
			ep=0;
			if (!c) return 0;
			if (!parse_value(c,skip(value))) {delete_json(c);return 0;}
			return c;
		}
		const char *get_last_error(){return ep;}
		void manage_memory_alloc(void *(*newmalloc)(size_t sz),void (*newfree)(void *ptr))
		{
			crtjson_malloc = newmalloc?newmalloc:(::malloc);
			crtjson_free = newfree?newfree:(::free);
		}
		void delete_json(crtjson *c)
		{
			crtjson *next;
			while(c)
			{
				next=c->next;
				if (!(c->type& crtjsontype_reference) && c->child) delete_json(c->child);
				if (!(c->type& crtjsontype_reference) && c->valuestring) crtjson_free(c->valuestring);
				if (c->string) crtjson_free(c->string);
				crtjson_free(c);
				c=next;
			}
		}
		double finddouble(crtjson *item, const char* key) {
			item = find(item, key);
			if (!item) return 0;
			return atof(tostring(item).c_str());
		}
		int findint(crtjson *item, const char* key) {
			item = find(item, key);
			if (!item) return 0;
			return atoi(tostring(item).c_str());
		}
		bool findbool(crtjson *item, const char* key) {
			item = find(item, key);
			if (!item) return false;
			switch ((item->type)&255) {
				case crtjsontype_true: return true;
				case crtjsontype_number: return atoi(tostring(item).c_str())!=0;
			}
			return false;
		}
		string findstr(crtjson *item, const char* key) {return tostring(find(item,key));}
		//支持 . [] 查找 e.g.: a.b    a[1].b = a.[1].b = a.1.b   [1].c
		crtjson *find(crtjson *item, const char* key) {
			if (!key) return NULL;
			crtstringtoken token(key,".");
			while (item) {
				if (!token.ismore()) break;
				string key=token.nexttoken();
				string idx;
				size_t pos=key.find('[');
				if (pos!=string::npos) {
					idx=key.substr(pos+1);
					key=key.substr(0,pos);
				}
				string_trim(key);
				string_trim(idx);
				if (!key.empty()) {
					if (((item->type)&255)==crtjsontype_object)
						item=objectat(item,key.c_str());
					else if (((item->type)&255)==crtjsontype_array)
						item=arrayat(item,atoi(key.c_str()));
					else return NULL;
				}
				if (!item) break;
				if (!idx.empty()) {
					if (((item->type)&255)==crtjsontype_array)
						item=arrayat(item,atoi(idx.c_str()));
					else return NULL;
				}
			}
			return item;
		}
		//0 for json text have ustring encoded, 1 for nice text for display
		string tostring(crtjson *item,int formatted=1) {
			string ret;
			if (item) {
				char *out;
				//if type is string, not need to add ""
				if (((item->type)&255)==crtjsontype_string)
					out=print_string(item,0,formatted);
				else
					out=print_value(item,0,formatted);
				if (out) {
					ret = out;
					crtjson_free(out);
				}
			}
			return ret;
		}
		int arraysize(crtjson *array) {crtjson *c=array->child;int i=0;while(c) i++,c=c->next;return i;}
		crtjson *arrayat(crtjson *array,int item) {crtjson *c=array->child; while (c && item>0) item--,c=c->next; return c;}
		crtjson *firstitem(crtjson *item) {if (item) item=item->child; return item;}
		crtjson *nextitem(crtjson *item) {if (item) item=item->next; return item;}
		string itemkey(crtjson *item) {if (item) return item->string;return "";}
		bool isempty(crtjson *item) {
			if (!item) return true;
			if (!item->child) return true;
			return false;
		}
		crtjson *objectat(crtjson *object,const char *string) {
			crtjson *c=object->child;
			while (c && crtjson_strcasecmp(c->string,string)) c=c->next;
			return c;
		}
		int objectatint(crtjson *object,const char *string)
		{
			int ret=0;
			crtjson *obj=objectat(object,string);
			if (obj && ((obj->type)&255)==crtjsontype_number)
			{
				char *out=print_number(obj);
				if (out)
				{
					ret=(int)atof(out);
					crtjson_free(out);
				}
			}
			return ret;
		}
		crtjson *createnull() {crtjson *item=new_json();if(item)item->type=crtjsontype_null;return item;}
		crtjson *createtrue() {crtjson *item=new_json();if(item)item->type=crtjsontype_true;return item;}
		crtjson *createfalse() {crtjson *item=new_json();if(item)item->type=crtjsontype_false;return item;}
		crtjson *createbool(int b) {crtjson *item=new_json();if(item)item->type=b?crtjsontype_true:crtjsontype_false;return item;}
		crtjson *createnumber(double num) {
			crtjson *item=new_json();
			if(item){
				item->type=crtjsontype_number;
				item->valuedouble=num;
				item->valueint=(int)num;
			}
			return item;
		}
		crtjson *createstring(const char *string) {
			crtjson *item=new_json();
			if(item){
				item->type= crtjsontype_string;
				item->valuestring=crtjson_strdup(string);
			}
			return item;
		}
		crtjson *createarray() {crtjson *item=new_json();if(item)item->type=crtjsontype_array;return item;}
		crtjson *createobject() {crtjson *item=new_json();if(item)item->type=crtjsontype_object;return item;}
		crtjson *createintarray(int *numbers,int count) {
			int i;crtjson *n=0,*p=0,*a=createarray();
			for(i=0;a && i<count;i++){
				n=createnumber(numbers[i]);
				if(!i)a->child=n;else suffix_object(p,n);p=n;
			}
			return a;
		}
		crtjson *createfloatarray(float *numbers,int count) {
			int i;crtjson *n=0,*p=0,*a=createarray();
			for(i=0;a && i<count;i++){
				n=createnumber(numbers[i]);
				if(!i)a->child=n;else suffix_object(p,n);p=n;
			}
			return a;
		}
		crtjson *createdoublearray(double *numbers,int count) {
			int i;crtjson *n=0,*p=0,*a=createarray();
			for(i=0;a && i<count;i++){
				n=createnumber(numbers[i]);
				if(!i)a->child=n;else suffix_object(p,n);p=n;
			}
			return a;
		}
		crtjson *createstringarray(const char **strings,int count) {
			int i;crtjson *n=0,*p=0,*a=createarray();
			for(i=0;a && i<count;i++){
				n=createstring(strings[i]);
				if(!i)a->child=n;else suffix_object(p,n);
				p=n;
			}
			return a;
		}
		void arrayadd(crtjson *array, crtjson *item) {
			crtjson *c=array->child;if (!item) return;
			if (!c) {array->child=item;} else {
				while (c && c->next) c=c->next;
				suffix_object(c,item);
			}
		}
		void arrayadd_null(crtjson *object) {return arrayadd(object,createnull());}
		void arrayadd_true(crtjson *object) {return arrayadd(object,createtrue());}
		void arrayadd_false(crtjson *object) {return arrayadd(object,createfalse());}
		void arrayadd_number(crtjson *object,double n) {return arrayadd(object,createnumber(n));}
		void arrayadd_string(crtjson *object,const char *s) {return arrayadd(object,createstring(s));}
		//当增加的是另一个存在的json对象时使用
		void arrayadd_reference(crtjson *array, crtjson *item) {arrayadd(array,create_reference(item));}
		void arraydeletejson(crtjson *array,int which) {delete_json(arraydelete(array,which));}
		void arrayreplace(crtjson *array,int which,crtjson *newitem)
		{
			crtjson *c=array->child;while (c && which>0) c=c->next,which--;
			if (!c) return;newitem->next=c->next;newitem->prev=c->prev;
			if (newitem->next) newitem->next->prev=newitem;
			if (c==array->child) array->child=newitem;
			else newitem->prev->next=newitem;
			c->next=c->prev=0;delete_json(c);
		}
		void arrayappend(crtjson *from,crtjson *to) {
			crtjson *first=firstitem(from);
			while(first) {
				arrayadd(to,duplicate_json(first));
				first=nextitem(first);
			}
		}
		void objectadd(crtjson *object,const char *name,crtjson *item) {
			if (!item) return; if (item->string) crtjson_free(item->string);
			item->string=crtjson_strdup(name);arrayadd(object,item);
		}
		//当增加的是另一个存在的json对象时使用
		void objectadd_reference(crtjson *object,const char *string,crtjson *item) {objectadd(object,string,create_reference(item));}
		void objectadd_null(crtjson *object,const char *name) {return objectadd(object,name,createnull());}
		void objectadd_true(crtjson *object,const char *name) {return objectadd(object,name,createtrue());}
		void objectadd_false(crtjson *object,const char *name) {return objectadd(object,name,createfalse());}
		void objectadd_number(crtjson *object,const char *name,double n) {return objectadd(object,name,createnumber(n));}
		void objectadd_string(crtjson *object,const char *name,const char *s) {return objectadd(object,name,createstring(s));}
		crtjson *arraydelete(crtjson *array,int which) {
			crtjson *c=array->child;while (c && which>0) c=c->next,which--;
			if (!c) return 0;if (c->prev) c->prev->next=c->next;
			if (c->next) c->next->prev=c->prev;if (c==array->child) array->child=c->next;
			c->prev=c->next=0;return c;
		}
		crtjson *objectdelete(crtjson *object,const char *string) {
			int i=0;crtjson *c=object->child;
			while (c && crtjson_strcasecmp(c->string,string)) i++,c=c->next;
			if (c) return arraydelete(object,i);
			return 0;
		}
		void objectdeletejson(crtjson *object,const char *string) {delete_json(objectdelete(object,string));}
		void objectreplace(crtjson *object,const char *string,crtjson *newitem)
		{
			int i=0;crtjson *c=object->child;
			while(c && crtjson_strcasecmp(c->string,string))i++,c=c->next;
			if(c){newitem->string=crtjson_strdup(string);
			arrayreplace(object,i,newitem);}
		}
	protected:
		crtjson *new_json()
		{
			crtjson* node = (crtjson*)crtjson_malloc(sizeof(crtjson));
			if (node) memset(node,0,sizeof(crtjson));
			return node;
		}
		static void suffix_object(crtjson *prev,crtjson *item) {prev->next=item;item->prev=prev;}
		static int crtjson_strcasecmp(const char *s1,const char *s2)
		{
			if (!s1) return (s1==s2)?0:1;if (!s2) return 1;
			for(; tolower(*s1) == tolower(*s2); ++s1, ++s2)	if(*s1 == 0) return 0;
			return tolower(*(const unsigned char *)s1) - tolower(*(const unsigned char *)s2);
		}
		char* crtjson_strdup(const char* str)
		{
		      size_t len;
		      char* copy;
		      len = strlen(str) + 1;
		      if (!(copy = (char*)crtjson_malloc(len))) return 0;
		      memcpy(copy,str,len);
		      return copy;
		}
		static const char *skip(const char *in) {while (in && *in && (unsigned char)*in<=32) in++; return in;}
		static const char *parse_number(crtjson *item,const char *num)
		{
			double n=0,sign=1,scale=0;int subscale=0,signsubscale=1;
			/* Could use sscanf for this? */
			if (*num=='-') sign=-1,num++;	/* Has sign? */
			if (*num=='0') num++;			/* is zero */
			if (*num>='1' && *num<='9')	do	n=(n*10.0)+(*num++ -'0');	while (*num>='0' && *num<='9');	/* Number? */
			if (*num=='.') {num++;		do	n=(n*10.0)+(*num++ -'0'),scale--; while (*num>='0' && *num<='9');}	/* Fractional part? */
			if (*num=='e' || *num=='E')		/* Exponent? */
			{	num++;if (*num=='+') num++;	else if (*num=='-') signsubscale=-1,num++;		/* With sign? */
				while (*num>='0' && *num<='9') subscale=(subscale*10)+(*num++ - '0');	/* Number? */
			}
			n=sign*n*pow(10.0,(scale+subscale*signsubscale));	/* number = +/- number.fraction * 10^+/- exponent */
			item->valuedouble=n;
			item->valueint=(int)n;
			item->type=crtjsontype_number;
			return num;
		}
		const char *parse_string(crtjson *item,const char *str)
		{
			static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
			const char *ptr=str+1;char *ptr2;char *out;int len=0;unsigned uc,uc2;
			if (*str!='\"') {ep=str;return 0;}	/* not a string! */
			while (*ptr!='\"' && *ptr && ++len) if (*ptr++ == '\\') ptr++;	/* Skip escaped quotes. */			
			out=(char*)crtjson_malloc(len+1);	/* This is how long we need for the string, roughly. */
			if (!out) return 0;
			ptr=str+1;ptr2=out;
			while (*ptr!='\"' && *ptr)
			{
				if (*ptr!='\\') *ptr2++=*ptr++;
				else
				{
					ptr++;
					switch (*ptr)
					{
						case 'b': *ptr2++='\b';	break;
						case 'f': *ptr2++='\f';	break;
						case 'n': *ptr2++='\n';	break;
						case 'r': *ptr2++='\r';	break;
						case 't': *ptr2++='\t';	break;
/*						case 'u':	 / * transcode utf16 to utf8. DOES NOT SUPPORT SURROGATE PAIRS CORRECTLY. * /
							sscanf(ptr+1,"%4x",&uc);	/ * get the unicode char. * /
							len=3;if (uc<0x80) len=1;else if (uc<0x800) len=2;ptr2+=len;
							
							switch (len) {
								case 3: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
								case 2: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
								case 1: *--ptr2 =(uc | firstByteMark[len]);
							}
							ptr2+=len;ptr+=4;
							break;
*/
						case 'u':	 /* transcode utf16 to utf8. */
							sscanf(ptr+1,"%4x",&uc);ptr+=4;	/* get the unicode char. */
							if ((uc>=0xDC00 && uc<=0xDFFF) || uc==0)	break;	// check for invalid.
							if (uc>=0xD800 && uc<=0xDBFF)	// UTF16 surrogate pairs.
							{
								if (ptr[1]!='\\' || ptr[2]!='u')	break;	// missing second-half of surrogate.
								sscanf(ptr+3,"%4x",&uc2);ptr+=6;
								if (uc2<0xDC00 || uc2>0xDFFF)		break;	// invalid second-half of surrogate.
								uc=0x10000 | ((uc&0x3FF)<<10) | (uc2&0x3FF);
							}
							len=4;if (uc<0x80) len=1;else if (uc<0x800) len=2;else if (uc<0x10000) len=3; ptr2+=len;
							switch (len) {
								case 4: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
								case 3: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
								case 2: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
								case 1: *--ptr2 =(uc | firstByteMark[len]);
							}
							ptr2+=len;
							break;
						default:  *ptr2++=*ptr; break;
					}
					ptr++;
				}
			}
			*ptr2=0;
			if (*ptr=='\"') ptr++;
			item->valuestring=out;
			item->type= crtjsontype_string;
			return ptr;
		}
		char *print_number(crtjson *item)
		{
			char *str;
			double d=item->valuedouble;
			if (fabs(((double)item->valueint)-d)<=DBL_EPSILON && d<=INT_MAX && d>=INT_MIN)
			{
				str=(char*)crtjson_malloc(21);	/* 2^64+1 can be represented in 21 chars. */
				if (str) sprintf(str,"%d",item->valueint);
			}
			else
			{
				str=(char*)crtjson_malloc(64);	/* This is a nice tradeoff. */
				if (str)
				{
					if (fabs(floor(d)-d)<=DBL_EPSILON)			sprintf(str,"%.0f",d);
					else if (fabs(d)<1.0e-6 || fabs(d)>1.0e9)	sprintf(str,"%e",d);
					else										sprintf(str,"%f",d);
				}
			}
			return str;
		}
		char *print_string(crtjson *item, int addquote=1, int fmt=1) {return print_string_ptr(item->valuestring, addquote, fmt);}
		char *print_string_ptr(const char *str, int addquote=1, int fmt=1)
		{
			const char *ptr;char *ptr2,*out;int len=0;unsigned char token;
			if (!str) return crtjson_strdup("");
			ptr=str;
			if (!fmt) len=(int)(strlen(str)*3+3);
			else
				while ((token=*ptr) && ++len) {if (strchr("\"\\\b\f\n\r\t",token)) len++; else if (token<32) len+=5;ptr++;}
			out=(char*)crtjson_malloc(len+3);
			if (!out) return 0;
			ptr2=out;ptr=str;
			if (addquote) *ptr2++='\"';
			if (!fmt) {
				vector<uint16_t> tmp;
				if (crtu82u16(ptr,tmp)) {
					for(size_t i=0;i<tmp.size();i++) {
						uint16_t t=tmp[i];
						if (t<128 && t>31 && t!='\"' && t!='\\')
							*ptr2++=(char)t;
						else {
							*ptr2++='\\';
							switch (t)
							{
								case '\\':	*ptr2++='\\';	break;
								case '\"':	*ptr2++='\"';	break;
								case '\b':	*ptr2++='b';	break;
								case '\f':	*ptr2++='f';	break;
								case '\n':	*ptr2++='n';	break;
								case '\r':	*ptr2++='r';	break;
								case '\t':	*ptr2++='t';	break;
								default: sprintf(ptr2,"u%04x",t);ptr2+=5;	break;	/* escape and print */
							}
						}
					}
				}
			} else {
				while (*ptr)
				{
					if ((unsigned char)*ptr>31 && *ptr!='\"' && *ptr!='\\') *ptr2++=*ptr++;
					else
					{
						*ptr2++='\\';
						switch (token=*ptr++)
						{
							case '\\':	*ptr2++='\\';	break;
							case '\"':	*ptr2++='\"';	break;
							case '\b':	*ptr2++='b';	break;
							case '\f':	*ptr2++='f';	break;
							case '\n':	*ptr2++='n';	break;
							case '\r':	*ptr2++='r';	break;
							case '\t':	*ptr2++='t';	break;
							default: sprintf(ptr2,"u%04x",token);ptr2+=5;	break;	/* escape and print */
						}
					}
				}
			}
			if (addquote) *ptr2++='\"';
			*ptr2++=0;
			return out;
		}
		const char *parse_value(crtjson *item,const char *value)
		{
			if (!value)						return 0;	/* Fail on null. */
			if (!strncmp(value,"null",4))	{ item->type=crtjsontype_null;  return value+4; }
			if (!strncmp(value,"false",5))	{ item->type=crtjsontype_false; return value+5; }
			if (!strncmp(value,"true",4))	{ item->type=crtjsontype_true; item->valueint=1;	return value+4; }
			if (*value=='\"')				{ return parse_string(item,value); }
			if (*value=='-' || (*value>='0' && *value<='9'))	{ return parse_number(item,value); }
			if (*value=='[')				{ return parse_array(item,value); }
			if (*value=='{')				{ return parse_object(item,value); }
		
			ep=value;return 0;	/* failure. */
		}
		char *print_value(crtjson *item,int depth,int fmt)
		{
			char *out=0;
			if (!item) return 0;
			switch ((item->type)&255)
			{
				case crtjsontype_null:	out=crtjson_strdup("null");	break;
				case crtjsontype_false:	out=crtjson_strdup("false");break;
				case crtjsontype_true:	out=crtjson_strdup("true"); break;
				case crtjsontype_number:	out=print_number(item);break;
				case  crtjsontype_string:	out=print_string(item,1,fmt);break;
				case  crtjsontype_array:	out=print_array(item,depth,fmt);break;
				case crtjsontype_object:	out=print_object(item,depth,fmt);break;
			}
			return out;
		}
		const char *parse_array(crtjson *item,const char *value)
		{
			crtjson *child;
			if (*value!='[')	{ep=value;return 0;}	/* not an array! */
			item->type= crtjsontype_array;
			value=skip(value+1);
			if (*value==']') return value+1;	/* empty array. */
			item->child=child=new_json();
			if (!item->child) return 0;		 /* memory fail */
			value=skip(parse_value(child,skip(value)));	/* skip any spacing, get the value. */
			if (!value) return 0;
			while (*value==',')
			{
				crtjson *new_item;
				if (!(new_item=new_json())) return 0; 	/* memory fail */
				child->next=new_item;new_item->prev=child;child=new_item;
				value=skip(parse_value(child,skip(value+1)));
				if (!value) return 0;	/* memory fail */
			}
			if (*value==']') return value+1;	/* end of array */
			ep=value;return 0;	/* malformed. */
		}
		char *print_array(crtjson *item,int depth,int fmt)
		{
			char **entries;
			char *out=0,*ptr,*ret;int len=5;
			crtjson *child=item->child;
			int numentries=0,i=0,fail=0;
			/* How many entries in the array? */
			while (child) numentries++,child=child->next;
			/* Allocate an array to hold the values for each */
			entries=(char**)crtjson_malloc(numentries*sizeof(char*));
			if (!entries) return 0;
			memset(entries,0,numentries*sizeof(char*));
			/* Retrieve all the results: */
			child=item->child;
			while (child && !fail)
			{
				ret=print_value(child,depth+1,fmt);
				entries[i++]=ret;
				if (ret) len+=(int)strlen(ret)+2+(fmt?1:0); else fail=1;
				child=child->next;
			}
			/* If we didn't fail, try to malloc the output string */
			if (!fail) out=(char*)crtjson_malloc(len);
			/* If that fails, we fail. */
			if (!out) fail=1;
			/* Handle failure. */
			if (fail)
			{
				for (i=0;i<numentries;i++) if (entries[i]) crtjson_free(entries[i]);
				crtjson_free(entries);
				return 0;
			}
			/* Compose the output array. */
			*out='[';
			ptr=out+1;*ptr=0;
			for (i=0;i<numentries;i++)
			{
				strcpy(ptr,entries[i]);ptr+=strlen(entries[i]);
				if (i!=numentries-1) {*ptr++=',';if(fmt)*ptr++=' ';*ptr=0;}
				crtjson_free(entries[i]);
			}
			crtjson_free(entries);
			*ptr++=']';*ptr++=0;
			return out;	
		}
		const char *parse_object(crtjson *item,const char *value)
		{
			crtjson *child;
			if (*value!='{') {ep=value;return 0;}	/* not an object! */
			item->type=crtjsontype_object;
			value=skip(value+1);
			if (*value=='}') return value+1;	/* empty array. */
			item->child=child=new_json();
			if (!item->child) return 0;
			value=skip(parse_string(child,skip(value)));
			if (!value) return 0;
			child->string=child->valuestring;child->valuestring=0;
			if (*value!=':') {ep=value;return 0;}	/* fail! */
			value=skip(parse_value(child,skip(value+1)));	/* skip any spacing, get the value. */
			if (!value) return 0;
			while (*value==',')
			{
				crtjson *new_item;
				if (!(new_item=new_json()))	return 0; /* memory fail */
				child->next=new_item;new_item->prev=child;child=new_item;
				value=skip(parse_string(child,skip(value+1)));
				if (!value) return 0;
				child->string=child->valuestring;child->valuestring=0;
				if (*value!=':') {ep=value;return 0;}	/* fail! */
				value=skip(parse_value(child,skip(value+1)));	/* skip any spacing, get the value. */
				if (!value) return 0;
			}
			if (*value=='}') return value+1;	/* end of array */
			ep=value;return 0;	/* malformed. */
		}
		char *print_object(crtjson *item,int depth,int fmt)
		{
			char **entries=0,**names=0;
			char *out=0,*ptr,*ret,*str;int len=7,i=0,j;
			crtjson *child=item->child;
			int numentries=0,fail=0;
			/* Count the number of entries. */
			while (child) numentries++,child=child->next;
			/* Allocate space for the names and the objects */
			entries=(char**)crtjson_malloc(numentries*sizeof(char*));
			if (!entries) return 0;
			names=(char**)crtjson_malloc(numentries*sizeof(char*));
			if (!names) {crtjson_free(entries);return 0;}
			memset(entries,0,sizeof(char*)*numentries);
			memset(names,0,sizeof(char*)*numentries);
			child=item->child;depth++;if (fmt) len+=depth;
			while (child)
			{
				names[i]=str=print_string_ptr(child->string,1,fmt);
				entries[i++]=ret=print_value(child,depth,fmt);
				if (str && ret) len+=(int)strlen(ret)+(int)strlen(str)+2+(fmt?2+depth:0); else fail=1;
				child=child->next;
			}
			if (!fail) out=(char*)crtjson_malloc(len);
			if (!out) fail=1;
			if (fail)
			{
				for (i=0;i<numentries;i++) {if (names[i]) crtjson_free(names[i]);if (entries[i]) crtjson_free(entries[i]);}
				crtjson_free(names);crtjson_free(entries);
				return 0;
			}
			*out='{';ptr=out+1;if (fmt)*ptr++='\n';*ptr=0;
			for (i=0;i<numentries;i++)
			{
				if (fmt) for (j=0;j<depth;j++) *ptr++='\t';
				strcpy(ptr,names[i]);ptr+=strlen(names[i]);
				*ptr++=':';if (fmt) *ptr++='\t';
				strcpy(ptr,entries[i]);ptr+=strlen(entries[i]);
				if (i!=numentries-1) *ptr++=',';
				if (fmt) *ptr++='\n';*ptr=0;
				crtjson_free(names[i]);crtjson_free(entries[i]);
			}
			crtjson_free(names);crtjson_free(entries);
			if (fmt) for (i=0;i<depth-1;i++) *ptr++='\t';
			*ptr++='}';*ptr++=0;
			return out;	
		}
		crtjson *create_reference(crtjson *item) {
			crtjson *ref=new_json();if (!ref) return 0;
			memcpy(ref,item,sizeof(crtjson));ref->string=0;
			ref->type|= crtjsontype_reference;ref->next=ref->prev=0;
			return ref;
		}
		crtjson *duplicate_json(crtjson *item) {
			crtjson *ref=new_json();if (!ref) return 0;
			memcpy(ref,item,sizeof(crtjson));
			if (item->string) ref->string=crtjson_strdup(item->string);
			if (item->valuestring) ref->valuestring=crtjson_strdup(item->valuestring);
			ref->child=ref->next=ref->prev=0;
			crtjson *child=item->child;
			crtjson *prev=0;
			while(child) {
				crtjson *newchild=duplicate_json(child);
				if (ref->child==0) ref->child=newchild;
				newchild->prev=prev;
				if (prev) prev->next=newchild;
				prev=newchild;
				child=child->next;
			}
			return ref;
		}
		void *(*crtjson_malloc)(size_t sz);
		void (*crtjson_free)(void *ptr);
		const char *ep;
	};
	/*class crtjsonparser1{
	public:
		crtjsonparser1() {}
		crtjsonparser1(const char* jsonstr) {json=NULL;parse(jsonstr);}
		virtual ~crtjsonparser1() {if (json) cJSON_Delete(json);}
		bool parse(const char* jsonstr)
		{
			if (json) cJSON_Delete(json);
			json=cJSON_Parse(jsonstr);
			return json!=NULL;
		}
		string get_last_error() {return cJSON_GetErrorPtr();}
		string get_json() {
			string ret;
			if (json) {
				char *out=cJSON_Print(json);
				ret=out;
				free(out);
			}
			return ret;
		}
	protected:
		cJSON *json;
	};*/
};
