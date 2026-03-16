#include "Logger.h"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

Logger::Logger(const std::string& filePath)
    : m_file(filePath, std::ios::app)
{
}

Logger::~Logger()
{
    if (m_file.is_open())
        m_file.flush();
}

void Logger::Info(const std::string& message)
{
    Write("INFO", message);
}

void Logger::Warn(const std::string& message)
{
    Write("WARN", message);
}

void Logger::Error(const std::string& message)
{
    Write("ERROR", message);
}

void Logger::Write(const std::string& level, const std::string& message)
{
    const std::string line = "[" + Timestamp() + "] [" + level + "] " + message;

    std::lock_guard<std::mutex> lock(m_mutex);

    std::cout << line << std::endl;

    if (m_file.is_open())
    {
        m_file << line << std::endl;
        m_file.flush();
    }
}

std::string Logger::Timestamp() const
{
    const std::time_t now = std::time(nullptr);

    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    std::ostringstream stream;
    stream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}