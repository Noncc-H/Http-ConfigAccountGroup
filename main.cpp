#include <iostream>
#include "HttpServer.h"
#include "Logger.h"
#include "Config.h"
#include "MT4Module.h"
#include <iostream>

int main()
{
	Config::getInstance().readConf("config/app.conf");
	DirectConn mt4Conn;
	if (!mt4Conn.createConnToMT4())
	{
		Logger::getInstance()->error("connect to mt4 failed.");
		std::cout << "connect to mt4 failed." << std::endl;
		return -1;
	}
	else
	{
		Logger::getInstance()->info("connect to mt4 success.");
		std::cout << "connect to mt4 success." << std::endl;
	}

	HttpServer http;
	if (!http.init())
	{
		Logger::getInstance()->error("http server init failed.");
		std::cout << "http server init failed." << std::endl;
		return -1;
	}
	else
	{
		Logger::getInstance()->info("http server init success.");
		std::cout << "http server init success." << std::endl;
	}
	http.setMT4Conn(mt4Conn);
	std::cout << "running..." << std::endl;
	http.startServer();
	return 0;
}