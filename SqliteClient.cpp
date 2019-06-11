#include "SqliteClient.h"
#include "Utils.h"
#include "Config.h"
#include "Logger.h"

SqliteClient* SqliteClient::m_self = nullptr;
std::mutex SqliteClient::m_mtx;

SqliteClient& SqliteClient::getInstance()
{
	if (nullptr == m_self)
	{
		std::lock_guard<std::mutex> lgd(m_mtx);
		if (nullptr == m_self)
		{
			m_self = new SqliteClient;
			m_self->init();
		}
	}
	return *m_self;
}


bool SqliteClient::init()
{
	std::string path = Utils::getInstance().getProjectPath();
	path += "group.db";

	if (SQLITE_OK == sqlite3_open(path.c_str(), &m_sqlite3))
	{
		std::string sql_createTB = R"(create table group_securities(
               '_index' int,
               'securities' TEXT,
               'group_name' TEXT,
               'show' int,
               'trade' int,
               'confirmation' int,
               'execution' int,
               'spread_diff' int,
               'freemargin_mode' int,
               'ie_deviation' int,
               'ie_quick_mode' int,
               'lot_min' int,
               'lot_max' int,
               'lot_step' int,
               'comm_base' double,
               'comm_type' int,
               'comm_lots' int,
               'comm_agent' int,
               'comm_agent_type' double,
               'comm_tax' double,
               'comm_agent_lots' int,
               'trade_rights' int,
               'autocloseout_mode' int
               );)";
		if (!exec(sql_createTB))
		{
			return initCached();
		}
	}
	else
	{
		return false;
	}
}

void SqliteClient::uninit()
{
	sqlite3_close(m_sqlite3);
}

bool SqliteClient::get( wCallBack func, void* param)
{
	std::string sql = R"(SELECT * FROM "group_securities")";

	Param  p = {nullptr, nullptr};
	p.func = (void*)func;
	p.args = (void*)param;
	return exec(sql, SqliteClient::my_callback, (void*)&p);
}

bool  SqliteClient::getCached(std::unordered_map<std::string, std::vector<std::string>>& records)
{
	records = m_cached;
	return true;
}

bool SqliteClient::getAllRecords(std::vector<std::vector<std::string>>& records)
{
	std::string sql = R"(SELECT * FROM "group_securities")";

	char **recordsTmp = NULL;
	int row;
	int column;
	char* errMSg = nullptr;

	int rc = sqlite3_get_table(m_sqlite3, sql.c_str(), &recordsTmp, &row, &column, &errMSg);
	if (rc != SQLITE_OK) 
	{
		Logger::getInstance()->error("sqlite exec failed. {}", errMSg==nullptr ? "" : errMSg);
		sqlite3_free(errMSg);
		return false;
	}

	for (int i = 1; i < row; i++)
	{
		std::vector<std::string> c;
		for (int j = 0; j < column; j++)
		{
			c.push_back(recordsTmp[i*column+j]);
		}
		records.push_back(c);
		m_cached[c.at(2) + c.at(0)] = c;
	}
	sqlite3_free_table(recordsTmp);
	return true;
}

bool SqliteClient::add(const int index, const std::string& security, const ConGroup& conGroup)
{
	std::string checkKey = std::string(conGroup.group) + std::to_string(index);

	if (m_cached.find(checkKey) != m_cached.end())
	{
		std::vector<std::string> cached;
		assemblyCached(index, security, conGroup, cached);
		m_cached[checkKey] = cached;

		std::string updateSql;
		assemblyUpdateSql(index, security, conGroup, updateSql);
		return exec(updateSql);
	}
	else
	{
		std::vector<std::string> cached;
		assemblyCached(index, security, conGroup, cached);
		m_cached[checkKey] = cached;

		std::string insertSql;
		assemblyInsertSql(index, security, conGroup, insertSql);
		return exec(insertSql);
	}	
}

bool SqliteClient::del(const int ticket)
{
	std::string sql = R"(delete from order_record where ticket=)";
	sql += std::to_string(ticket);

	return exec(sql);
}

bool SqliteClient::exec(std::string sql, rCallBack func, void* param)
{
	char* errmsg = nullptr;
	int res = sqlite3_exec(m_sqlite3, sql.c_str(), func, param, &errmsg);
	if (res != SQLITE_OK)
	{
		Logger::getInstance()->info("sql :{}", sql);
		Logger::getInstance()->error("sqlite exec failed. {}", std::string(errmsg==nullptr?"":errmsg));
		//sqlite3_close(m_sqlite3);
		sqlite3_free(errmsg);
		return false;
	}
	return true;
}

int SqliteClient::my_callback(void *list, int count, char **data, char **columns)
{
	Param f = *(Param*)list;
	
	wCallBack cb = (wCallBack)f.func;
	ConGroupSec sec = {0};
	int index;
	std::string groupname;
	for (int i = 0; i < count; i++)
	{
		if (strcmp(columns[i], "_index") == 0)
			index = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "group_name") == 0)
			groupname = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "show") == 0)
			sec.show = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "trade") == 0)
			sec.trade = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "confirmation") == 0)
			sec.confirmation = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "execution") == 0)
			sec.execution = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "spread_diff") == 0)
			sec.spread_diff = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "freemargin_mode") == 0)
			sec.freemargin_mode = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "ie_deviation") == 0)
			sec.ie_deviation = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "ie_quick_mode") == 0)
			sec.ie_quick_mode = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "lot_min") == 0)
			sec.lot_min = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "lot_max") == 0)
			sec.lot_max = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "log_step") == 0)
			sec.lot_step = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "comm_base") == 0)
			sec.comm_base = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "comm_type") == 0)
			sec.comm_type = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "comm_lots") == 0)
			sec.comm_lots = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "comm_agent") == 0)
			sec.comm_agent = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "comm_agent_type") == 0)
			sec.comm_agent_type = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "comm_tax") == 0)
			sec.comm_tax = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "trade_rights") == 0)
			sec.trade_rights = std::stoi(std::string(data[i]));
		if (strcmp(columns[i], "autocloseout_mode") == 0)
			sec.autocloseout_mode = std::stoi(std::string(data[i]));

		cb(sec,index, groupname, f.args);
	}
	return 0;
}

void SqliteClient::assemblyInsertSql(const int index, const std::string& security, const ConGroup& conGroup, std::string& sql)
{
	//(_index,securities, group_name,show ,trade,confirmation,execution,spread_diff,freemargin_mode,ie_deviation,ie_quick_mode,
	//lot_min, lot_max, lot_step, comm_base, comm_type, comm_lots, comm_agent, comm_agent_type, comm_tax, comm_agent_lots, trade_rights, autocloseout_mode)
	sql = R"(insert into group_securities values()";
	sql += std::to_string(index) + ",";
	sql += "\'" + security + "\'" + ",";
	sql += "\'" + std::string(conGroup.group) + "\'" + ",";
	sql += std::to_string(conGroup.secgroups[index].show) + ",";
	sql += std::to_string(conGroup.secgroups[index].trade) + ",";
	sql += std::to_string(conGroup.secgroups[index].confirmation) + ",";
	sql += std::to_string(conGroup.secgroups[index].execution) + ",";
	sql += std::to_string(conGroup.secgroups[index].spread_diff) + ",";
	sql += std::to_string(conGroup.secgroups[index].freemargin_mode) + ",";
	sql += std::to_string(conGroup.secgroups[index].ie_deviation) + ",";
	sql += std::to_string(conGroup.secgroups[index].ie_quick_mode) + ",";
	sql += std::to_string(conGroup.secgroups[index].lot_min) + ",";
	sql += std::to_string(conGroup.secgroups[index].lot_max) + ",";
	sql += std::to_string(conGroup.secgroups[index].lot_step) + ",";
	sql += std::to_string(conGroup.secgroups[index].comm_base) + ",";
	sql += std::to_string(conGroup.secgroups[index].comm_type) + ",";
	sql += std::to_string(conGroup.secgroups[index].comm_lots) + ",";
	sql += std::to_string(conGroup.secgroups[index].comm_agent) + ",";
	sql += std::to_string(conGroup.secgroups[index].comm_agent_type) + ",";
	sql += std::to_string(conGroup.secgroups[index].comm_tax) + ",";
	sql += std::to_string(conGroup.secgroups[index].comm_agent_lots) + ",";
	sql += std::to_string(conGroup.secgroups[index].trade_rights) + ",";
	sql += std::to_string(conGroup.secgroups[index].autocloseout_mode) + ");";
}
void SqliteClient::assemblyUpdateSql(const int index, const std::string& security, const ConGroup& conGroup, std::string& sql)
{
	sql = R"(update group_securities set )";
	sql += "show=" + std::to_string(conGroup.secgroups[index].show) + ",";
	sql += "trade=" + std::to_string(conGroup.secgroups[index].trade) + ",";
	sql += "confirmation=" + std::to_string(conGroup.secgroups[index].confirmation) + ",";
	sql += "execution=" + std::to_string(conGroup.secgroups[index].execution) + ",";
	sql += "spread_diff=" + std::to_string(conGroup.secgroups[index].spread_diff) + ",";
	sql += "freemargin_mode=" + std::to_string(conGroup.secgroups[index].freemargin_mode) + ",";
	sql += "ie_deviation=" + std::to_string(conGroup.secgroups[index].ie_deviation) + ",";
	sql += "ie_quick_mode=" + std::to_string(conGroup.secgroups[index].ie_quick_mode) + ",";
	sql += "lot_min=" + std::to_string(conGroup.secgroups[index].lot_min) + ",";
	sql += "lot_max=" + std::to_string(conGroup.secgroups[index].lot_max) + ",";
	sql += "lot_step=" + std::to_string(conGroup.secgroups[index].lot_step) + ",";
	sql += "comm_base=" + std::to_string(conGroup.secgroups[index].comm_base) + ",";
	sql += "comm_type=" + std::to_string(conGroup.secgroups[index].comm_type) + ",";
	sql += "comm_lots=" + std::to_string(conGroup.secgroups[index].comm_lots) + ",";
	sql += "comm_agent=" + std::to_string(conGroup.secgroups[index].comm_agent) + ",";
	sql += "comm_agent_type=" + std::to_string(conGroup.secgroups[index].comm_agent_type) + ",";
	sql += "comm_tax=" + std::to_string(conGroup.secgroups[index].comm_tax) + ",";
	sql += "comm_agent_lots=" + std::to_string(conGroup.secgroups[index].comm_agent_lots) + ",";
	sql += "trade_rights=" + std::to_string(conGroup.secgroups[index].trade_rights) + ",";
	sql += "autocloseout_mode=" + std::to_string(conGroup.secgroups[index].autocloseout_mode);
	sql += " where _index=" + std::to_string(index) + " and ";
	sql += "group_name=" + std::string("\'") + std::string(conGroup.group) + "\'" + ";";
}

void SqliteClient::assemblyCached(const int index, const std::string& security, const ConGroup& conGroup, std::vector<std::string>& cached)
{
	cached.push_back(std::to_string(index));
	cached.push_back(security);
	cached.push_back(conGroup.group);
	cached.push_back(std::to_string(conGroup.secgroups->show));
	cached.push_back(std::to_string(conGroup.secgroups->trade));
	cached.push_back(std::to_string(conGroup.secgroups->confirmation));
	cached.push_back(std::to_string(conGroup.secgroups->execution));
	cached.push_back(std::to_string(conGroup.secgroups->spread_diff));
	cached.push_back(std::to_string(conGroup.secgroups->freemargin_mode));
	cached.push_back(std::to_string(conGroup.secgroups->ie_deviation));
	cached.push_back(std::to_string(conGroup.secgroups->ie_quick_mode));
	cached.push_back(std::to_string(conGroup.secgroups->lot_min));
	cached.push_back(std::to_string(conGroup.secgroups->lot_max));
	cached.push_back(std::to_string(conGroup.secgroups->lot_step));
	cached.push_back(std::to_string(conGroup.secgroups->comm_base));
	cached.push_back(std::to_string(conGroup.secgroups->comm_type));
	cached.push_back(std::to_string(conGroup.secgroups->comm_lots));
	cached.push_back(std::to_string(conGroup.secgroups->comm_agent));
	cached.push_back(std::to_string(conGroup.secgroups->comm_agent_type));
	cached.push_back(std::to_string(conGroup.secgroups->comm_tax));
	cached.push_back(std::to_string(conGroup.secgroups->comm_agent_lots));
	cached.push_back(std::to_string(conGroup.secgroups->trade_rights));
	cached.push_back(std::to_string(conGroup.secgroups->autocloseout_mode));
}

bool SqliteClient::initCached()
{
	std::string sql = R"(SELECT * FROM "group_securities")";

	char **recordsTmp = NULL;
	int row;
	int column;
	char* errMSg = nullptr;

	int rc = sqlite3_get_table(m_sqlite3, sql.c_str(), &recordsTmp, &row, &column, &errMSg);
	if (rc != SQLITE_OK)
	{
		Logger::getInstance()->error("sqlite exec failed. {}", errMSg == nullptr ? "" : errMSg);
		sqlite3_free(errMSg);
		return false;
	}

	for (int i = 1; i < row; i++)
	{
		std::vector<std::string> c;
		for (int j = 0; j < column; j++)
		{
			c.push_back(recordsTmp[i*column + j]);
		}
		m_cached[c.at(2) + c.at(0)] = c;
	}
	sqlite3_free_table(recordsTmp);
	return true;
}