#pragma once
#include <string>
#include <set>
#include <map>
#include <mutex>
#include <unordered_map>
#include "MT4ManagerAPI.h"


struct TradeRecord;
struct ConGroupSec;

class Utils
{
public:
	static Utils& getInstance();

	void  split(const std::string& in, std::vector<std::string>& out, const std::string& delimeter);
	std::string getProjectPath();

	const std::string& getCmdDes(int cmd);
	const std::string& getStateDes(int state);
	const std::string& getReasonDes(int reason);
	const std::string& getActivationDes(int activation);

	bool parseFromJsonToSec(std::string json, std::map<int,ConGroupSec>& sec, std::set<int>& index, std::string& group);
	bool parseFromSecToJson(const std::string& group, ConGroupSec sec[], int size, std::string& json);

private:
	Utils();
	~Utils() = default;

	Utils(const Utils& other) = default;
	Utils& operator= (const Utils& other) = default;

private:
	static Utils* m_self;
	static std::mutex m_mtx;

	std::unordered_map<int, std::string> m_cmd;
	std::unordered_map<int, std::string> m_state;
	std::unordered_map<int, std::string> m_reason;
	std::unordered_map<int, std::string> m_activation;
};