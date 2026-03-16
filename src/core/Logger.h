#pragma once

#include <fstream>
#include <mutex>
#include <string>

class Logger
{
public:
    explicit Logger(const std::string& filePath);
    ~Logger();

    void Info(const std::string& message);
    void Warn(const std::string& message);
    void Error(const std::string& message);

private:
    void Write(const std::string& level, const std::string& message);
    std::string Timestamp() const;

private:
    std::ofstream m_file;
    std::mutex m_mutex;
};