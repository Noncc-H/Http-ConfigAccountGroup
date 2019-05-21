#include "MT4Module.h"
#include "Logger.h"
#include <chrono>
#include <thread>
#include "Utils.h"

DirectConn::DirectConn()
{
}


DirectConn::~DirectConn()
{
}

bool DirectConn::mt4Init()
{
	std::string path = Utils::getInstance().getProjectPath();
	path += Config::getInstance().getMT4ConnConf().find("lib")->second;
	m_factoryInter.Init(path.c_str());
	if (m_factoryInter.IsValid() == FALSE)
	{
		SPDLOG(error, "mt4 lib init failed");
		return false;
	}
	if (m_factoryInter.WinsockStartup() != RET_OK)
	{
		SPDLOG(error, "mt4 lib init winsock lib failed.");
		return false;
	}
	if (NULL == (m_managerInter = m_factoryInter.Create(ManAPIVersion)))
	{
		SPDLOG(error, "mt4 factory create mananger interface failed.");
		return false;
	}
	return true;
}

bool DirectConn::mt4Login(const int login, const char* passwd)
{
	if (!mt4Init())
		return false;
	if (RET_OK != mt4Conn(Config::getInstance().getMT4ConnConf().find(std::move("host"))->second.c_str()))
		return false;
	int cnt = 0;
	while (cnt < 3)
	{
		if (RET_OK != m_managerInter->Login(login, passwd))
		{
			SPDLOG(error, "mt4 server login failed.---{}", cnt+1);
			cnt++;
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		else
		{
			cnt = 0;
			break;
		}
	}
	if (cnt != 0)
	{
		SPDLOG(error, "3 times of login to mt4 server failed.");
		return false;
	}

	//if (RET_OK != m_managerInter->PumpingSwitchEx(NotifyCallBack, 0, this))
	//{
	//	SPDLOG(error, "pumpswitch failed.")
	//	return false;
	//}
		
	return true;
}

int DirectConn::mt4Conn(const char* host)
{
	int cnt = 0;
	while (cnt < 3)
	{
		if (m_managerInter->Connect(host) != RET_OK)
		{
			SPDLOG(error, "try to connect to mt4 server failed---{}", cnt + 1);
			cnt++;
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		else
		{
			cnt = 0;
			break;
		}
	}
	if (cnt != 0)
	{
		SPDLOG(error, "3 times of connecting to mt4 server failed.");
		return RET_NO_CONNECT;
	}
	return RET_OK;
}

bool DirectConn::mt4IsConnected()
{
	if (m_managerInter->IsConnected() != 0)
		return true;
	else
		return false;
}

bool DirectConn::createConnToMT4()
{
	if (mt4Login(std::stod(Config::getInstance().getMT4ConnConf().find(std::move("login"))->second), 
		Config::getInstance().getMT4ConnConf().find(std::move("passwd"))->second.c_str()))
		return true;
	else
		return false;
}

bool DirectConn::heartBeat()
{
	return m_managerInter->Ping();
}

void DirectConn::watchConntoMT4()
{
	std::thread t([&]()
	{
		while (true)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));

			if (!mt4IsConnected())
			{
				Logger::getInstance()->warn("disconnected to mt4. will try to connect to mt4.");
				if (createConnToMT4())
				{
					Logger::getInstance()->info("connect to mt4 success.");
				}
				else
				{
					Logger::getInstance()->info("connect to mt4 failed.");
				}
			}
			else
			{ 
				heartBeat();
			}
		}
	});
	t.detach();
}

ConGroup DirectConn::getGroupCfg(std::string group)
{
	int total = 0;
	ConGroup* cfgGroup = nullptr;
	ConGroup ret = { 0 };
	
	int tryTimes = 0;
	do
	{
		if (mt4IsConnected())
		{
			cfgGroup = m_managerInter->CfgRequestGroup(&total);
			if (total)
			{
				for (int i = 0; i < total; i++)
				{
					if (group.compare(cfgGroup[i].group) == 0)
						ret = cfgGroup[i];
				}
			}
		}
		else
		{
			if (tryTimes > 5)
				break;
			createConnToMT4();
			tryTimes++;
			continue;
		}
	} while (0);

	m_managerInter->MemFree(cfgGroup);
	return ret;
}

bool DirectConn::updateGroupSec(const std::string& group,const std::map<int, ConGroupSec>& cfgGroupSec, std::set<int> index)
{
	ConGroup cfgGroup(std::move(getGroupCfg(group)));
	int size = sizeof(cfgGroup.secgroups) / sizeof(ConGroupSec);

	if (group.compare(cfgGroup.group) == 0)
	{
		for (int i = 0; i < size; i++)
		{
			if (index.find(i) != index.end())
			{
				cfgGroup.secgroups[i] = cfgGroupSec.at(i);
			}
		}
	}
	else
	{
		return false;
	}	

	int res = 0;
	if (RET_OK != (res = m_managerInter->CfgUpdateGroup(&cfgGroup)))
	{
		Logger::getInstance()->error("update group securities failed.{}", m_managerInter->ErrorDescription(res));
		return false;
	}
	return true;
}


bool DirectConn::updateGroupMargins(const std::string& group, const std::map<std::string, ConGroupMargin>& cfgGroupMargin)
{
	ConGroup cfgGroup(std::move(getGroupCfg(group)));
	int oldSize = cfgGroup.secmargins_total;

	std::set<std::string> exclusiveSymbol;
	if (group.compare(cfgGroup.group) == 0)
	{
		for (int i = 0; i < cfgGroup.secmargins_total; i++)
		{
			if (cfgGroupMargin.find(cfgGroup.secmargins[i].symbol) != cfgGroupMargin.end())
			{
				cfgGroup.secmargins[i] = cfgGroupMargin.at(cfgGroup.secmargins[i].symbol);
				exclusiveSymbol.insert(cfgGroup.secmargins[i].symbol);
			}
		}
		for (auto& m : cfgGroupMargin)
		{
			if (exclusiveSymbol.find(m.first) != exclusiveSymbol.end())
				continue;
			cfgGroup.secmargins[oldSize++] = m.second;
		}
	}
	else
	{
		return false;
	}

	int res = 0;
	if (RET_OK != (res = m_managerInter->CfgUpdateGroup(&cfgGroup)))
	{
		Logger::getInstance()->error("update group margins failed.{}", m_managerInter->ErrorDescription(res));
		return false;
	}
	return true;
}
