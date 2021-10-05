#pragma once

#include "crtlib.h"
#include "crtstring.h"
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"

using namespace rapidxml;

/**
* @brief xml处理类
* @author Barry(barrytan@21cn.com,QQ:20962493)
*/
/**<pre>
使用rapidxml库实现，帮助参考 rapidxml/xml_rapidxml_manual.html
使用Sample：
</pre>*/

namespace crtfun {
	class crtxml {
	public:
		void parse(const char *str){doc.parse<0>(str);}
		//xml_node->name() ->append_node
		xml_node<> *first_node(const char *nodename=NULL,size_t nodesize=0,bool bcase=true){return doc.first_node(nodename,nodesize,bcase);}
		//xml_attribute->name() ->value()
		xml_attribute<> *first_attribute(const char *attribname=NULL,size_t namesize=0,bool bcase=true){return doc.first_attribute(attribname,namesize,bcase);}
		xml_node<> alloc_node(const char *nodename,const char *nodevalue)
			{return doc.allocate_node(node_element,doc.allocate_string(nodename),doc.allocate_string(nodevalue));}
		xml_attribute<> alloc_attribute(const char *name,const char *value)
			{return doc.allocate_attribute(doc.allocate_string(name),doc.allocate_string(value));}
		void append_node(xml_node<> *node){doc.append_node(node);}
		void append_attribute(xml_attribute<> *att){doc.append_attribute(att);}
		string tostring(){string s;rapidxml::print(std::back_inserter(s), doc, 0);return s;}
		int attribute_count(xml_node<> *node){
			xml_attribute<> *attr = node->first_attribute();
			size_t count = 0;
			while (attr) {
				++count;
				attr = attr->next_attribute();
			}
			return count;
		}
		int node_count(xml_node<> *node){
			xml_node<> *child = node->first_node();
			size_t count = 0;
			while (child) {
				++count;
				child = child->next_sibling();
			}
			return count;
		}
		xml_document<> doc;
	protected:
	};
};
