#pragma once

#include "crtlib.h"
#include "crtstring.h"
#include "crtcookie.h"
#include "crtfile.h"

#define CRTSINAWEIBO_USERAGENT	"Mozilla/5.0 (compatible)"

namespace crtfun {
	//return success or not, cookie is the success cookie json, result is user info or why fail json
	static bool crtsinaweibo_get_login_cookie(const char *username,const char *passwd,string &cookies,string &result) {
		crtcookie cookie;
		crtjsonparser parser;
		char url[4096];
		setup_http_user_agent(CRTSINAWEIBO_USERAGENT);

		//prelogin for get nonce
		sprintf(url,"http://login.sina.com.cn/sso/prelogin.php?entry=weibo&callback=sinaSSOController.preloginCallBack&su=%s&client=ssologin.js(v1.3.16)",crtbase64::encode_str(username).c_str());
		string ret=cookie.http_download_to_str(url,NULL,NULL);
		string jsonstr=crtregex::match_one_bytoken(ret.c_str(),"sinaSSOController.preloginCallBack(",")");
		//string jsonstr=crtregex::match_one(ret.c_str(),"(?<=sinaSSOController\\.preloginCallBack\\().*?(?=\\))");
		crtjson *json=parser.parse(jsonstr.c_str());
		if (!json) {
			result="{\"result\":false,\"errno\":1,\"reason\":\"prelogin failed!\"}";
			crtdebug("[SINAWEIBO][ERROR]prelogin failed: error:1\n");
			return false;
		}
		crtdebug("[SINAWEIBO][1]prelogin:%s\n",parser.tostring(json).c_str());

		//ajaxlogin for direct url
		map<string,string> postdata;
		postdata["entry"]="weibo";
		postdata["gateway"]="1";
		postdata["from"]="";
		postdata["savestate"]="7";
		postdata["useticket"]="1";
		postdata["ssosimplelogin"]="1";
		postdata["su"]=crtbase64::encode_str(crturl_encode(username).c_str());
		postdata["service"]="miniblog";
		postdata["servertime"]=parser.findstr(json,"servertime");
		postdata["nonce"]=parser.findstr(json,"nonce");
		postdata["pwencode"]="wsse";
		postdata["sp"]=crtsha::shastr2str(
			(crtsha::shastr2str(crtsha::shastr2str(passwd,crtshatype_sha1).c_str(),crtshatype_sha1)+
			postdata["servertime"]+postdata["nonce"]
			).c_str(),crtshatype_sha1);
		postdata["encoding"]="UTF-8";
		postdata["url"]="http://weibo.com/ajaxlogin.php?framelogin=1&callback=parent.sinaSSOController.feedBackUrlCallBack";
		postdata["returntype"]="META";
		parser.delete_json(json);
		string logindata = cookie.http_download_to_str("http://login.sina.com.cn/sso/login.php?client=ssologin.js(v1.3.16)",crtmap2post(postdata).c_str(),NULL);
		crtdebug("[SINAWEIBO][2]logindata:%s\n",logindata.c_str());

		//ajaxlogin for feedback
		string loginret=crtregex::match_one(logindata.c_str(),"(?<=replace\\(['\"]).*?(?=['\"]\\))");
		string loginresult = cookie.http_download_to_str(loginret.c_str(),NULL,NULL);
		crtdebug("[SINAWEIBO][3]loginresult:%s\n",loginresult.c_str());

		//feedback Callback
		string userinfo=crtregex::match_one_bytoken(loginresult.c_str(),"feedBackUrlCallBack(",")");
		//string userinfo=crtregex::match_one(loginresult.c_str(),"(?<=feedBackUrlCallBack\\().*?(?=\\))");
		crtjson *info=parser.parse(userinfo.c_str());
		if (!info) {
			result="{\"result\":false,\"errno\":2,\"reason\":\"ajaxlogin failed!\"}";
			crtdebug("[SINAWEIBO][ERROR]ajaxlogin failed: error:2\n");
			return false;
		}
		crtdebug("[SINAWEIBO][4]loginjson:%s\n",parser.tostring(info).c_str());
		if (!parser.findbool(info,"result")) {
			crtdebug("[SINAWEIBO][ERROR]loginfailed: error:%s reason:%s"
				,parser.findstr(info,"errno").c_str(),parser.findstr(info,"reason").c_str());
			return false;
		}
		cookies=cookie.savecookietojsonstring();
		result=userinfo;
		parser.delete_json(info);
		return true;
	}
	static bool crtsinaweibo_needrelogin(const char *rethtml) {
		return crtregex::iswhole_matched(rethtml,".*location\\.replace\\(\\'http://weibo.com/sso/login\\.php.*");		
	}
	static string crtsinaweibo_get_info_page(const char *cookies, const char *weiboid) {
		crtcookie cookie;
		char url[4096];
		setup_http_user_agent(CRTSINAWEIBO_USERAGENT);
		sprintf(url,"http://weibo.com/%s/info",weiboid);
		if (!cookie.getcookiefromjsonstring(cookies)) {
			crtdebug("[SINAWEIBO][ERROR]parse cookies error!\n");
			return "";
		}
		string cnt=cookie.http_download_to_str(url,NULL,NULL);
		if (crtsinaweibo_needrelogin(cnt.c_str())) return "";
		return cnt;
	}
	static string crtsinaweibo_get_follow_page(const char *cookies, const char *weiboid, int page=1) {
		crtcookie cookie;
		char url[4096];
		setup_http_user_agent(CRTSINAWEIBO_USERAGENT);
		sprintf(url,"http://weibo.com/%s/follow?page=%d",weiboid,page);
		if (!cookie.getcookiefromjsonstring(cookies)) {
			crtdebug("[SINAWEIBO][ERROR]parse cookies error!\n");
			return "";
		}
		string cnt=cookie.http_download_to_str(url,NULL,NULL);
		if (crtsinaweibo_needrelogin(cnt.c_str())) return "";
		return cnt;
	}
	static string crtsinaweibo_parse_info_page(const char *infopage, const char *weiboid) {
		vector<string> values=crtregex::match_all(infopage,"(?<=STK\\.pageletM\\.view\\().*?(?=\\)\\</script\\>)");
		vector<string>::iterator it=values.begin();
		crtjsonparser parser;
		crtjson *root=parser.createobject();
		while(it!=values.end()) {
			crtjson *json=parser.parse((*it).c_str());
			if (json) {
				//"pl_content_top" 顶栏的一些页面定义
				//"pl_content_myPersonalInfo" 自己的个人资料
				//"pl_content_hisPersonalInfo" 个人资料，取html
				//"pl_content_userInfo" 右中栏个人信息
				//"pl_content_myData" 自己的个人经历
				//"pl_content_setskin" 自己的皮肤设置
				//"pl_content_hisData" 个人经历，取html
				//"pl_content_litePersonInfo" 认证信息，个人信息统计，取html
				//"pl_content_medal" 勋章
				//"pl_content_chainFollowers" 右栏关注关系，谁也关注
				//"pl_content_sameFriends" 共同关注
				//"pl_content_myTags" 自己的标签
				//"pl_content_hisTags" 他的标签
				//"pl_content_topic" 关注话题
				//"pl_content_hisFans" 右栏他的粉丝
				//"pl_common_feedback" 意见反馈
				//"pl_content_hisOperationPlate" 举报中心
				//"pl_content_Base" 内容基础js
				string pid=parser.findstr(json,"pid");
				//js css html
				if (pid=="pl_content_hisPersonalInfo" || pid=="pl_content_hisData" ||
					pid=="pl_content_litePersonInfo" || pid=="pl_content_hisTags") {
					string html=parser.findstr(json,"html");
					const char *chtml=html.c_str();
					if (pid=="pl_content_hisPersonalInfo") {
						/*
						  昵称：<div class=\"name clearfix\">\n\t\t<div class=\"left\">\n\t\tttissing<a target=\"_blank\"
						  	<div class=\"name clearfix\">\n\t\t<div class=\"left\">\n\t\tskyzhw\t\t<a suda-data=\"
						  头像：<div class=\"face\">...<img src=\"http://tp4.sinaimg.cn/2169116411/180/5602553982/1\" alt=\"
						  级别：<span node-type=\"level\" class=\"W_level_num l6\"></span>
						  认证：
							<img src=\"http://img.t.sinajs.cn/t4/style/images/common/transparent.gif\" title= \"新浪个人认证 \" alt=\"新浪个人认证 \" class=\"approve\"/>
							<img src=\"http://img.t.sinajs.cn/t4/style/images/common/transparent.gif\" title= \"新浪机构认证\" alt=\"新浪机构认证\" class=\"approve_co\"/>
							<img src=\"http://img.t.sinajs.cn/t4/style/images/common/transparent.gif\" title= \"微博达人\" alt=\"微博达人\" class=\"ico_club\" node-type=\"daren\"/>
						  性别，地区，描述：
						  	<p><img width=\"11\" height=\"12\" class=\"male\" src=\"http://img.t.sinajs.cn/t4/style/images/common/transparent.gif\" title=\"男\">&nbsp;北京，朝阳区</p>\n\t<p>他还没填写个人介绍</p>\n\t\t<div class=\"concern clearfix\">
						  	<p><img width=\"11\" height=\"12\" class=\"female\" src=\"http://img.t.sinajs.cn/t4/style/images/common/transparent.gif\" title=\"女\">&nbsp;台湾，台北市</p>\n\t<p>博客：<a href=\"http://www.yukihsu.com/forum.php\" target=\"_blank\">http://www.yukihsu.com/forum.php</a></p>\t\t<p>工作連繫:yeh3g2001@gmail.com愛鈺官網：http://www.yukihsu.com臉書粉絲團：https://www.facebook.com/IsYukiHsu</p>\n\t\t<div class=\"concern clearfix\">\n\t\t<div class=\"handle_btn\"
						*/
						parser.objectadd_string(root,"nick",string_replace(string_replace(string_replace(crtregex::match_one_bytoken(chtml,
							"<div class=\\\"left\\\">","<a "),"\\r",""),"\\n",""),"\\t","").c_str());
						/*parser.objectadd_string(root,"nick",string_replace(string_replace(string_replace(crtregex::match_one(chtml,
							"(?<=\\<div class\\=\\\\\"left\\\\\"\\>).*?(?=\\<a )"//(\\\\[ntr]){2,2}
							),"\\r",""),"\\n",""),"\\t","").c_str());*/
						/*parser.objectadd_string(root,"nick1",crtregex::match_one(chtml,
							"(?<=\\<div class\\=\\\\\"left\\\\\"\\>(\\\\[ntr])*).*?(?=\\<a )"
							).c_str());*/
						parser.objectadd_string(root,"picurl",crtregex::match_one_bytoken(crtregex::match_one_bytoken(chtml,
							"<div class=\\\"face\\\">","</div>").c_str(),
							"<img src=\\\"","\\\"").c_str());
						/*parser.objectadd_string(root,"picurl",crtregex::match_one(crtregex::match_one(chtml,
							"(?<=\\<div class\\=\\\\\"face\\\\\"\\>).*?(?=\\</div\\>)").c_str(),
							"(?<=\\<img src\\=\\\\\").*?(?=\\\\\")").c_str());*/
						parser.objectadd_number(root,"level",atoi(crtregex::match_one_bytoken(chtml,
							"<span node-type=\\\"level\\\" class=\\\"W_level_num l","\\\"").c_str()));
						/*parser.objectadd_number(root,"level",atoi(crtregex::match_one(chtml,
							"(?<=\\<span node\\-type\\=\\\\\"level\\\\\" class=\\\\\"W_level_num l).*?(?=\\\\\")").c_str()));*/
						parser.objectadd_string(root,"verifytype",crtregex::match_one(chtml,
							"(?<=\\<img src\\=\\\\\"http://img.t.sinajs.cn/t4/style/images/common/transparent.gif\\\\\" title=[ ]*\\\\\").*?(?=[ ]*\\\\\")").c_str());
						string sexareadesc=crtregex::match_one_bytoken(chtml,"<img width=\\\"11\\\" height=\\\"12\\\"","<div class=\\\"");
						/*string sexareadesc=crtregex::match_one(chtml,
							"(?<=\\<img width\\=\\\\\"11\\\\\" height\\=\\\\\"12\\\\\").*?(?=\\<div class\\=\\\\\")");*/
						parser.objectadd_string(root,"gender",crtregex::match_one_bytoken(sexareadesc.c_str(),
							"class=\\\"","\\\"").c_str());
						/*parser.objectadd_string(root,"gender",crtregex::match_one(sexareadesc.c_str(),
							"(?<=class=\\\\\").*?(?=\\\\\")").c_str());*/
						string area=crtregex::match_one_bytoken(sexareadesc.c_str(),
							"&nbsp;","</p>");
						/*string area=crtregex::match_one(sexareadesc.c_str(),
							"(?<=\\&nbsp;).*?(?=\\</p\\>)");*/
						string subarea;
						size_t pos=area.find("\xef\xbc\x8c");//， 
						if (pos!=string::npos) {
							subarea=area.substr(pos+3);
							area=area.substr(0,pos);
						}
						parser.objectadd_string(root,"area",area.c_str());
						parser.objectadd_string(root,"subarea",subarea.c_str());
						parser.objectadd_string(root,"desc",string_replace(string_replace(string_replace(
							string_replace(string_replace(crtregex::match_one_bytoken(sexareadesc.c_str(),
							"</p>",NULL),"\\r",""),"\\n",""),"\\t",""),"<p>",""),"</p>","").c_str());
						/*parser.objectadd_string(root,"desc",string_replace(string_replace(string_replace(
							string_replace(string_replace(crtregex::match_one(sexareadesc.c_str(),
							"(?<=\\</p\\>).*"),"\\r",""),"\\n",""),"\\t",""),"<p>",""),"</p>","").c_str());*/
					} else if (pid=="pl_content_hisData") {
						/*
							replace all \r\n\t
							<strong>基本信息</strong></dt><dd><ul>
							<li>生日：巨蟹座</li>
							...
							</ul>\n\t</dd>
							<strong>教育信息</strong></dt><dd><ul>
							<li><span class=\"title\">大学：</span>
							<div class=\"detail\"><p><a class=\"name\" href=\"…\">佛山科学技术学院</a> (2000年)<br>工学院00电子</p>
							<p><a class=\"name\" href=\"...\">华中科技大学</a> (2004年)<br>04软件工程</p>
							</div></li>
							<li><span class=\"title\">高中：</span>
							<div class=\"detail\"><p><a class=\"name\" href=\"…\">佛山三中</a> (1997年)<br>2班</p>
							</div></li>
							<li><span class=\"title\">初中：</span>
							<div class=\"detail\"><p><a class=\"name\" href=\"…\">佛山三中初中部</a> (1994年)<br>7班</p>
							</div></li></ul></dd>
							<strong>工作信息</strong></dt><dd><ul>
							<li><span class=\"title\">公司：</span>
							<div class=\"detail\"><p><a href=\"...\">酷6网</a>(2008 - 2011)<br>地区：北京 ，朝阳区<br>职位：开发部技术副总监</p>
							<p><a href=\"...\">炫一下</a>(2011 - 至今)<br>地区：北京 ，朝阳区<br>职位：CTO</p>
							</div><li></ul></dd>
							<strong>信用信息</strong>
						*/
						vector<string> infos=crtregex::match_all(chtml,
							"(?<=\\<strong\\>).*?(?=\\</ul\\>)");
						vector<string>::iterator it=infos.begin();
						while(it!=infos.end()) {
							const char *chtml=(*it).c_str();
							string key=crtregex::match_one_bytoken(chtml,NULL,"</strong>");
							//string key=crtregex::match_one(chtml,".*(?=\\</strong\\>)");
							//信用信息 
							if (key!="\xe4\xbf\xa1\xe7\x94\xa8\xe4\xbf\xa1\xe6\x81\xaf") {
								crtjson *info=parser.createobject();
								vector<string> lis=crtregex::match_all(chtml,"(?<=\\<li\\>).*?(?=\\<[/]*li\\>)");//工作信息的</li>笔误了 
								vector<string>::iterator it1=lis.begin();
								while(it1!=lis.end()) {
									const char *subinfo=(*it1).c_str();
									string key1=crtregex::match_one_bytoken(subinfo,"<span class=\\\"title\\\">","</span>");
									/*string key1=crtregex::match_one(subinfo,"(?<=\\<span class\\=\\\\\"title\\\\\"\\>).*?(?=\\</span\\>)");*/
									size_t pos=key1.find("\xef\xbc\x9a");//： 
									if (pos!=string::npos) key1=key1.substr(0,pos);
									if (!key1.empty()) {
										crtjson *subinfoj=parser.createarray();
										vector<string> details=crtregex::match_all(
											string_replace(string_replace(string_replace(subinfo,"\\n",""),"\\r",""),"\\t","").c_str(),
											"(?<=\\<p\\>\\<a ).*?(?=\\</p\\>)");
										vector<string>::iterator it2=details.begin();
										while(it2!=details.end()) {
											crtjson *sub2info=parser.createobject();
											const char *detail=(*it2).c_str();
											string subkey=crtregex::match_one_bytoken(detail,"\\\">","</a>");
											//string subkey=crtregex::match_one(detail,"(?<=\\\\\"\\>).*?(?=\\</a\\>)");
											parser.objectadd_string(sub2info,"name",subkey.c_str());
											crtstringtoken token(crtregex::match_one_bytoken(detail,"</a>",NULL),"<br>");
											//crtstringtoken token(crtregex::match_one(detail,"(?<=\\</a\\>).*"),"<br>");
											int i=0;
											//教育信息 
											bool edu=(key=="\xe6\x95\x99\xe8\x82\xb2\xe4\xbf\xa1\xe6\x81\xaf");
											while(token.ismore()) {
												string str=token.nexttoken();
												string_trim(str);
												if (!str.empty()) {
													if (edu) parser.objectadd_string(sub2info,(i==0)?"date":"subject",str.c_str());
													else {
														if (i==0) parser.objectadd_string(sub2info,"date",str.c_str());
														else {
															string value;
															size_t pos=str.find("\xef\xbc\x9a");//： 
															if (pos!=string::npos) {
																value=str.substr(pos+3);
																str=str.substr(0,pos);
															}
															if (str=="\xe5\x9c\xb0\xe5\x8c\xba") { //地区 
																string subarea;
																size_t pos=value.find("\xef\xbc\x8c");//， 
																if (pos!=string::npos) {
																	subarea=value.substr(pos+3);
																	value=value.substr(0,pos);
																}
																string_trim(value);
																string_trim(subarea);
																if (!value.empty()) parser.objectadd_string(sub2info,"area",value.c_str());
																if (!subarea.empty()) parser.objectadd_string(sub2info,"subarea",subarea.c_str());
															} else {
																//职位 
																if (str=="\xe8\x81\x8c\xe4\xbd\x8d") str="position";
																parser.objectadd_string(sub2info,str.c_str(),value.c_str());
															}
														}
													}
												}
												i++;
											}
											//crtdebug("result:key:%s,sub:%s\n",key1.c_str(),detail);
											parser.arrayadd(subinfoj,sub2info);
											it2++;
										}
										parser.objectadd(info,key1.c_str(),subinfoj);
									} else {
										string key1=subinfo;
										string value;
										size_t pos=key1.find("\xef\xbc\x9a");//： 
										if (pos!=string::npos) {
											value=key1.substr(pos+3);
											key1=key1.substr(0,pos);
										}
										parser.objectadd_string(info,key1.c_str(),value.c_str());
									}
									it1++;
								}
								parser.objectadd(root,key.c_str(),info);
								//crtdebug("result:%s\n",key.c_str());
							}
							it++;
						}
					} else if (pid=="pl_content_litePersonInfo") {
						/*
						  新浪认证：
							公司class icon_entr：
							<div class=\"W_sina_vip\">\n  <dl>\n\t  <dt><span title=\"优酷官方微博\" class=\"icon_entr\"></span></dt>
							<dd class=\"W_textb\">\n\t\t<p>优酷官方微博</p>\n  </dd>
							个人:
							<div class=\"W_sina_vip\">\n  <dl>\n\t  <dt><span title=\"炫一下（北京）科技有限公司 CTO\"></span></dt>
							<dd class=\"W_textb\">\n\t\t<p>炫一下（北京）科技有限公司 CTO</p>\n  </dd>
							达人class club：
							<div class=\"W_sina_vip\">\n  <dl>\n  <dt><a href=\"http://club.weibo.com/intro?from=otherprofile&wvr=3.6&loc=darenicon\"><span class=\"club\"></span></a></dt>
							<a node-type=\"daren\" target=\"__blank\" href=\"http://club.weibo.com/myRank?from=otherprofile&wvr=3.6&loc=daren\">高级达人</a></dd>
						  关注：<strong node-type=\"follow\">1642</strong>
						  粉丝：<strong node-type=\"fans\">1227243</strong>
						  微博：<strong node-type=\"weibo\">10105</strong>
						*/
						string info=string_replace(string_replace(string_replace(string_replace(
							crtregex::match_one_bytoken(chtml,"<dd class=\\\"W_textb\\\">","</p>")
							//crtregex::match_one(chtml,"(?<=\\<dd class\\=\\\\\"W_textb\\\\\"\\>).*?(?=\\</p\\>)")
							,"\\n",""),"\\r",""),"\\t",""),"<p>","");
						if (info.empty()) {
							info=crtregex::match_one_bytoken(chtml,"<a node-type=\\\"daren\\\"","</a>");
							//info=crtregex::match_one(chtml,"(?<=\\<a node-type\\=\\\\\"daren\\\\\").*?(?=\\</a\\>)");
							size_t pos=info.rfind("\\\">");
							if (pos!=string::npos) info=info.substr(pos+3);
						}
						parser.objectadd_string(root,"verifyinfo",info.c_str());
						parser.objectadd_string(root,"follow",
							crtregex::match_one_bytoken(chtml,"<strong node-type=\\\"follow\\\">","</strong>").c_str());
						parser.objectadd_string(root,"fans",
							crtregex::match_one_bytoken(chtml,"<strong node-type=\\\"fans\\\">","</strong>").c_str());
						parser.objectadd_string(root,"weibo",
							crtregex::match_one_bytoken(chtml,"<strong node-type=\\\"weibo\\\">","</strong>").c_str());
						/*parser.objectadd_string(root,"follow",
							crtregex::match_one(chtml,"(?<=\\<strong node-type\\=\\\\\"follow\\\\\"\\>).*?(?=\\</strong\\>)").c_str());
						parser.objectadd_string(root,"fans",
							crtregex::match_one(chtml,"(?<=\\<strong node-type\\=\\\\\"fans\\\\\"\\>).*?(?=\\</strong\\>)").c_str());
						parser.objectadd_string(root,"weibo",
							crtregex::match_one(chtml,"(?<=\\<strong node-type\\=\\\\\"weibo\\\\\"\\>).*?(?=\\</strong\\>)").c_str());*/
					} else if (pid=="pl_content_hisTags") {
						/*
						  <span><a href=\"http://s.weibo.com/user/&tag=%E6%97%85%E8%A1%8C&from=otherprofile&wvr=3.6\" class=\"ft16\">旅行</a></span>
						  <span><a href=\"http://s.weibo.com/user/&tag=%E9%9F%B3%E4%B9%90&from=otherprofile&wvr=3.6\" class=\"ft18 ft_b\">音乐</a></span>
						  <span><a href=\"http://s.weibo.com/user/&tag=%E5%BE%B7%E5%B7%9E%E6%89%91%E5%85%8B&from=otherprofile&wvr=3.6\" class=\"ft12\">德州扑克</a></span>
						*/
						vector<string> infos=crtregex::match_all(chtml,
							"(?<=\\<span\\>).*?(?=\\</span\\>)");
						vector<string>::iterator it=infos.begin();
						if (infos.size()>0) {
							crtjson *ar=parser.createarray();
							while(it!=infos.end()) {
								parser.arrayadd_string(ar,crtregex::match_one((*it).c_str(),
									"(?<=\\<a href\\=.*\\\"\\>).*?(?=\\</a\\>)").c_str());
								it++;
							}
							parser.objectadd(root,"tags",ar);
						}
					}
				}
				//crtdebug("[SINAWEIBO][INFO]pid:%s\n%s\n",pid.c_str(),parser.tostring(json).c_str());
				parser.delete_json(json);
			} else
				crtdebug("[SINAWEIBO][INFO]warning: parse failed in %s\n",(*it).c_str());
			it++;
		}
		if (!parser.isempty(root)) {
			parser.objectadd_string(root,"uid",weiboid);
			string ret=parser.tostring(root);
			parser.delete_json(root);
			return ret;
		}
		parser.delete_json(root);
		return "";
	}
	static string crtsinaweibo_parse_follow_page(const char *pageinfo, const char *weiboid , bool *havenext) {
		vector<string> values=crtregex::match_all(pageinfo,"(?<=STK\\.pageletM\\.view\\().*?(?=\\)\\</script\\>)");
		vector<string>::iterator it=values.begin();
		crtjsonparser parser;
		if (havenext) *havenext=false;
		crtjson *root=parser.createobject();
		parser.objectadd_string(root,"uid",weiboid);
		while(it!=values.end()) {
			crtjson *json=parser.parse((*it).c_str());
			if (json) {
				//"pl_relation_hisfollow" 
				string pid=parser.findstr(json,"pid");
				//js css html
				if (pid=="pl_relation_hisfollow") {
					crtjson *followsj=parser.createarray();
					parser.objectadd(root,"follows",followsj);
					string html=parser.findstr(json,"html");
					const char *chtml=html.c_str();
					vector<string> follows=crtregex::match_all(chtml,
						"(?<=\\<li class\\=\\\\\"clearfix W_linecolor).*?(?=\\</li\\>)");
					vector<string>::iterator it=follows.begin();
					while(it!=follows.end()) {
						const char *html=(*it).c_str();
						crtjson *followsjobj=parser.createobject();
						string name_space=crtregex::match_one_bytoken(html,"<p class=\\\"name mbspace\\\">","</p>");
						//string name_space=crtregex::match_one(html,"(?<=\\<p class\\=\\\\\"name mbspace\\\\\"\\>).*?(?=\\</p\\>)");
						const char *cname_space=name_space.c_str();
						string connectspace=crtregex::match_one_bytoken(html,"<p class=\\\"connect mbspace\\\">","</p>");
						//string connectspace=crtregex::match_one(html,"(?<=\\<p class\\=\\\\\"connect mbspace\\\\\"\\>).*?(?=\\</p\\>)");
						parser.objectadd_string(followsjobj,"nick",crtregex::match_one_bytoken(cname_space,"\\\"W_f14\\\">","</a>").c_str());
						/*parser.objectadd_string(followsjobj,"nick",crtregex::match_one(cname_space,"(?<=\\\\\"W_f14\\\\\"\\>).*?(?=\\</a\\>)").c_str());*/
						parser.objectadd_string(followsjobj,"weiboid",crtregex::match_one_bytoken(cname_space,"usercard=\\\"id=","\\\"").c_str());
						/*parser.objectadd_string(followsjobj,"weiboid",crtregex::match_one(cname_space,"(?<=usercard\\=\\\\\"id\\=).*?(?=\\\\\")").c_str());*/
						parser.objectadd_string(followsjobj,"verifytype",crtregex::match_one(cname_space,
							"(?<=\\<img src\\=\\\\\"http://img.t.sinajs.cn/t4/style/images/common/transparent.gif\\\\\" title=[ ]*\\\\\").*?(?=[ ]*\\\\\")").c_str());
						vector<string> cnts=crtregex::match_all(connectspace.c_str(),"(?<=\\<a ).*?(?=\\</a\\>)");
						if (cnts.size()>=3) {
							parser.objectadd_string(followsjobj,"follow",crtregex::match_one_bytoken(cnts[0].c_str(),"\">",NULL).c_str());
							parser.objectadd_string(followsjobj,"fans",crtregex::match_one_bytoken(cnts[1].c_str(),"\">",NULL).c_str());
							parser.objectadd_string(followsjobj,"weibo",crtregex::match_one_bytoken(cnts[2].c_str(),"\">",NULL).c_str());
							/*parser.objectadd_string(followsjobj,"follow",crtregex::match_one(cnts[0].c_str(),"(?<=\\\"\\>).*").c_str());
							parser.objectadd_string(followsjobj,"fans",crtregex::match_one(cnts[1].c_str(),"(?<=\\\"\\>).*").c_str());
							parser.objectadd_string(followsjobj,"weibo",crtregex::match_one(cnts[2].c_str(),"(?<=\\\"\\>).*").c_str());*/
						}
						parser.arrayadd(followsj,followsjobj);
						it++;
					}
					if (havenext) {
						string pages=crtregex::match_one_bytoken(chtml,"<div class=\\\"W_pages W_pages_comment\\\"","</div>");
						//string pages=crtregex::match_one(chtml,"(?<=\\<div class\\=\\\\\"W_pages W_pages_comment\\\\\").*?(?=\\</div\\>)");
						*havenext=(pages.find("\xe4\xb8\x8b\xe4\xb8\x80\xe9\xa1\xb5")!=string::npos);
					}
					//crtdebug("%s",html)
				}
				//crtdebug("[SINAWEIBO][FOLLOW]pid:%s\n%s\n",pid.c_str(),parser.tostring(json).c_str());
				parser.delete_json(json);
			} else
				crtdebug("[SINAWEIBO][FOLLOW]warning: parse failed in %s\n",(*it).c_str());
			it++;
		}
		string ret=parser.tostring(root);
		parser.delete_json(root);
		return ret;
	}
	//savetmpfile for cache data
	static string crtsinaweibo_fetchallinfo(const char *cookies,const char *weiboid, bool savetmpfile) {
		crtjsonparser parser;
		//frist parse info
		string content;
		//check cache first
		if (savetmpfile)
			content=crtread_str_from_file((string("info_")+weiboid).c_str());
		if (!content.empty())
			crtdebug("[SINAWEIBO]use cached info result\n");
		else
			content=crtsinaweibo_get_info_page(cookies,weiboid);
		string json=crtsinaweibo_parse_info_page(content.c_str(),weiboid);
		if (json.empty()) {
			crtdebug("[SINAWEIBO][ERROR]error get info!\n");
			return "";
		}
		if (savetmpfile) crtsave_str_to_file((string("info_")+weiboid).c_str(),content.c_str());
		crtdebug("[SINAWEIBO][INFO]%s\n",json.c_str());
		crtjson *root=parser.parse(json.c_str());
		crtjson *allfollows=parser.createarray();
		bool next=true;
		int page=1;
		char file[1024];
		while(next) {
			sprintf(file,"page_%s_%d",weiboid,page);

			//check cache first
			if (savetmpfile)
				content=crtread_str_from_file(file);
			else
				content.clear();
			if (!content.empty())
				crtdebug("[SINAWEIBO]use cached page %d result\n",page);
			else
				content=crtsinaweibo_get_follow_page(cookies,weiboid,page);
			string json=crtsinaweibo_parse_follow_page(content.c_str(),weiboid,&next);
			if (json.empty()) {
				crtdebug("[SINAWEIBO][ERROR]error get page %d!\n",page);
				parser.delete_json(root);
				return "";
			}
			if (savetmpfile) crtsave_str_to_file(file,content.c_str());
			crtdebug("[SINAWEIBO][PAGE]%d:%s\n",page,json.c_str());
			crtjson *follow=parser.parse(json.c_str());
			parser.arrayappend(parser.find(follow,"follows"),allfollows);
			parser.delete_json(follow);
			page++;
		}
		parser.objectadd(root,"follows",allfollows);
		//parser.objectadd_number(root,"followscnt",parser.arraysize(allfollows));
		string ret = parser.tostring(root,0);
		parser.delete_json(root);
		crtdebug("[SINAWEIBO]finish fetch for %s\n%s\n",weiboid,ret.c_str());
		return ret;
	}
};