#pragma once
#include "MT4ManagerAPI.h"
#include "Config.h"
#include <set>

//using URI = enum { COMMON, PERMISSIONS, ARCHIVING, MARGINS, SECURITIES, SYMBOLS, REPORTS };
class DirectConn
{
public:
	DirectConn();
	~DirectConn();

public:
	/************************************************
	** Checks the state of connection to a trading server.
	** Arguments:
	**   none.
	** Returns:
	**   false: disconnected.
	**   true : connected
	*************************************************/
	bool mt4IsConnected();

	bool createConnToMT4();

	ConGroup getGroupCfg(std::string group);
	bool updateGroupSec(const std::string& group, const std::map<int, ConGroupSec>& cfgGroupSec,std::set<int> index);
	
	bool updateGroupMargins(const std::string& group, const std::map<std::string, ConGroupMargin>& cfgGroupMargin);

private:
	/************************************************
	** Connection to a trading server
	** Arguments:
	**   host: trading server's ip and port, format like "localhost:443".
	** Returns:
	**   RET_OK(0): success
	**   RET_ERROR(2): Common error.
    **   RET_INVALID_DATA(3): Invalid information.
    **   RET_TECH_PROBLEM(4): Technical errors on the server.
    **   RET_OLD_VERSION(5): Old terminal version.
    **   RET_NO_CONNECT(6): No connection.
    **   RET_NOT_ENOUGH_RIGHTS(7): Not enough permissions to perform the operation.
    **   RET_TOO_FREQUENT(8): Too frequent requests.
	**   RET_MALFUNCTION(9): Operation cannot be completed.
	**   RET_GENERATE_KEY(10): Key generation is required.
    **   RET_SECURITY_SESSION(11): Connection using extended authentication.
	*************************************************/
	int mt4Conn(const char* host);

	/************************************************
    ** Authentication on the trading server using a manager account
    ** Arguments:
    **   login:  The login of the manager for connection.
    **   passwd:  The password of the manager for connection.
    ** Returns:
    **   true: success.
	**   false : failure.
    *************************************************/
	bool mt4Login(const int login, const char* passwd);

	/************************************************
	** create interface of manager api
	** Arguments:
	**   mt4LibPath: MT4 lib's path
	** Returns:
	**   true: success
	**   false: failure
	*************************************************/
	bool mt4Init();

	void watchConntoMT4();

	bool heartBeat();

private:
	CManagerInterface* m_managerInter;
	CManagerFactory    m_factoryInter;
};

