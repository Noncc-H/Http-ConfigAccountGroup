#include "Utils.h"
#include <typeinfo>

Utils* Utils::m_self = nullptr;
std::mutex Utils::m_mtx;

Utils& Utils::getInstance()
{
	if (m_self == nullptr)
	{
		std::lock_guard<std::mutex> lgd(m_mtx);
		if (m_self == nullptr)
			m_self = new Utils();
	}
	return *m_self;
}

Utils::Utils()
{
	m_state[TS_OPEN_NORMAL] = "TS_OPEN_NORMAL";     //open
	m_state[TS_CLOSED_NORMAL] = "TS_CLOSED_NORMAL"; //close
	m_state[TS_CLOSED_PART] = "TS_CLOSED_PART";     //partially close
	m_state[TS_CLOSED_BY] = "TS_CLOSED_BY";         //close by an opposite order
	m_state[TS_DELETED] = "TS_DELETED";             //delete
	
	m_cmd[OP_BUY] = "OP_BUY";
	m_cmd[OP_SELL] = "OP_SELL";
	m_cmd[OP_BUY_LIMIT] = "OP_BUY_LIMIT";
	m_cmd[OP_SELL_LIMIT] = "OP_SELL_LIMIT";
	m_cmd[OP_BUY_STOP] = "OP_BUY_STOP";
	m_cmd[OP_SELL_STOP] = "OP_SELL_STOP";

	m_reason[TR_REASON_CLIENT] = "TR_REASON_CLIENT";
	m_reason[TR_REASON_DEALER] = "TR_REASON_DEALER";
	m_reason[TR_REASON_API] = "TR_REASON_API";
	m_reason[TR_REASON_MOBILE] = "TR_REASON_MOBILE";
	m_reason[TR_REASON_WEB] = "TR_REASON_WEB";

	m_activation[ACTIVATION_NONE] = "ACTIVATION_NONE";
	m_activation[ACTIVATION_SL] = "ACTIVATION_SL";
	m_activation[ACTIVATION_TP] = "ACTIVATION_TP";
	m_activation[ACTIVATION_PENDING] = "ACTIVATION_PENDING";
	m_activation[ACTIVATION_STOPOUT] = "ACTIVATION_STOPOUT";
	m_activation[ACTIVATION_SL_ROLLBACK] = "ACTIVATION_SL_ROLLBACK";
	m_activation[ACTIVATION_TP_ROLLBACK] = "ACTIVATION_TP_ROLLBACK";
	m_activation[ACTIVATION_PENDING_ROLLBACK] = "ACTIVATION_PENDING_ROLLBACK";
	m_activation[ACTIVATION_STOPOUT_ROLLBACK] = "ACTIVATION_STOPOUT_ROLLBACK";
}

void  Utils::split(const std::string& in, std::vector<std::string>& out, const std::string& delimeter)
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


std::string Utils::getProjectPath()
{
	std::string path;
	char p[MAX_PATH] = { 0 };
	if (GetModuleFileName(0, p, MAX_PATH))
	{
		path = p;
		path = path.substr(0, path.rfind("\\"));
		path += "\\";
	}
	return std::move(path);
}


const std::string& Utils::getCmdDes(int cmd)
{
	return m_cmd[cmd];
}
const std::string& Utils::getStateDes(int state)
{
	return m_state[state];
}
const std::string& Utils::getReasonDes(int reason)
{
	return m_reason[reason];
}
const std::string& Utils::getActivationDes(int activation)
{
	return m_activation[activation];
}


bool Utils::parseFromJsonToSec(const std::string json, std::map<int, ConGroupSec>& sec, std::set<int>& index, std::string& group)
{
	using namespace rapidjson;
	Document doc;
	
	if (doc.Parse(json.c_str()).HasParseError())
	{
		return false;
	}

	if (doc.HasMember("Group") && doc["Group"].IsString())
	{
		group = doc["Group"].GetString();
	}
	else
	{
		return false;
	}

	if (doc.HasMember("GroupSec") && doc["GroupSec"].IsArray())
	{
		for (auto& v: doc["GroupSec"].GetArray())
		{
			if (v.HasMember("index") && v["index"].IsInt())
			{
				index.insert(v["index"].GetInt());
			}
			else
			{
				return false;
			}

			ConGroupSec tmp = { 0 };
			sec[v["index"].GetInt()] = tmp;

			if (addInt(v, "show", sec[v["index"].GetInt()].show) &&
				addInt(v, "trade", sec[v["index"].GetInt()].trade) &&
				addInt(v, "execution", sec[v["index"].GetInt()].execution) &&
				addDouble(v, "comm_base", sec[v["index"].GetInt()].comm_base) &&
				addInt(v, "comm_type", sec[v["index"].GetInt()].comm_type) &&
				addInt(v, "comm_lots", sec[v["index"].GetInt()].comm_lots) &&
				addDouble(v, "comm_agent", sec[v["index"].GetInt()].comm_agent) &&
				addInt(v, "comm_agent_type", sec[v["index"].GetInt()].comm_agent_type) &&
				addInt(v, "spread_diff", sec[v["index"].GetInt()].spread_diff) &&
				addInt(v, "lot_min", sec[v["index"].GetInt()].lot_min) &&
				addInt(v, "lot_max", sec[v["index"].GetInt()].lot_max) &&
				addInt(v, "lot_step", sec[v["index"].GetInt()].lot_step) &&
				addInt(v, "ie_deviation", sec[v["index"].GetInt()].ie_deviation) &&
				addInt(v, "confirmation", sec[v["index"].GetInt()].confirmation) &&
				addInt(v, "trade_rights", sec[v["index"].GetInt()].trade_rights) &&
				addInt(v, "ie_quick_mode", sec[v["index"].GetInt()].ie_quick_mode) &&
				addInt(v, "autocloseout_mode", sec[v["index"].GetInt()].autocloseout_mode) &&
				addDouble(v, "comm_tax", sec[v["index"].GetInt()].comm_tax) &&
				addInt(v, "comm_agent_lots", sec[v["index"].GetInt()].comm_agent_lots) &&
				addInt(v, "freemargin_mode", sec[v["index"].GetInt()].freemargin_mode))
			{
				
			}
			else
			{
				return false;
			}
		}
	}
	else
		return false;

	return true;
}

bool Utils::parseFromSecToJson(const std::string& group,const ConGroupSec sec[],const int size, std::string& json)
{
	if (sec == nullptr)
		return false;

	using namespace rapidjson;
	
	rapidjson::StringBuffer strBuf;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> w(strBuf);
	w.StartObject();

	w.Key("Group");
	w.String(group.c_str());

	w.Key("GroupSec");
	w.StartArray();

	for (int i = 0; i < size; i++)
	{
		w.StartObject();
		w.Key("index");
		w.Int(i);

		w.Key("show");
		w.Int(sec[i].show);

		w.Key("trade");
		w.Int(sec[i].trade);

		w.Key("execution");
		w.Int(sec[i].execution);

		w.Key("comm_base");
		w.Double(sec[i].comm_base);

		w.Key("comm_type");
		w.Int(sec[i].comm_type);

		w.Key("comm_lots");
		w.Int(sec[i].comm_lots);

		w.Key("comm_agent");
		w.Double(sec[i].comm_agent);

		w.Key("comm_agent_type");
		w.Int(sec[i].comm_agent_type);

		w.Key("spread_diff");
		w.Int(sec[i].spread_diff);

		w.Key("lot_min");
		w.Int(sec[i].lot_min);

		w.Key("lot_max");
		w.Int(sec[i].lot_max);

		w.Key("lot_step");
		w.Int(sec[i].lot_step);

		w.Key("ie_deviation");
		w.Int(sec[i].ie_deviation);

		w.Key("confirmation");
		w.Int(sec[i].confirmation);

		w.Key("trade_rights");
		w.Int(sec[i].trade_rights);

		w.Key("ie_quick_mode");
		w.Int(sec[i].ie_quick_mode);

		w.Key("autocloseout_mode");
		w.Int(sec[i].autocloseout_mode);

		w.Key("comm_tax");
		w.Double(sec[i].comm_tax);

		w.Key("comm_agent_lots");
		w.Int(sec[i].comm_agent_lots);

		w.Key("freemargin_mode");
		w.Int(sec[i].freemargin_mode);
		w.EndObject();
	}
	w.EndArray();

	w.EndObject();

	json = strBuf.GetString();

	return true;
}


bool Utils::parseFromJsonToSymbol(const std::string json, std::map<std::string, ConGroupMargin>& margins, std::string& group)
{
	using namespace rapidjson;
	Document doc;

	if (doc.Parse(json.c_str()).HasParseError())
	{
		return false;
	}

	if (doc.HasMember("Group") && doc["Group"].IsString())
	{
		group = doc["Group"].GetString();
	}
	else
	{
		return false;
	}

	if (doc.HasMember("GroupSymbol") && doc["GroupSymbol"].IsArray())
	{
		for (auto& v : doc["GroupSymbol"].GetArray())
		{
			ConGroupMargin tmp = { 0 };

			if (v.HasMember("symbol") && v["symbol"].IsString())
			{
				memcpy(tmp.symbol, v["symbol"].GetString(), v["symbol"].Size());
				margins[v["symbol"].GetString()] = tmp;
			}
			else
			{
				return false;
			}

			if (addDouble(v, "swap_short", margins[v["swap_short"].GetString()].swap_short) &&
				addDouble(v, "swap_long", margins[v["swap_long"].GetString()].swap_long) &&
				addDouble(v, "margin_divider", margins[v["margin_divider"].GetString()].margin_divider))
			{

			}
			else
			{
				return false;
			}
		}
	}
	else
		return false;

	return true;
}

bool Utils::parseFromSymbolToJson(const std::string& group, const ConGroupMargin margins[],const int size, std::string& json)
{
	if (margins == nullptr)
		return false;

	using namespace rapidjson;

	rapidjson::StringBuffer strBuf;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> w(strBuf);
	w.StartObject();

	w.Key("Group");
	w.String(group.c_str());

	w.Key("GroupSymbol");
	w.StartArray();

	for (int i = 0; i < size; i++)
	{
		w.StartObject();

		w.Key("symbol");
		w.String(margins[i].symbol);

		w.Key("swap_short");
		w.Double(margins[i].swap_short);

		w.Key("swap_long");
		w.Double(margins[i].swap_long);

		w.Key("margin_divider");
		w.Double(margins[i].margin_divider);

		w.EndObject();
	}

	w.EndArray();

	w.EndObject();

	json = strBuf.GetString();

	return true;
}

bool Utils::parseFromJsonToCommon(const std::string json, GroupCommon& common, std::string& group)
{
	using namespace rapidjson;
	Document doc;

	if (doc.Parse(json.c_str()).HasParseError())
	{
		return false;
	}

	if (doc.HasMember("Group") && doc["Group"].IsString())
	{
		group = doc["Group"].GetString();
	}
	else
	{
		return false;
	}

	if (doc.HasMember("GroupCommon") && doc["GroupCommon"].IsObject())
	{
		Value o = doc["GroupCommon"].GetObject();

		if (addString(o, "group", common.group) &&
			addInt(o, "enable", common.enable) &&
			addInt(o, "otp_mode", common.otp_mode) &&
			addString(o, "company", common.company) &&
			addString(o, "support_page", common.support_page) &&
			addDouble(o, "default_deposit", common.default_deposit) &&
			addString(o, "currency", common.currency) &&
			addInt(o, "default_leverage", common.default_leverage) &&
			addDouble(o, "interastrate", common.interestrate))
		{

		}
		else
		{
			return false;
		}

	}
	else
		return false;

	return true;
}

bool Utils::parseFromCommonToJson(const std::string& group, const GroupCommon common, std::string& json)
{
	using namespace rapidjson;

	rapidjson::StringBuffer strBuf;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> w(strBuf);
	w.StartObject();

	w.Key("Group");
	w.String(group.c_str());

	w.Key("GroupCommon");
	w.StartObject();

	w.Key("group");
	w.String(common.group.c_str());

	w.Key("enable");
	w.Int(common.enable);

	w.Key("otp_mode");
	w.Int(common.otp_mode);

	w.Key("company");
	w.String(common.company.c_str());

	w.Key("support_page");
	w.String(common.support_page.c_str());

	w.Key("default_deposit");
	w.Double(common.default_deposit);

	w.Key("currency");
	w.String(common.currency.c_str());

	w.Key("default_leverage");
	w.Int(common.default_leverage);

	w.Key("interestrate");
	w.Double(common.interestrate);

	w.EndObject();

	w.EndObject();

	json = strBuf.GetString();

	return true;
}


bool Utils::parseFromJsonToMargin(const std::string json, GroupMargin& margin, std::string& group)
{
	using namespace rapidjson;
	Document doc;

	if (doc.Parse(json.c_str()).HasParseError())
	{
		return false;
	}

	if (doc.HasMember("Group") && doc["Group"].IsString())
	{
		group = doc["Group"].GetString();
	}
	else
	{
		return false;
	}

	if (doc.HasMember("GroupMargin") && doc["GroupMargin"].IsObject())
	{
		Value o = doc["GroupMargin"].GetObject();

		if (addInt(o, "margin_call", margin.margin_call) &&
			addInt(o, "margin_stopout", margin.margin_stopout) &&
			addInt(o, "margin_type", margin.margin_type) &&
			addInt(o, "margin_mode", margin.margin_mode) &&
			addDouble(o, "credit", margin.credit) &&
			addInt(o, "stopout_skip_hedged", margin.stopout_skip_hedged) &&
			addInt(o, "hedge_largeleg", margin.hedge_large_leg))
		{

		}
		else
		{
			return false;
		}

	}
	else
		return false;

	return true;
}


bool Utils::parseFromMarginToJson(const std::string& group, const GroupMargin& margin, std::string& json)
{
	using namespace rapidjson;

	rapidjson::StringBuffer strBuf;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> w(strBuf);
	w.StartObject();

	w.Key("Group");
	w.String(group.c_str());

	w.Key("GroupMargin");
	w.StartObject();

	w.Key("margin_call");
	w.Int(margin.margin_call);

	w.Key("margin_stopout");
	w.Int(margin.margin_stopout);

	w.Key("margin_type");
	w.Int(margin.margin_type);

	w.Key("margin_mode");
	w.Int(margin.margin_mode);

	w.Key("credit");
	w.Double(margin.credit);

	w.Key("stopout_skip_hedged");
	w.Double(margin.stopout_skip_hedged);

	w.Key("hedge_largeleg");
	w.Int(margin.hedge_large_leg);

	w.EndObject();

	w.EndObject();

	json = strBuf.GetString();

	return true;
}

bool Utils::parseFromArchiveToJson(const std::string& group, const GroupArchive& archive, std::string& json)
{
	using namespace rapidjson;

	rapidjson::StringBuffer strBuf;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> w(strBuf);
	w.StartObject();

	w.Key("Group");
	w.String(group.c_str());

	w.Key("GroupArchive");
	w.StartObject();

	w.Key("archive_period");
	w.Int(archive.archive_period);

	w.Key("archive_max_balance");
	w.Int(archive.archive_max_balance);

	w.Key("archive_pending_period");
	w.Int(archive.archive_pending_period);

	w.EndObject();

	w.EndObject();

	json = strBuf.GetString();

	return true;
}
bool Utils::parseFromJsonToArchive(const std::string json, GroupArchive& archive, std::string& group)
{
	using namespace rapidjson;
	Document doc;

	if (doc.Parse(json.c_str()).HasParseError())
	{
		return false;
	}

	if (doc.HasMember("Group") && doc["Group"].IsString())
	{
		group = doc["Group"].GetString();
	}
	else
	{
		return false;
	}

	if (doc.HasMember("GroupArchive") && doc["GroupArchive"].IsObject())
	{
		Value o = doc["GroupArchive"].GetObject();

		if (addInt(o, "archive_period", archive.archive_period) &&
			addInt(o, "archive_max_balance", archive.archive_max_balance) &&
			addInt(o, "archive_pending_period", archive.archive_pending_period))
		{

		}
		else
		{
			return false;
		}

	}
	else
		return false;

	return true;
}

bool Utils::parseFromReportToJson(const std::string& group, const GroupReport& report, std::string& json)
{
	using namespace rapidjson;

	rapidjson::StringBuffer strBuf;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> w(strBuf);
	w.StartObject();

	w.Key("Group");
	w.String(group.c_str());

	w.Key("GroupReport");
	w.StartObject();

	w.Key("reports");
	w.Int(report.reports);

	w.Key("copies");
	w.Int(report.copies);

	w.Key("smtp_server");
	w.String(report.smtp_server.c_str());

	w.Key("template_path");
	w.String(report.template_path.c_str());

	w.Key("smtp_login");
	w.String(report.smtp_login.c_str());

	w.Key("smtp_passwd");
	w.String(report.smtp_passwd.c_str());

	w.Key("support_email");
	w.String(report.support_email.c_str());

	w.Key("signature");
	w.String(report.signature.c_str());

	w.EndObject();

	w.EndObject();

	json = strBuf.GetString();

	return true;
}
bool Utils::parseFromJsonToReport(const std::string json, GroupReport& report, std::string& group)
{
	using namespace rapidjson;
	Document doc;

	if (doc.Parse(json.c_str()).HasParseError())
	{
		return false;
	}

	if (doc.HasMember("Group") && doc["Group"].IsString())
	{
		group = doc["Group"].GetString();
	}
	else
	{
		return false;
	}

	if (doc.HasMember("GroupReport") && doc["GroupReport"].IsObject())
	{
		Value o = doc["GroupReport"].GetObject();

		if (addInt(o, "reports", report.reports) &&
			addString(o, "smtp_server", report.smtp_server) &&
			addString(o, "template_path", report.template_path) &&
			addString(o, "smtp_login", report.smtp_login) &&
			addString(o, "smtp_passwd", report.smtp_passwd) &&
			addString(o, "support_email", report.support_email) &&
			addString(o, "signature", report.signature) &&
			addInt(o, "copies", report.copies))
		{

		}
		else
		{
			return false;
		}

	}
	else
		return false;

	return true;
}

bool Utils::parseFromPermissionToJson(const std::string& group, const GroupPermission& permission, std::string& json)
{
	using namespace rapidjson;

	rapidjson::StringBuffer strBuf;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> w(strBuf);
	w.StartObject();

	w.Key("Group");
	w.String(group.c_str());

	w.Key("GroupPermission");
	w.StartObject();

	w.Key("timeout");
	w.Int(permission.timeout);

	w.Key("news");
	w.Int(permission.news);

	w.Key("news_language");
	w.StartArray();
	for (int i = 0; i < 8; i++)
	{
		w.Int(permission.news_language[i]);
	}
	w.EndArray();

	w.Key("news_language_total");
	w.Int(permission.news_language_total);

	w.Key("maxsecurities");
	w.Int(permission.maxsecurities);

	w.Key("use_swap");
	w.Int(permission.use_swap);

	w.Key("rights");
	w.Int(permission.rights);

	w.Key("check_ie_prices");
	w.Int(permission.check_ie_prices);

	w.Key("maxpositions");
	w.Int(permission.maxpositions);

	w.Key("close_reopen");
	w.Int(permission.close_reopen);

	w.Key("hedge_prohibited");
	w.Int(permission.hedge_prohibited);

	w.Key("close_fifo");
	w.Int(permission.close_fifo);

	w.Key("unused_rights");
	w.StartArray();
	for (int i = 0; i < 2; i++)
	{
		w.Int(permission.unused_rights[i]);
	}
	w.EndArray();

	w.Key("securities_hash");
	w.String(permission.securities_hash.c_str());

	w.EndObject();

	w.EndObject();

	json = strBuf.GetString();

	return true;
}
bool Utils::parseFromJsonToPermission(const std::string json, GroupPermission& permission, std::string& group)
{
	using namespace rapidjson;
	Document doc;

	if (doc.Parse(json.c_str()).HasParseError())
	{
		return false;
	}

	if (doc.HasMember("Group") && doc["Group"].IsString())
	{
		group = doc["Group"].GetString();
	}
	else
	{
		return false;
	}

	if (doc.HasMember("GroupPermission") && doc["GroupPermission"].IsObject())
	{
		Value o = doc["GroupPermission"].GetObject();

		int timeout = 0;
		int news = 0;
		int news_language[8] = { 0 };
		int news_language_total = 0;
		int maxsecurities = 0;

		int use_swap = 0;
		int rights = 0;                      // Flags of group permissions
		int check_ie_prices = 0;             // Check the prices of request in the Instant Execution mode
		int maxpositions = 0;                // Limit of open positions
		int close_reopen = 0;                // Partial closure mode
		int hedge_prohibited = 0;            // Hedging not allowed
		int close_fifo = 0;                  // Enable closing by FIFO rule
		int unused_rights[2] = { 0 };          // A reserved field
		std::string securities_hash = "";    // Internal data
		if(addInt(o, "timeout", permission.timeout) && 
			addInt(o, "news", permission.news) &&
			addInt(o, "news_language_total", permission.news_language_total)&&
			addInt(o, "maxsecurities", permission.maxsecurities) &&
			addInt(o, "use_swap", permission.use_swap) &&
			addInt(o, "rights", permission.rights) &&
			addInt(o, "check_ie_prices", permission.check_ie_prices) &&
			addInt(o, "maxpositions", permission.maxpositions) &&
			addInt(o, "close_reopen", permission.close_reopen) &&
			addInt(o, "hedge_prohibited", permission.hedge_prohibited) &&
			addInt(o, "close_fifo", permission.close_fifo) &&
			addString(o, "securities_hash", permission.securities_hash) &&
			addIntArray(o, "news_language", permission.news_language) &&
			addIntArray(o, "unused_rights", permission.unused_rights))
		{

		}
		else
		{
			return false;
		}
	}
	else
		return false;

	return true;
}

bool Utils::parseFromCommmonSecuritesToJson(const ConSymbolGroup securites[], const int size, std::string& json)
{
	rapidjson::StringBuffer strBuf;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> w(strBuf);
	w.StartObject();

	w.Key("total");
	w.Int(size);

	w.Key("securities");
	w.StartArray();
	for (int i = 0; i < size; i++)
	{
		w.StartObject();

		w.Key("name");
		w.String(securites[i].name);

		w.Key("description");
		w.String(securites[i].description);

		w.EndObject();
	}
	w.EndArray();

	w.EndObject();

	json = strBuf.GetString();

	return true;
}

bool Utils::parseFromCommonGroupsToJson(const std::vector<std::string>& groups, std::string& json)
{
	rapidjson::StringBuffer strBuf;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> w(strBuf);
	w.StartObject();

	w.Key("total");
	w.Int(groups.size());

	w.Key("groups");
	w.StartArray();

	for (auto &v : groups)
	{
		w.String(v.c_str());
	}

	w.EndArray();

	w.EndObject();

	json = strBuf.GetString();

	return true;
}

bool Utils::addInt(rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>> &obj, std::string key, int& value)
{
	if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsInt())
	{
		value = obj[key.c_str()].GetInt();
		return true;
	}
	else
		return false;
}
bool Utils::addDouble(rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>> &obj, std::string key, double& value)
{
	if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsDouble())
	{
		value = obj[key.c_str()].GetDouble();
	}
	else if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsInt())
	{
		value = obj[key.c_str()].GetInt();
		return true;
	}
	else
		return false;
}
bool Utils::addString(rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>> &obj, std::string key, std::string& value)
{
	if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsString())
	{
		value = obj[key.c_str()].GetString();
		return true;
	}
	else
		return false;
}

bool Utils::addIntArray(rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>> &obj, std::string key, int* value)
{
	assert(value);
	if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsArray())
	{
		int size = obj[key.c_str()].Size();
		auto a = obj[key.c_str()].GetArray();
		for (int i = 0; i < size; i++)
			value[i] = a[i].GetInt();
	}
	else
		return false;
}

bool Utils::parseFromJsonToAccuntConfiguration(const std::string& json, AccountConfiguration& configuration, std::string login)
{
	using namespace rapidjson;
	Document doc;

	if (doc.Parse(json.c_str()).HasParseError())
	{
		return false;
	}

	if (doc.HasMember("login") && doc["login"].IsString())
	{
		login = doc["Group"].GetString();
	}
	else
	{
		return false;
	}

	if (doc.HasMember("password") && doc["password"].IsString())
	{
		configuration.password = doc["password"].GetString();
	}
	else
	{
		return false;
	}

	if (doc.HasMember("enable_change_password") && doc["enable_change_password"].IsInt())
	{
		configuration.enable_change_password = doc["enable_change_password"].GetInt();
	}
	else
	{
		return false;
	}
	return true;
}