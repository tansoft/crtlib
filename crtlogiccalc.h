#pragma once

#include <math.h>
#include "crtlib.h"
#include "crtstring.h"

/**
* @brief 四则混合运算类
* @author Barry(barrytan@21cn.com,QQ:20962493)
* @2007-06-07 新建类
* @2007-06-16 改正支持(-1)和(+1)计算，改正表示式空时计算出错问题
* @2007-09-20 修正一内存泄漏问题
*/
/**<pre>
用法：
	//支持+ - * /
	//支持^次方 ~余数 %余数
	//支持()
	double ret;
	bool bret = crtlogiccalc::logiccalc("((1.0+2.3)*12.2-6.5*23-2^5+8.2)/2.3+5~3", &ret);
	ret = -55.843479
</pre>*/
class crtlogiccalc {
public:
	static bool calc(string expr,double *ret)
	{
		crtlogiccalc calc;
		return calc.logiccalc(expr, ret);
	}

	//expr:运算公式,ret执行结果
	bool logiccalc(string expr,double *ret) {
		expr = string_trim(expr);
		while(!m_oper.empty()) m_oper.pop();
		while(!m_num.empty()) m_num.pop();
		string numstr;
		const char *poper=expr.c_str();
		char lastexp=0;
		while(*poper!='\0') {
			if ((*poper>='0' && *poper<='9') || *poper=='.') {
				//分析数字
				numstr+=*poper;
			} else {
				if (!numstr.empty()) {
					//数字压栈
					m_num.push(atof(numstr.c_str()));
					numstr.clear();
				}
				if (*poper=='(') {
					//如果是'('开始，无条件压栈
					m_oper.push(*poper);
				} else if (*poper=='+' || *poper=='-' ||
					*poper=='*' || *poper=='/' ||
					*poper=='~' || *poper=='%' ||
					*poper=='^' || *poper==')') {
					//如果还没有数字数据并且以+或-开头，说明该符号是加减号，理应保存到数字处理中，并跳出循环，但由于)压出栈时需要两个操作数，不能处理例如(-1)这种情况，因此可直接按成加减号操作，压多一个0在前面如0-1这样
					if ((m_num.empty() || lastexp=='(') && (*poper=='-' || *poper=='+')) {
						m_num.push(0.0);
					}
					if (!m_oper.empty()) {
						char oper = m_oper.top();
						//如果当前表达式优先级大于栈中的数进行压栈
						if (getpriority(*poper)>getpriority(oper)) {
							m_oper.push(*poper);
						} else {
							//栈中的数据优先级较高，计算栈中的数据
							while(!m_oper.empty() && !m_num.empty()) {
								double a,b,c;
								char o;
								//取出栈中表达式值
								b = m_num.top();
								m_num.pop();
								a = m_num.top();
								m_num.pop();
								o = m_oper.top();
								m_oper.pop();
								//已完成()中的计算
								if(o=='(') {
									//操作数没有用到，重新压回栈中
									m_num.push(a);
									m_num.push(b);
									//如果当前字符不是')'，需要继续处理该符号，减一以实现不加当前的字符
									if (*poper!=')') {
										poper--;
										break;
									}
								}
								//计算栈中的表达式
								if (!workout(a,b,&c,o)) return false;
								//把计算结果压回栈
								m_num.push(c);
								//如果当前不是')'括号,只需要计算一次结果
								if (*poper!=')') {
									//减一以实现不加当前的字符
									poper--;
									break;
								}
								//如果栈顶没有数据了,可以退出
								if (m_oper.empty()) {
									//认为')'是多出来的，不处理
									break;
								} else {
									//栈顶已经是'('，完成()内的所有操作
									o = m_oper.top();
									if (o=='(') {
										m_oper.pop();
										break;
									}
								}
							}
						}
					} else {
						//证明没有操作数在栈中，直接压栈
						m_oper.push(*poper);
					}
				}
			}
			lastexp=*poper;
			poper++;
		}
		//最后的数字压栈
		if (!numstr.empty()) {
			//数字压栈
			m_num.push(atof(numstr.c_str()));
			numstr.clear();
		}
		//表达式已处理完毕，计算栈中剩下的数据
		while(!m_oper.empty()) {
			//还有操作符，但是已经没有操作数了，报错
			if(m_num.empty()) {
				errmsg="发现多余的运算符";
				return false;
			}
			double a,b,c;
			char o;
			b = m_num.top();
			m_num.pop();
			a = m_num.top();
			m_num.pop();
			
			//丢弃()的处理
			do{
				if (m_oper.empty()) return false;
				o = m_oper.top();
				m_oper.pop();
			} while(o=='(' || o==')');
			//计算栈中的表达式
			if (!workout(a,b,&c,o)) return false;
			//把计算结果压回栈
			m_num.push(c);
		}
		//返回最后结果
		if (m_num.empty()) {
			//如果表达式不需要任何计算，返回0
			*ret=0.0;
		} else {
			if (m_num.empty()) return false;
			*ret = m_num.top();
			m_num.pop();
		}
		//如果栈中还有剩下的数据，返回出错
		if (!m_num.empty()) {
			numstr="发现多余的数";
			return false;
		}
		numstr.clear();
		return true;
	}
	string getlasterror(){return errmsg;}
private:
	stack<char> m_oper;	///<操作符栈
	stack<double> m_num; ///<操作数栈
	string errmsg; ///<错误代码
	///<计算对a,b进行操作opr的值
	bool workout(double a,double b,double *ret,char opr)
	{
		switch(opr)
		{
			case '+':*ret=a+b;return true;
			case '-':*ret=a-b;return true;
			case '*':*ret=a*b;return true;
			case '/':if (b==0) {
						errmsg="除零错误";
						return false;
					} else {
						*ret=a/b;
						return true;
					}
			case '~':
			case '%':
					if (b==0) {
						errmsg="求余数除零错误";
						return false;
					} else {
						*ret=(double)((int)a%(int)b);
						return true;
					}
			case '^':*ret=pow(a,b);return true;
		}
		errmsg="未知符号";
		return false;
	}
	///<获取运算符的运算级别
	int getpriority(char oper)
	{
		if (oper=='(' || oper==')') return 0;
		else if (oper=='+' || oper=='-') return 10;
		else if (oper=='~' || oper=='%') return 20;
		else if (oper=='*' || oper=='/') return 30;
		else if (oper=='^') return 40;
		return 0;
	}
};
