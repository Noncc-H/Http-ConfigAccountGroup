#pragma once
#include "event2/http.h"
#include "event2/event.h"
#include "event2/thread.h"
#include "event2/keyvalq_struct.h"
#include "event2/buffer.h"
#include <string>
#include <map>
#include "MT4Module.h"

using STATUS = enum { OK, BAD_URL, BAD_METHOD, PARAM_INVALID, SERVER_ERROR};
using URI = enum {COMMON = 1, PERMISSIONS, ARCHIVING, MARGINS, SECURITIES, SYMBOLS, REPORTS, COMMON_GROUPS, COMMON_SECURITIES, SECURITIES_AUTO_GET, SECURITIES_AUTO_SET};

class HttpServer
{
public:
	HttpServer();
	~HttpServer();

	bool init();
	int  stopServer();
	int  startServer();

	void setMT4Conn(DirectConn conn);

private:
	static void cbFunc(struct evhttp_request *, void *args);
	bool parseReq(struct evhttp_request* req, evhttp_cmd_type& method, std::string& uri, std::map<std::string, std::string>& uriArgs , std::string& body);
	int  handleReq(const evhttp_cmd_type& method, const std::string& uri, const std::map<std::string, std::string>& uriArgs, const std::string& body,std::string& response);

	int getGroupsNames(std::string& response);
	int getSecuritiesNames(std::string& response);

	int setGroupSecurities(const std::string& body, std::string& response);
	int getGroupSecurities(const std::map<std::string, std::string>& uriArgs, std::string& response);

	int setGroupCommon(const std::string& body, std::string& response);
	int getGroupCommon(const std::map<std::string, std::string>& uriArgs, std::string& response);

	int setGroupSymbols(const std::string& body, std::string& response);
	int getGroupSymbols(const std::map<std::string, std::string>& uriArgs, std::string& response);

	int getGroupMargins(const std::map<std::string, std::string>& uriArgs, std::string& response);
	int setGroupMargins(const std::string& body, std::string& response);

	int getGroupArchive(const std::map<std::string, std::string>& uriArgs, std::string& response);
	int setGroupArchive(const std::string& body, std::string& response);

	int getGroupReport(const std::map<std::string, std::string>& uriArgs, std::string& response);
	int setGroupReport(const std::string& body, std::string& response);

	int getGroupPermission(const std::map<std::string, std::string>& uriArgs, std::string& response);
	int setGroupPermission(const std::string& body, std::string& response);


	int getAllGroupsSecurities(std::string& response);  //out
	int setAllGroupsSecurities(std::string& response);
	
	static void readdb(ConGroupSec, int index, std::string, void*);
private:
	struct event_base* m_evBase;
	struct evhttp* m_http;
	std::map<evhttp_cmd_type, std::string> m_httpMethod;
	std::map<std::string, URI> m_uri;
	DirectConn m_mt4Conn;
};