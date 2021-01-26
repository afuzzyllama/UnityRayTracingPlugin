#pragma once
//
//// https://stackoverflow.com/questions/43732825/use-debug-log-from-c
//
#include <stdio.h>
#include <string>
//#include <sstream>
//#include <vector>
//
//#include "../Unity/IUnityInterface.h"
//
#define PFG_EDITORLOG(msg) fprintf(stdout, ("[Raytracing Plugin] " + std::string(msg) + "\n").c_str());
#define PFG_EDITORLOGERROR(msg) fprintf(stderr, ("[Raytracing Plugin] " + std::string(msg) + "\n").c_str());
//
//extern "C"
//{
//    //Create a callback delegate
//    typedef void(*FuncCallBack)(const char* message, int level, int size);
//    static FuncCallBack callbackInstance = nullptr;
//    void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API RegisterDebugCallback(FuncCallBack cb);
//    void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API ClearDebugCallback();
//}
//
//namespace PixelsForGlory
//{
//    class Debug
//    {
//
//    public:
//
//        enum class LogLevel {
//            Message,
//            Warning,
//            Error
//        };
//
//        struct Message {
//            std::string message;
//            LogLevel level;
//        };
//
//        static std::vector<Message> QueuedMessages;
//
//        static void Log(const char* message);
//        static void Log(const std::string message);
//        static void Log(const int message);
//        static void Log(const char message);
//        static void Log(const float message);
//        static void Log(const double message);
//        static void Log(const bool message);
//
//        static void LogWarning(const char* message);
//        static void LogWarning(const std::string message);
//        static void LogWarning(const int message);
//        static void LogWarning(const char message);
//        static void LogWarning(const float message);
//        static void LogWarning(const double message);
//        static void LogWarning(const bool message);
//
//        static void LogError(const char* message);
//        static void LogError(const std::string message);
//        static void LogError(const int message);
//        static void LogError(const char message);
//        static void LogError(const float message);
//        static void LogError(const double message);
//        static void LogError(const bool message);
//
//    private:
//        static void send_log(const std::stringstream& ss, const LogLevel& level);
//    };
//
//
//}
