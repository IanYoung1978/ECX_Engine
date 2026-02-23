#include "ECX_Logging.h"
#include <fstream>
#include <iostream>
namespace LOGGING
{
	std::unique_ptr<ECX_Logger> ECX_Logger::instance = nullptr;
}
namespace
{
	const char* Reset = "\033[0m";
	const char* Green = "\033[32m";
	const char* Cyan = "\033[36m";
	const char* Yellow = "\033[33m";
	const char* Red = "\033[31m";
	const char* Magenta = "\033[35m";

	const char* GetColour(LOGGING::LogLevel level)
	{
		using LOGGING::LogLevel;

		switch (level)
		{
		case LogLevel::INFORMATION: return Green;
		case LogLevel::TRIVIAL:     return Cyan;
		case LogLevel::WARNING:     return Yellow;
		case LogLevel::SEVERE:      return Red;
		case LogLevel::CRITICAL:    return Magenta;
		default:                    return Reset;
		}
	}

	std::string GetHTMLColour(LOGGING::LogLevel level)
	{
		using LOGGING::LogLevel;

		switch (level)
		{
		case LogLevel::INFORMATION: return "green";
		case LogLevel::TRIVIAL:     return "deepskyblue";
		case LogLevel::WARNING:     return "orange";
		case LogLevel::SEVERE:      return "red";
		case LogLevel::CRITICAL:    return "purple";
		default:                    return "black";
		}
	}
}
namespace LOGGING
{
	void ECX_Logger::LogMessage(const std::string& message, LogLevel loglevel)
	{
#ifdef LOG_ENABLED
		std::scoped_lock<std::mutex> scopedLock(lock);

		std::stringstream ss;
		auto t = std::time(nullptr);
		auto tm = *std::localtime(&t);

		ss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S") << ": ";

		switch (loglevel)
		{
		case LogLevel::INFORMATION: ss << "INFORMATION"; break;
		case LogLevel::TRIVIAL:     ss << "TRIVIAL"; break;
		case LogLevel::WARNING:     ss << "WARNING"; break;
		case LogLevel::SEVERE:      ss << "SEVERE"; break;
		case LogLevel::CRITICAL:    ss << "CRITICAL"; break;
		default:                    ss << "UNKNOWN"; break;
		}

		ss << ": " << message;

		std::string finalMessage = ss.str();

		// Store clean message for HTML
		log[std::this_thread::get_id()].push_back(
			"<span style='color:" + GetHTMLColour(loglevel) + "'>" +
			finalMessage + "</span>"
		);

		// Print coloured message to console
		printf("%s%s%s\n", GetColour(loglevel), finalMessage.c_str(), Reset);

#endif
	}
	void ECX_Logger::printToFile()
	{
#ifdef LOG_ENABLED
		size_t numColumns = log.size();
		size_t columnsToLog = numColumns;
		std::fstream file{ outputFilename, std::ios::app };
		if (!file.is_open())
		{
			std::cout << "failed to open " << outputFilename << '\n';
			return;
		}
		file << "<!DOCTYPE html><html><head></head><body><table>"; //starting html
		for (auto &header : log)
		{
			file << "<th>" << header.first << "</th>";
		}
		file << std::endl;
		while (columnsToLog > 0)
		{
			file << "<tr>";
			for (auto& c : log)
			{
				if (c.second.empty())
				{
					file << "<td>" << "" << "</td>";
					columnsToLog--;
					break;
				}
				file << "<td>" << c.second.front() <<"</td>";
				c.second.pop_front();
			}
			file << "</tr>"<<std::endl;
		}
		//ending html
		file << "</table></body></html>";
		file.close();
#endif // LOG_ENABLED
	}
}