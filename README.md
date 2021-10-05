# crtlib
* crt lib for windows and linux
* crtlib 是一个跨平台的c++库，库依赖了stl，include h即可使用
* crttestcase.h 测试用例，使用方法参考

## 数据结构：
* crtlib.h 基础库头文件，包括跨平台环境下一些基础定义，包含的库函数等
* crtstring.h 常用字符串处理
* crtcharset.h 字符集处理
* crtbuffer.h 缓冲管理类
* crtobjmap.h 对象管理
* crtstructure.h 常用算法汇总，例如lru

## 多线程：
* crtthread.h 线程，锁，单例等
* crtmsgthread.h 消息队列

## 底层封装：
* crtdebug.h 调试相关
* crtfile.h 文件操作封装
* crttime.h 时间操作封装

## 网络：
* crtsystem.h 常用网络设置
* crtsocket.h 网络操作封装
* crtsocketthread.h 网络线程
* crtsocketmodel.h 网络模型
* crthttp.h HttpClient
* crtcookie.h 带cookie自动处理的HttpClient
* crtlibuvhttpserver.h 依赖于libuv的http服务器实现

## 算法处理：
* crtsha.h sha实现，参照nettlelib
* crtsha.old.h sha原理实现，有问题
* crtbase64.h base64实现
* crtmd5.h md5实现
* crtsha.h sha实现，使用nettle库
* crtlogiccalc.h 四则运算
* crtregex.h 正则表达式运算

## 格式：
* crtjson.h json处理器，参照cJSON
* crtxml.h xml处理器，参照rapidxml
* crtbitparser.h 位处理
* crtcmdline.h 命令行处理
* crtpartfile.h 进度描述文件

## 正则表达式：
* crtregex.h 正则处理，参照deelx
* deelx/regex_deelx.chm 正则语法
* deelx/regex_MTracer2.1.msi 正则验证器

## 应用类：
* crtsinaweibo.h 新浪微博相关操作
