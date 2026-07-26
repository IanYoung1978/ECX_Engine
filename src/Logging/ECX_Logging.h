#pragma once
#include <map>
#include <deque>
#include <string>
#include <mutex>
#include <memory>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <vector>
#include <thread>
namespace LOGGING
{
	enum class LogLevel
	{
		INFORMATION,
		TRIVIAL,
		SEVERE,
		WARNING,
		CRITICAL
	};

	class ECX_Logger
	{
	private:
		ECX_Logger() { ; }
		static std::unique_ptr<ECX_Logger> instance;
		std::map<std::thread::id, std::deque<std::string>> log;
		// Flat (not per-thread), plain-text (no HTML span wrapper) recent-message buffer for
		// runtime consumers (e.g. the debug telemetry overlay) - populated alongside `log` but
		// independent of it, so reading recent messages never depends on/triggers printToFile()'s
		// flush-and-clear cycle.
		std::deque<std::string> m_PlainLog;
		static constexpr size_t kMaxPlainLogEntries = 200;
		mutable std::mutex lock;
		std::string outputFilename;
	public:
		~ECX_Logger() { ; }
		static std::unique_ptr<ECX_Logger>& GetInstance()
		{
			if (instance == nullptr)
			{
				instance = std::unique_ptr <ECX_Logger>(new ECX_Logger());
			}
			return instance;
		}
		void LogMessage(const std::string& message, LogLevel loglevel);
		void setOutputFilename(const std::string& output)
		{
			outputFilename = output;
		}
		void printToFile();
		std::deque<std::string> GetRecentPlainLogs(size_t maxLines) const;
	};
	
}