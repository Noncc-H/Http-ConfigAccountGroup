#include "Config.h"
#include <fstream>
#include <windows.h>

Config Config::m_self;

Config::Config()
{
	m_confKey["mt4-conn"] = Conf::MT4_CONN_CONF;
	m_confKey["httpserver-conn"] = Conf::HTTP_CONN_CONF;
}

bool Config::readConf(const std::string& conf)
{
	assert(!conf.empty());
	std::fstream _file;
	std::string path;
	getProjectPath(path);
	if (path.empty())
	{
		return false;
	}
	m_exePath = path;
	std::string fullPath = path + conf;
	_file.open(fullPath, std::ios::in);

	if (!_file)
	{
		std::ofstream ofs(path + "logs/error.log");
		ofs << "read configure file: " << fullPath << "failed.";
		ofs.close();
		return false;
	}

	m_confPath = fullPath;

	if (conf.compare(7, -1, "app.conf") == 0)
		readAppConf(m_confPath);
	else if (conf.compare(7, -1, "spdlog.conf") == 0)
		readLogConf(m_confPath);

	return true;
}

void Config::readAppConf(const std::string& path)
{
	file<> fdoc(path.c_str());
	xml_document<> doc;
	doc.parse<0>(fdoc.data());

	xml_node<> *root = doc.first_node();
	assert(root);
	xml_node<> *node = root->first_node();
	while (node)
	{
		readAppNode(node);
		node = node->next_sibling();
	}
}
void Config::readLogConf(const std::string& path)
{
	file<> fdoc(m_confPath.c_str());
	xml_document<> doc;
	doc.parse<0>(fdoc.data());

	//root node
	xml_node<> *root = doc.first_node();

	xml_node<> *node = root->first_node();
	while (node)
	{
		if(std::string("file").compare(node->name()) == 0)
		    m_logConf[node->name()] = m_exePath + node->value();
		else
			m_logConf[node->name()] = node->value();
		node = node->next_sibling();
	}
}

void Config::readAppNode(xml_node<> * node)
{
	if (node == nullptr)
		return;

	std::vector<std::string> v;
	xml_node<> *n = node->first_node();
	xml_attribute<> *attr = nullptr;

	while (n)
	{
		switch (m_confKey[node->name()])
		{
		case Conf::MT4_CONN_CONF:
			m_mt4ConnConf[n->name()] = n->value();
			break;
		case Conf::HTTP_CONN_CONF:
			m_HTTPConf[n->name()] = n->value();
			break;
		default:
			break;
		}
		n = n->next_sibling();
	}
}

void Config::showConf()
{
	std::ofstream ofs(m_exePath + "logs/config.log");
	ofs << "--------mt4 conn conf----------" << std::endl;
	for (auto &c : m_mt4ConnConf)
	{
		ofs << c.first << ":" << c.second << std::endl;
	}
	ofs << "--------http conn conf---------" << std::endl;
	for (auto &c : m_HTTPConf)
	{
		ofs << c.first << ":" << c.second << std::endl;
	}
	ofs.close();
}

bool Config::writeConf(const std::string& key, const std::string& value)
{
	assert(!key.empty() && !m_confPath.empty());
	file<> fdoc(m_confPath.c_str());

	xml_document<> doc;
	doc.parse<0>(fdoc.data());

	//root node
	xml_node<>* root = doc.first_node();

	xml_node<>* node = root->first_node(key.c_str());
	if (node == nullptr)
		node = root->first_node();
	while (node)
	{
		xml_node<> *tmp = node->next_sibling(key.c_str());
		if (tmp == nullptr)
			node = node->next_sibling();
		else
		{
			tmp->value(value.c_str());
			return true;
		}
	}
	return false;
}

void Config::getProjectPath(std::string &path)
{
	char p[MAX_PATH] = { 0 };
	if (GetModuleFileName(0, p, MAX_PATH))
	{
		path = p;
		path = path.substr(0, path.rfind("\\"));
		path += "\\";
	}
}


void  Config::split(const std::string& in, std::vector<std::string>& out, const std::string& delimeter)
{
	int begin = 0;
	int end = in.find(delimeter);
	std::string tmp = in.substr(0, end);
	out.push_back(tmp);
	while (end != std::string::npos)
	{
		begin = end + 1;
		end = in.find(delimeter, end + 1);
		tmp = in.substr(begin, end - begin);
		out.push_back(tmp);
	}
}
