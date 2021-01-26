//// https://stackoverflow.com/questions/43732825/use-debug-log-from-c
//
//#include <stdio.h>
//#include <string>
//#include <stdio.h>
//#include <sstream>
//
//#include "Debug.h"
//
//std::vector<PixelsForGlory::Debug::Message> PixelsForGlory::Debug::QueuedMessages;
//
//// Log
//void PixelsForGlory::Debug::Log(const char* message) {
//    std::stringstream ss;
//    ss << message;
//    send_log(ss, LogLevel::Message);
//}
//
//void PixelsForGlory::Debug::Log(const std::string message) {
//    std::stringstream ss;
//    ss << message;
//    send_log(ss, LogLevel::Message);
//}
//
//void PixelsForGlory::Debug::Log(const int message) {
//    std::stringstream ss;
//    ss << message;
//    send_log(ss, LogLevel::Message);
//}
//
//void PixelsForGlory::Debug::Log(const char message) {
//    std::stringstream ss;
//    ss << message;
//    send_log(ss, LogLevel::Message);
//}
//
//void PixelsForGlory::Debug::Log(const float message) {
//    std::stringstream ss;
//    ss << message;
//    send_log(ss, LogLevel::Message);
//}
//
//void PixelsForGlory::Debug::Log(const double message) {
//    std::stringstream ss;
//    ss << message;
//    send_log(ss, LogLevel::Message);
//}
//
//void PixelsForGlory::Debug::Log(const bool message) {
//    std::stringstream ss;
//    if (message)
//        ss << "true";
//    else
//        ss << "false";
//
//    send_log(ss, LogLevel::Message);
//}
//
//// LogWarning
//void PixelsForGlory::Debug::LogWarning(const char* message) {
//    std::stringstream ss;
//    ss << message;
//    send_log(ss, LogLevel::Warning);
//}
//
//void PixelsForGlory::Debug::LogWarning(const std::string message) {
//    std::stringstream ss;
//    ss << message;
//    send_log(ss, LogLevel::Warning);
//}
//
//void PixelsForGlory::Debug::LogWarning(const int message) {
//    std::stringstream ss;
//    ss << message;
//    send_log(ss, LogLevel::Warning);
//}
//
//void PixelsForGlory::Debug::LogWarning(const char message) {
//    std::stringstream ss;
//    ss << message;
//    send_log(ss, LogLevel::Warning);
//}
//
//void PixelsForGlory::Debug::LogWarning(const float message) {
//    std::stringstream ss;
//    ss << message;
//    send_log(ss, LogLevel::Warning);
//}
//
//void PixelsForGlory::Debug::LogWarning(const double message) {
//    std::stringstream ss;
//    ss << message;
//    send_log(ss, LogLevel::Warning);
//}
//
//void PixelsForGlory::Debug::LogWarning(const bool message) {
//    std::stringstream ss;
//    if (message)
//        ss << "true";
//    else
//        ss << "false";
//
//    send_log(ss, LogLevel::Warning);
//}
//
//
//// LogError
//void PixelsForGlory::Debug::LogError(const char* message) {
//    std::stringstream ss;
//    ss << message;
//    send_log(ss, LogLevel::Error);
//}
//
//void PixelsForGlory::Debug::LogError(const std::string message) {
//    std::stringstream ss;
//    ss << message;
//    send_log(ss, LogLevel::Error);
//}
//
//void PixelsForGlory::Debug::LogError(const int message) {
//    std::stringstream ss;
//    ss << message;
//    send_log(ss, LogLevel::Error);
//}
//
//void PixelsForGlory::Debug::LogError(const char message) {
//    std::stringstream ss;
//    ss << message;
//    send_log(ss, LogLevel::Error);
//}
//
//void PixelsForGlory::Debug::LogError(const float message) {
//    std::stringstream ss;
//    ss << message;
//    send_log(ss, LogLevel::Error);
//}
//
//void PixelsForGlory::Debug::LogError(const double message) {
//    std::stringstream ss;
//    ss << message;
//    send_log(ss, LogLevel::Error);
//}
//
//void PixelsForGlory::Debug::LogError(const bool message) {
//    std::stringstream ss;
//    if (message)
//        ss << "true";
//    else
//        ss << "false";
//
//    send_log(ss, LogLevel::Error);
//}
//
//void PixelsForGlory::Debug::send_log(const std::stringstream& ss, const LogLevel& level) {
//    const std::string tmp = ss.str();
//    const char* tmsg = tmp.c_str();
//
//    if (callbackInstance != nullptr)
//    {
//        callbackInstance(tmsg, (int)level, (int)strlen(tmsg));
//    }
//    else
//    {
//        Message message;
//        message.message = tmsg;
//        message.level = level;
//        QueuedMessages.push_back(message);
//    }
//}
////-------------------------------------------------------------------
//
////Create a callback delegate
//void RegisterDebugCallback(FuncCallBack cb) {
//    callbackInstance = cb;
//
//    if (callbackInstance != nullptr)
//    {
//        for (auto const& msg : PixelsForGlory::Debug::QueuedMessages)
//        {
//            callbackInstance(msg.message.c_str(), (int)msg.level, (int)strlen(msg.message.c_str()));
//        }
//        PixelsForGlory::Debug::QueuedMessages.clear();
//    }
//}
//
//void ClearDebugCallback() {
//    callbackInstance = nullptr;
//}