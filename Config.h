#pragma once
#include <map>
#include <vector>
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_utils.hpp"

using namespace rapidxml;

class Config
{
public:
	static Config& getInstance() { return m_self; }
	bool readConf(const std::string& conf);
	void showConf();
	
	const std::map<std::string, std::string>& getMT4ConnConf() { return m_mt4ConnConf; }
	const std::map<std::string, std::string>& getHTTPConf() { return m_HTTPConf; }
	const std::map<std::string, std::string>& getLogConf() { return m_logConf; }

	std::string getExePath(){ return m_exePath; }
	
private:
	Config() ;
	~Config() = default;

	Config(const Config& other) = default;
	Config& operator= (const Config& other) = default;

	void getProjectPath(std::string &path);
	void  split(const std::string& in, std::vector<std::string>& out, const std::string& delimeter);

	void readAppConf(const std::string& path);
	void readLogConf(const std::string& path);

	void readAppNode(xml_node<> * node);

	bool writeConf(const std::string& key, const std::string& value);

private:
	std::map <std::string, std::string> m_HTTPConf;
	std::map <std::string, std::string> m_mt4ConnConf;
	std::map <std::string, std::string> m_logConf;
	std::string m_confPath;
	std::string m_exePath;
	
	enum class Conf{ HTTP_CONN_CONF, MT4_CONN_CONF};
	std::map <std::string, Conf> m_confKey;

	static Config m_self;
};
