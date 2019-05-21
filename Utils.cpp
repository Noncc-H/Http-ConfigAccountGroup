#include "Utils.h"
#include <windows.h>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

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


bool Utils::parseFromJsonToSec(std::string json, std::map<int, ConGroupSec>& sec, std::set<int>& index, std::string& group)
{
	using namespace rapidjson;
	Document doc;
	
	if (doc.Parse(json.c_str()).HasParseError())
	{
		return false;
	}

	auto addInt = [&](GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>> &obj, std::string key, int& value)
	{
		if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsInt())
		{
			value = obj[key.c_str()].GetInt();
			return true;
		}
		else
		{
			return false;
		}
	};

	auto addDouble = [&](GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>> &obj, std::string key, double& value)
	{
		if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsDouble())
		{
			value = obj[key.c_str()].GetDouble();
			return true;
		}
		else if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsInt())
		{
			value = obj[key.c_str()].GetInt();
			return true;
		}
		else
			return false;
	};
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

bool Utils::parseFromSecToJson(const std::string& group, ConGroupSec sec[], int size, std::string& json)
{
	if (sec == nullptr)
		return false;

	using namespace rapidjson;
	
	rapidjson::StringBuffer strBuf;
	rapidjson::Writer<rapidjson::StringBuffer> w(strBuf);
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


bool Utils::parseFromJsonToMargins(std::string json, std::map<std::string, ConGroupMargin>& margins, std::string& group)
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

	if (doc.HasMember("GroupMargin") && doc["GroupMargin"].IsArray())
	{
		for (auto& v : doc["GroupMargin"].GetArray())
		{
			ConGroupMargin tmp = { 0 };

			if (v.HasMember("symbol") && v["symbol"].IsString)
			{
				memcpy(tmp.symbol, v["symbol"].GetString(), v["symbol"].Size());
				margins[v["symbol"].GetString()] = tmp;
			}
			else
			{
				return false;
			}

			auto addDouble = [&](GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>> &obj, std::string key, double& value)
			{
				if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsDouble())
				{
					value = obj[key.c_str()].GetDouble();
					return true;
				}
				else if (obj.HasMember(key.c_str()) && obj[key.c_str()].IsInt())
				{
					value = obj[key.c_str()].GetInt();
					return true;
				}
				else
					return false;
			};

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

bool Utils::parseFromMarginToJson(const std::string& group, ConGroupMargin margins[], int size, std::string& json)
{
	if (margins == nullptr)
		return false;

	using namespace rapidjson;

	rapidjson::StringBuffer strBuf;
	rapidjson::Writer<rapidjson::StringBuffer> w(strBuf);
	w.StartObject();

	w.Key("Group");
	w.String(group.c_str());

	w.Key("GroupMargin");
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

