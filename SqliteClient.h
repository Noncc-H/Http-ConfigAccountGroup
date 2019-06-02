#pragma once
#include "sqlite3.h"
#include <mutex>
#include <vector>
#include <string>
#include <unordered_map>
#include "MT4ManagerAPI.h"

using rCallBack = int(*)(void*, int, char**, char**);
using wCallBack = /*std::function<void(int, int)>;*/void(*)(ConGroupSec, int, std::string, void*);

struct Param
{
	void* func;
	void* args;
};

class SqliteClient
{
public:
	~SqliteClient() = default;
	static SqliteClient& getInstance();

	bool init();
	bool get(wCallBack func, void* param);
	bool add(const int index, const std::string& security, const ConGroup& conGroup);
	bool del(const int ticket);
	void uninit();

	bool getAllRecords(std::vector<std::vector<std::string>>& records);
	bool getCached(std::unordered_map<std::string, std::vector<std::string>>& records);
private:
	SqliteClient()=default;
	
	SqliteClient(const SqliteClient& other) = default;
	SqliteClient& operator= (const SqliteClient& other) = default;
	SqliteClient(SqliteClient&& other) = default;

	bool exec(std::string sql, rCallBack func=nullptr, void* param=nullptr);
	static int my_callback(void *list, int count, char **data, char **columns);

	bool initCached();
	void assemblyInsertSql(const int index, const std::string& security, const ConGroup& conGroup, std::string& sql);
	void assemblyUpdateSql(const int index, const std::string& security, const ConGroup& conGroup, std::string& sql);
	void assemblyCached(const int index, const std::string& security, const ConGroup& conGroup, std::vector<std::string>& cached);
private:
	static SqliteClient *m_self;
	static std::mutex m_mtx;
	sqlite3* m_sqlite3;

	std::unordered_map<std::string, std::vector<std::string>> m_cached;
};