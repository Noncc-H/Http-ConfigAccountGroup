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
using URI = enum {COMMON, PERMISSIONS, ARCHIVING, MARGINS, SECURITIES, SYMBOLS, REPORTS};

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
private:
	struct event_base* m_evBase;
	struct evhttp* m_http;
	std::map<evhttp_cmd_type, std::string> m_httpMethod;
	std::map<std::string, URI> m_uri;
	DirectConn m_mt4Conn;
};