#pragma once

#include "crtlib.h"

/*
	crt part file 描述格式
	该类主要用于
*/

namespace crtfun {
	/**
	* @brief 文件片段描述类
	* @author Barry(barrytan@21cn.com,QQ:20962493)
	* @2014-05-09 新建类
	*/
	/**<pre>
	  主要用于描述文件中是否有缺失部分，常用于多线程下载任务等
	  最后的字节描述未下载区域
	  u32    CA FE BC 14 magic
	  u32    off
	  0x0    xxxxxxxxxx
	  0x600  CA FE 00 00 u16 mask u16 block length
	         FF FF FF FF next block pointer
	  0x1000 00 00 06 00
	</pre>*/
	class crtpartfile{
	}
};
