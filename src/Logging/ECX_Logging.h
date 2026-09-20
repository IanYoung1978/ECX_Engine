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
		// The file-writing half of printToFile(), with no locking of its own - callers hold
		// `lock` (printToFile) or have deliberately given up on getting it (logCrashAndFlush).
		void writeLogFile();
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
		// For a dying process (CRT assertion / unhandled exception - see ECX_CrashHook.h):
		// appends `message` as a CRITICAL entry, then immediately writes the whole log to
		// file. Waits briefly for the log's mutex but proceeds without it if it can't get
		// it - the crashing thread may be the one holding it (e.g. an assertion fired inside
		// LogMessage itself), and hanging there would lose the very report this exists to
		// save. The process is about to die anyway, so best-effort beats deadlock.
		void logCrashAndFlush(const std::string& message);
		std::deque<std::string> GetRecentPlainLogs(size_t maxLines) const;
	};
	
}