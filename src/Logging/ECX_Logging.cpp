#include "ECX_Logging.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

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

        ss << std::put_time(&tm, "%d-%m-%Y %H:%M:%S") << ": ";

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

        // Store coloured HTML version
        log[std::this_thread::get_id()].push_back(
            "<span style='color:" + GetHTMLColour(loglevel) + "'>" +
            finalMessage + "</span>"
        );

        // Plain-text version for runtime consumers (debug overlay, etc.) - independent of the
        // HTML buffer's flush/clear cycle.
        m_PlainLog.push_back(finalMessage);
        if (m_PlainLog.size() > kMaxPlainLogEntries)
            m_PlainLog.pop_front();

        // Console output
        std::printf("%s%s%s\n", GetColour(loglevel), finalMessage.c_str(), Reset);

#endif
    }

    std::deque<std::string> ECX_Logger::GetRecentPlainLogs(size_t maxLines) const
    {
        std::scoped_lock<std::mutex> scopedLock(lock);
        if (maxLines >= m_PlainLog.size())
            return m_PlainLog;
        return std::deque<std::string>(m_PlainLog.end() - maxLines, m_PlainLog.end());
    }

    void ECX_Logger::printToFile()
    {
#ifdef LOG_ENABLED
        std::scoped_lock<std::mutex> scopedLock(lock);

        // Check if file exists
        std::ifstream checkFile(outputFilename);
        bool fileExists = checkFile.good();
        checkFile.close();

        std::ofstream out(outputFilename, std::ios::app);
        if (!out.is_open())
        {
            std::cout << "Failed to open " << outputFilename << '\n';
            return;
        }

        // Write HTML header once
        if (!fileExists)
        {
            out << "<!DOCTYPE html>\n<html>\n<head>\n"
                << "<meta charset='UTF-8'>\n"
                << "<title>Log Output</title>\n"
                << "<style>"
                << "body { font-family: monospace; }"
                << "table { border-collapse: collapse; margin-bottom:20px; }"
                << "th, td { border:1px solid #444; padding:4px 8px; }"
                << "th { background:#222; color:white; }"
                << "</style>\n"
                << "</head>\n<body>\n";
        }

        // ===== Session Start =====
        auto startTime = std::time(nullptr);
        auto startTm = *std::localtime(&startTime);

        out << "<hr>\n";
        out << "<h2>Log Session Start: "
            << std::put_time(&startTm, "%d-%m-%Y %H:%M:%S")
            << "</h2>\n";

        // Determine maximum number of rows
        size_t maxRows = 0;
        for (const auto& col : log)
            maxRows = std::max(maxRows, col.second.size());

        if (maxRows > 0)
        {
            out << "<table>\n<tr>";

            // Headers
            for (const auto& header : log)
                out << "<th>" << header.first << "</th>";

            out << "</tr>\n";

            // Rows
            for (size_t row = 0; row < maxRows; ++row)
            {
                out << "<tr>";

                for (const auto& col : log)
                {
                    if (row < col.second.size())
                    {
                        auto it = col.second.begin();
                        std::advance(it, row);
                        out << "<td>" << *it << "</td>";
                    }
                    else
                    {
                        out << "<td></td>";
                    }
                }

                out << "</tr>\n";
            }

            out << "</table>\n";
        }
        else
        {
            out << "<p><em>No log entries.</em></p>\n";
        }

        // ===== Session End =====
        auto endTime = std::time(nullptr);
        auto endTm = *std::localtime(&endTime);

        out << "<h3>Log Session End: "
            << std::put_time(&endTm, "%d-%m-%Y %H:%M:%S")
            << "</h3>\n";

        out.flush();
        out.close();

        // Optional: clear logs after writing
        for (auto& col : log)
            col.second.clear();

#endif
    }

} // namespace LOGGING