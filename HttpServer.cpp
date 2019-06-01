#include "HttpServer.h"
#include "Logger.h"
#include "Config.h"
#include "Utils.h"
#include <map>
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

HttpServer::HttpServer()
{
	m_httpMethod = { { EVHTTP_REQ_GET, "GET" },
	{ EVHTTP_REQ_POST , "POST" },
	{ EVHTTP_REQ_PUT , "PUT" },
	{ EVHTTP_REQ_DELETE , "DELETE" },
	{ EVHTTP_REQ_PATCH , "PATCH" },
	{ EVHTTP_REQ_OPTIONS , "OPTIONS" },
	{ EVHTTP_REQ_HEAD , "HEAD" },
	{ EVHTTP_REQ_TRACE , "TRACE" },
	{ EVHTTP_REQ_CONNECT , "CONNECT" } };


	std::string common = Config::getInstance().getHTTPConf().find("account-group-common")->second;
	std::string permissions = Config::getInstance().getHTTPConf().find("account-group-permissions")->second;
	std::string archiving = Config::getInstance().getHTTPConf().find("account-group-archiving")->second;
	std::string margins = Config::getInstance().getHTTPConf().find("account-group-margins")->second;
	std::string securities = Config::getInstance().getHTTPConf().find("account-group-securities")->second;
	std::string symbols = Config::getInstance().getHTTPConf().find("account-group-symbols")->second;
	std::string reports = Config::getInstance().getHTTPConf().find("account-group-reports")->second;
	std::string common_groups = Config::getInstance().getHTTPConf().find("common-account-groups")->second;
	std::string common_securities = Config::getInstance().getHTTPConf().find("common-symbol-groups")->second;
	m_uri = { {common, COMMON},
	{permissions, PERMISSIONS},
	{archiving, ARCHIVING},
	{margins, MARGINS},
	{securities, SECURITIES},
	{symbols, SYMBOLS},
	{reports, REPORTS},
	{common_groups, COMMON_GROUPS},
	{common_securities, COMMON_SECURITIES} };
}

HttpServer::~HttpServer()
{

}

void HttpServer::setMT4Conn(DirectConn conn)
{
	m_mt4Conn = conn;
}

bool HttpServer::init()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	evthread_use_windows_threads();

	m_evBase = event_base_new();
	if (nullptr == m_evBase)
	{
		Logger::getInstance()->error("event_base_new failed.");
		return false;
	}

	m_http = evhttp_new(m_evBase);
	if (nullptr == m_http)
	{
		Logger::getInstance()->error("evhttp_new failed.");
		event_base_free(m_evBase);
		m_evBase = nullptr;
		return false;
	}

	evhttp_set_gencb(m_http, HttpServer::cbFunc, this);

	std::string port = Config::getInstance().getHTTPConf().find("port")->second;
	struct evhttp_bound_socket *handle = evhttp_bind_socket_with_handle(m_http, "0.0.0.0", std::stoi(port));

	if (nullptr == handle)
	{
		Logger::getInstance()->error("evhttp_bind_socket failed.");
		evhttp_free(m_http);
		m_http = nullptr;
		event_base_free(m_evBase);
		m_evBase = nullptr;
		return false;
	}
	Logger::getInstance()->info("http server listen port {}", port);
	return true;
}

void HttpServer::cbFunc(struct evhttp_request *req, void *args)
{
	HttpServer* pSelf = (HttpServer *)args;
	evbuffer* evb = nullptr;

	std::string strMessage;
	std::map<std::string, std::string> mapParams;
	evhttp_cmd_type eRequestType;

	std::string uri;
	evhttp_cmd_type method;

	std::map<std::string, std::string> uriArgs;
	std::string body;


	int res = 0;
	std::string response;
	if (pSelf->parseReq(req, method, uri, uriArgs, body))
	{
		res = pSelf->handleReq(method, uri, uriArgs, body, response);
	}
	else
	{
		response = R"({"message":"http service parse failed"})";
	}

	rapidjson::StringBuffer sb;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> w(sb);
	w.StartObject();

	w.Key("code");
	w.Int(res);

	w.Key("message");
	w.String(response.c_str());

	w.EndObject();

	std::string resp = sb.GetString();

	//设置返回消息
	evb = evbuffer_new();
	auto oh = evhttp_request_get_output_headers(req);
	evhttp_add_header(oh, "Content-type", "application/json; charset=UTF-8");
	evhttp_add_header(oh, "Connection", "Close");
	evhttp_add_header(oh, "Content-length", std::to_string(resp.size()).c_str());

	evbuffer_add_printf(evb, "%s", resp.c_str());
	Logger::getInstance()->info("response: {}", resp);

	switch (res)
	{
	case STATUS::OK:
	{
		evhttp_send_reply(req, 200, "OK", evb);
		Logger::getInstance()->info("response ok.");
	}
	break;
	case BAD_URL:
	{
		evhttp_send_reply(req, 404, "Not Found", evb);
		Logger::getInstance()->error("response: bad url！");
	}
	break;
	case BAD_METHOD:
	{
		evhttp_send_reply(req, 405, "Method Not Allowed", evb);
		Logger::getInstance()->error("response: method not allowed！");
	}
	break;
	case PARAM_INVALID://处理失败
	{
		evhttp_send_reply(req, 400, "Bad Request", evb);//此种情况可以将请求下发到dbcache，处理结果根据返回值判断
		Logger::getInstance()->error("response: bad request！");
	}
	break;
	case SERVER_ERROR:
	{
		evhttp_send_reply(req, 500, "Internal Server Error", evb);
		Logger::getInstance()->error("response: internal server error！");
	}
	break;
	default:
	{
		evhttp_send_reply(req, 500, "Internal Server Error", evb);
		Logger::getInstance()->error("response: internal server error!");
	}
	break;
	}

	if (evb)
	{
		evbuffer_free(evb);
	}
}

int  HttpServer::startServer()
{
	Logger::getInstance()->info("http server start...");
	int exitCode = event_base_dispatch(m_evBase);
	if (exitCode < 0)
	{
		Logger::getInstance()->error("event_base_dispath exit occure error.");
	}
	evhttp_free(m_http);
	event_base_free(m_evBase);

	m_http = nullptr;
	m_evBase = nullptr;
	return exitCode;
}

int HttpServer::stopServer()
{
	return event_base_loopbreak(m_evBase);
}

bool HttpServer::parseReq(struct evhttp_request* req, evhttp_cmd_type& method, std::string& uri, std::map<std::string, std::string>& uriArgs, std::string& body)
{
	evkeyvalq querykvs;
	struct evhttp_uri* pDecodedUrl = nullptr;
	char *pDecodedPath = nullptr;
	evbuffer *evbuf = nullptr;
	bool bRes = true;
	do
	{
		method = evhttp_request_get_command(req);

		const char *pUri = evhttp_request_get_uri(req);

		pDecodedUrl = evhttp_uri_parse(pUri);
		if (!pDecodedUrl)
		{
			evhttp_send_error(req, HTTP_BADREQUEST, 0);
			bRes = false;
			break;
		}
		const char *pPath = evhttp_uri_get_path(pDecodedUrl);
		if (nullptr == pPath)
		{
			pPath = "/";
		}

		uri = pPath;

		pDecodedPath = evhttp_uridecode(pPath, 0, nullptr);
		if (!pDecodedPath)
		{
			evhttp_send_error(req, 404, 0);
			bRes = false;
			break;
		}

		querykvs.tqh_first = nullptr;
		const char *pQueryStr = evhttp_uri_get_query(pDecodedUrl);
		if (pQueryStr)
		{
			evhttp_parse_query_str(pQueryStr, &querykvs);
		}
		for (evkeyval *it = querykvs.tqh_first; it != nullptr; it = it->next.tqe_next)
		{
			uriArgs[it->key] = it->value;
		}
		//clear memory
		evhttp_clear_headers(&querykvs);

		evbuf = evhttp_request_get_input_buffer(req);
		while (evbuffer_get_length(evbuf) > 0)
		{
			char cbuf[1024];
			memset(cbuf, 0, sizeof(cbuf));
			int n = evbuffer_remove(evbuf, cbuf, sizeof(cbuf) - 1);
			if (n > 0)
			{
				body.append(cbuf, n);
			}
		}
		Logger::getInstance()->info("request method:{}, uri:{}, body:{}", m_httpMethod[method], pUri, body);
	} while (0);

	if (pDecodedUrl)
	{
		evhttp_uri_free(pDecodedUrl);
	}
	if (pDecodedPath)
	{
		free(pDecodedPath);
	}
	return bRes;
}

int HttpServer::handleReq(const evhttp_cmd_type& method, const std::string& uri, const std::map<std::string, std::string>& uriArgs, const std::string& body, std::string& response)
{
	int res = OK;
	if (method == EVHTTP_REQ_POST)
	{
		switch (m_uri[uri])
		{
		case COMMON:
			res = setGroupCommon(body, response);
			break;
		case PERMISSIONS:
			res = setGroupPermission(body, response);
			break;
		case ARCHIVING:
			res = setGroupArchive(body, response);
			break;
		case MARGINS:
			res = setGroupMargins(body, response);
			break;
		case SECURITIES:
			res = setGroupSecurities(body, response);
			break;
		case SYMBOLS:
			res = setGroupSymbols(body, response);
			break;
		case REPORTS:
			res = setGroupReport(body, response);
			break;
		default:
			response = "bad url";
			res = BAD_URL;
			break;
		}
	}
	else if (method == EVHTTP_REQ_GET)
	{
		switch (m_uri[uri])
		{
		case COMMON:
			res = getGroupCommon(uriArgs, response);
			break;
		case PERMISSIONS:
			res = getGroupPermission(uriArgs, response);
			break;
		case ARCHIVING:
			res = getGroupArchive(uriArgs, response);
			break;
		case MARGINS:
			res = getGroupMargins(uriArgs, response);
			break;
		case SECURITIES:
			res = getGroupSecurities(uriArgs, response);
			break;
		case SYMBOLS:
			res = getGroupSymbols(uriArgs, response);
			break;
		case REPORTS:
			res = getGroupReport(uriArgs, response);
			break;
		case COMMON_GROUPS:
			res = getGroupsNames(response);
			break;
		case COMMON_SECURITIES:
			res = getSecuritiesNames(response);
			break;
		default:
			response = "bad url";
			res = BAD_URL;
			break;
		}
	}
	else
	{
		response = "not support the request method, only can ues get and post for now.";
		res = BAD_METHOD;
	}

	return res;
}

int HttpServer::setGroupSecurities(const std::string& body, std::string& response)
{
	int res = 0;
	std::map<int, ConGroupSec> sec;
	std::set<int> index;
	std::string group;
	if (Utils::getInstance().parseFromJsonToSec(body, sec, index, group))
	{
		if (m_mt4Conn.updateGroupSec(group, sec, index))
		{
			Logger::getInstance()->info("update group securities success.");
			response = "update group securities success.";
		}
		else
		{
			response = "update group securities failed.";
			res = SERVER_ERROR;
		}
	}
	else
	{
		response = "param invalid.please check and try again.";
		res = PARAM_INVALID;
	}
	return res;
}

int HttpServer::setGroupCommon(const std::string& body, std::string& response)
{
	int res = 0;
	GroupCommon groupCommon = { 0 };
	std::string group;
	if (Utils::getInstance().parseFromJsonToCommon(body, groupCommon, group))
	{
		if (m_mt4Conn.updateGroupCommon(group, groupCommon))
		{
			Logger::getInstance()->info("update group common success.");
			response = "update group common success.";
		}
		else
		{
			response = "update group common failed.";
			res = SERVER_ERROR;
		}
	}
	else
	{
		response = "param invalid.please check and try again.";
		res = PARAM_INVALID;
	}
	return res;
}

int HttpServer::setGroupSymbols(const std::string& body, std::string& response)
{
	int res = 0;
	std::map<std::string, ConGroupMargin> margins;
	std::string group;
	if (Utils::getInstance().parseFromJsonToSymbol(body, margins, group))
	{
		if (m_mt4Conn.updateGroupSymbol(group, margins))
		{
			Logger::getInstance()->info("update group securities success.");
			response = "update group securities success.";
		}
		else
		{
			response = "update group securities failed.";
			res = SERVER_ERROR;
		}
	}
	else
	{
		response = "param invalid.please check and try again.";
		res = PARAM_INVALID;
	}
	return res;
}

int HttpServer::getGroupCommon(const std::map<std::string, std::string>& uriArgs, std::string& response)
{
	int res = 0;
	if (uriArgs.find("Group") == uriArgs.end())
	{
		response = "param invalid.please check and try again.";
		res = PARAM_INVALID;
	}
	GroupCommon groupComm;//default constructor
	groupComm = m_mt4Conn.getGroupCommon(uriArgs.at("Group"));
	if (Utils::getInstance().parseFromCommonToJson(uriArgs.at("Group"), groupComm, response))
	{
		Logger::getInstance()->info("serialize group common success.");
	}
	else
	{
		response = "serialize group common failed.";
		res = SERVER_ERROR;
	}
	return res;
}

int HttpServer::getGroupSecurities(const std::map<std::string, std::string>& uriArgs, std::string& response)
{
	int res = 0;
	ConGroup conGroup = { 0 };
	if (uriArgs.find("Group") == uriArgs.end())
	{
		response = "param invalid.please check and try again.";
		res = PARAM_INVALID;
		return res;
	}
	conGroup = m_mt4Conn.getGroupCfg(uriArgs.at("Group"));
	int size = sizeof(conGroup.secgroups) / sizeof(ConGroupSec);
	if (Utils::getInstance().parseFromSecToJson(conGroup.group, conGroup.secgroups, size, response))
	{
		Logger::getInstance()->info("serialize group securities success.");
	}
	else
	{
		response = "serialize group securities failed.";
		res = SERVER_ERROR;
	}
	return res;
}

int HttpServer::getGroupSymbols(const std::map<std::string, std::string>& uriArgs, std::string& response)
{
	int res = 0;
	ConGroup conGroup = { 0 };
	if (uriArgs.find("Group") == uriArgs.end())
	{
		response = "param invalid.please check and try again.";
		res = PARAM_INVALID;
		return res;
	}
	conGroup = m_mt4Conn.getGroupCfg(uriArgs.at("Group"));
	int size = conGroup.secmargins_total;
	if (Utils::getInstance().parseFromSymbolToJson(conGroup.group, conGroup.secmargins, size, response))
	{
		Logger::getInstance()->info("serialize group margins success.");
	}
	else
	{
		response = "serialize group margins failed.";
		res = SERVER_ERROR;
	}
	return res;
}

int HttpServer::getGroupsNames(std::string& response)
{
	int res = 0;
	std::vector<std::string> common_groups;
	if (m_mt4Conn.getGroupNames(common_groups))
	{
		if (Utils::getInstance().parseFromCommonGroupsToJson(common_groups, response))
		{
			Logger::getInstance()->info("serialize groups success.");;
		}
		else
		{
			response = "serialize groups failed.";
			res = SERVER_ERROR;
		}
	}
	else
	{
		response = "get groups failed.";
		res = SERVER_ERROR;
	}
	return res;
}

int HttpServer::getSecuritiesNames(std::string& response)
{
	int res = 0;
	ConSymbolGroup securities[MAX_SEC_GROUPS] = { 0 };
	if (0 == m_mt4Conn.getSecuritiesNames(securities))
	{
		int size = sizeof(securities) / sizeof(ConSymbolGroup);
		if (Utils::getInstance().parseFromCommmonSecuritesToJson(securities, size, response))
		{
			Logger::getInstance()->info("serialize securities success.");;
		}
		else
		{
			response = "serialize securities failed.";
			res = SERVER_ERROR;
		}
	}
	else
	{
		response = "get securites failed.";
		res = SERVER_ERROR;
	}
	return res;
}

int HttpServer::getGroupMargins(const std::map<std::string, std::string>& uriArgs, std::string& response)
{
	int res = 0;
	GroupMargin margin;
	if (uriArgs.find("Group") == uriArgs.end())
	{
		response = "param invalid.please check and try again.";
		res = PARAM_INVALID;
		return res;
	}
	margin = m_mt4Conn.getGroupMargin(uriArgs.at("Group"));

	if (Utils::getInstance().parseFromMarginToJson(uriArgs.at("Group"), margin,  response))
	{
		Logger::getInstance()->info("serialize group margins success.");
	}
	else
	{
		response = "serialize group margins failed.";
		res = SERVER_ERROR;
	}
	return res;
}


int HttpServer::getGroupArchive(const std::map<std::string, std::string>& uriArgs, std::string& response)
{
	int res = 0;
	GroupArchive archive;
	if (uriArgs.find("Group") == uriArgs.end())
	{
		response = "param invalid.please check and try again.";
		res = PARAM_INVALID;
		return res;
	}
	archive = m_mt4Conn.getGroupArchive(uriArgs.at("Group"));

	if (Utils::getInstance().parseFromArchiveToJson(uriArgs.at("Group"), archive, response))
	{
		Logger::getInstance()->info("serialize group archive success.");
	}
	else
	{
		response = "serialize group archive failed.";
		res = SERVER_ERROR;
	}
	return res;
}

int HttpServer::getGroupReport(const std::map<std::string, std::string>& uriArgs, std::string& response)
{
	int res = 0;
	GroupReport report;
	if (uriArgs.find("Group") == uriArgs.end())
	{
		response = "param invalid.please check and try again.";
		res = PARAM_INVALID;
		return res;
	}
	report = m_mt4Conn.getGroupReport(uriArgs.at("Group"));

	if (Utils::getInstance().parseFromReportToJson(uriArgs.at("Group"), report, response))
	{
		Logger::getInstance()->info("serialize group report success.");
	}
	else
	{
		response = "serialize group report failed.";
		res = SERVER_ERROR;
	}
	return res;
}

int HttpServer::getGroupPermission(const std::map<std::string, std::string>& uriArgs, std::string& response)
{
	int res = 0;
	GroupPermission permission;
	if (uriArgs.find("Group") == uriArgs.end())
	{
		response = "param invalid.please check and try again.";
		res = PARAM_INVALID;
		return res;
	}
	permission = m_mt4Conn.getGroupPermission(uriArgs.at("Group"));

	if (Utils::getInstance().parseFromPermissionToJson(uriArgs.at("Group"), permission, response))
	{
		Logger::getInstance()->info("serialize group permission success.");
	}
	else
	{
		response = "serialize group permission failed.";
		res = SERVER_ERROR;
	}
	return res;
}

int HttpServer::setGroupMargins(const std::string& body, std::string& response)
{
	int res = 0;
	GroupMargin margins;
	std::string group;
	if (Utils::getInstance().parseFromJsonToMargin(body, margins, group))
	{
		if (m_mt4Conn.updateGroupMargin(group, margins))
		{
			Logger::getInstance()->info("update group margin success.");
			response = "update group margin success.";
		}
		else
		{
			response = "update group margin failed.";
			res = SERVER_ERROR;
		}
	}
	else
	{
		response = "param invalid.please check and try again.";
		res = PARAM_INVALID;
	}
	return res;
}

int HttpServer::setGroupArchive(const std::string& body, std::string& response)
{
	int res = 0;
	GroupArchive archive;
	std::string group;
	if (Utils::getInstance().parseFromJsonToArchive(body, archive, group))
	{
		if (m_mt4Conn.updateGroupArchive(group, archive))
		{
			Logger::getInstance()->info("update group archive success.");
			response = "update group archive success.";
		}
		else
		{
			response = "update group archive failed.";
			res = SERVER_ERROR;
		}
	}
	else
	{
		response = "param invalid.please check and try again.";
		res = PARAM_INVALID;
	}
	return res;
}

int HttpServer::setGroupReport(const std::string& body, std::string& response)
{
	int res = 0;
	GroupReport report;
	std::string group;
	if (Utils::getInstance().parseFromJsonToReport(body, report, group))
	{
		if (m_mt4Conn.upateGroupReport(group, report))
		{
			Logger::getInstance()->info("update group report success.");
			response = "update group report success.";
		}
		else
		{
			response = "update group report failed.";
			res = SERVER_ERROR;
		}
	}
	else
	{
		response = "param invalid.please check and try again.";
		res = PARAM_INVALID;
	}
	return res;
}

int HttpServer::setGroupPermission(const std::string& body, std::string& response)
{
	int res = 0;
	GroupPermission permission;
	std::string group;
	if (Utils::getInstance().parseFromJsonToPermission(body, permission, group))
	{
		if (m_mt4Conn.updateGroupPerssion(group, permission))
		{
			Logger::getInstance()->info("update group permission success.");
			response = "update group permission success.";
		}
		else
		{
			response = "update group permission failed.";
			res = SERVER_ERROR;
		}
	}
	else
	{
		response = "param invalid.please check and try again.";
		res = PARAM_INVALID;
	}
	return res;
}