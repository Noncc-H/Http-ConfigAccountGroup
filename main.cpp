#include <iostream>
#include "HttpServer.h"
#include "Logger.h"
#include "Config.h"
#include "MT4Module.h"

int main()
{
	Config::getInstance().readConf("config/app.conf");
	DirectConn mt4Conn;
	if (!mt4Conn.createConnToMT4())
	{
		Logger::getInstance()->error("connect to mt4 failed.");
		return -1;
	}
	else
	{
		Logger::getInstance()->info("connect to mt4 success.");
	}

	HttpServer http;
	if (!http.init())
	{
		Logger::getInstance()->error("http server init failed.");
		return -1;
	}
	else
	{
		Logger::getInstance()->info("http server init success.");
	}
	http.setMT4Conn(mt4Conn);

	http.startServer();
	return 0;
}