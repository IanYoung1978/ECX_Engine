#include "ECX_Logging.h"
#include <fstream>
#include <iostream>


namespace LOGGING
{
	std::unique_ptr< ECX_Logger> ECX_Logger::instance = nullptr;
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
		case LogLevel::INFORMATION:
			ss << "INFORMATION";
			break;
		case LogLevel::TRIVIAL:
			ss << "TRIVIAL";
			break;
		case LogLevel::SEVERE:
			ss << "SEVERE";
			break;
		default:
			ss << "CRITICAL";
			break;
		}

		ss << ": " << message << std::endl;;
		log[std::this_thread::get_id()].push_back(ss.str());
#endif // LOG_ENABLED
	}
	void ECX_Logger::printToFile()
	{
#ifdef LOG_ENABLED
		size_t numColumns = log.size();
		size_t columnsToLog = numColumns;
		std::fstream file{ outputFilename, file.app };
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
					continue;
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