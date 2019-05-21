#include "Config.h"
#include "Logger.h"

std::mutex Logger::m_mtx;
std::shared_ptr<spdlog::logger> Logger::m_logger = nullptr;

std::shared_ptr<spdlog::logger> Logger::getInstance()
{
	if (m_logger == nullptr)
	{
		std::lock_guard<std::mutex> lck(m_mtx);
		if (m_logger == nullptr)
		{
			Config::getInstance().readConf("config/spdlog.conf");

			m_logger = spdlog::create_async<spdlog::sinks::daily_file_sink_mt>(
				Config::getInstance().getLogConf().find("logger")->second, 
				Config::getInstance().getLogConf().find("file")->second,0, 0);

			//m_logger->set_error_handler([](const std::string &msg){spdlog::get("console")->error("***LOGGER ERROR***:{}", msg); });
			
			m_logger->set_level(spdlog::level::from_str(Config::getInstance().getLogConf().find("level")->second));

			m_logger->set_pattern(Config::getInstance().getLogConf().find("pattern")->second);

			m_logger->flush_on(spdlog::level::from_str(Config::getInstance().getLogConf().find("flush")->second));

			return m_logger;
		}
	}
	return m_logger;
}
