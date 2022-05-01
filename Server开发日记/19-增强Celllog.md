# 增强Celllog

1. 完善输出信息种类 Info、 Error、 Warring、Debug 
2. 完善日志名：在日志文件名中添加时间日期

```c++
#ifndef _CELL_LOG_HPP_
#define	_CELL_LOG_HPP_

#include"Cell.hpp"
#include"CELLTask.hpp"
#include<ctime>

class CELLLog
{
    //仅在Debug模式下才输出调试信息
#ifdef _DEBUG
	#ifndef CELLLog_Debug
		#define CELLLog_Debug(...) CELLLog::Debug(__VA_ARGS__)
	#endif
	#else
	#ifndef CELLLog_Debug
		#define CELLLog_Debug(...)
	#endif
#endif // _DEBUG

#define CELLLog_Info(...) CELLLog::Info(__VA_ARGS__)
#define CELLLog_Warring(...) CELLLog::Warring(__VA_ARGS__)
#define CELLLog_Error(...) CELLLog::Error(__VA_ARGS__)

private:
	CELLLog()
	{
		_taskServer.Start();
	}
	~CELLLog() 
	{
		_taskServer.Close();
		if (_logFile)
		{
			Info("CELLLog fclose (_logFile)\n");
			fclose(_logFile);
			_logFile = nullptr;
		}
	}
public:
	static CELLLog& Instance()
	{
		static CELLLog slog;
		return slog;
	}

	void setLogPath(const char* logName, const char* mode)
	{
		if (_logFile)
		{
			Info("CELLLog::setLogPath _logFile != nullptr\n");
			fclose(_logFile);
			_logFile = nullptr;
		}
		static char logPath[256] = {};
		auto t = system_clock::now();
		auto tNow = system_clock::to_time_t(t);
		std::tm* now = std::localtime(&tNow);
		sprintf(logPath, "%s[%d-%d-%d_%d_%d_%d].txt", logName, now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
		_logFile = fopen(logPath, mode);
		if (_logFile)
		{
			Info("CELLLog::setLogPath success,<%s,%s>\n", logPath, mode);
		}
		else {
			Info("CELLLog::setLogPath failed,<%s,%s>\n", logPath, mode);
		}
	}

	static void Error(const char* pStr)
	{
		Error("%s", pStr);
	}

	template<typename ...Args>
	static void Error(const char* pformat, Args ... args)
	{
		Echo("Error", pformat, args...);
	}

	static void Warring(const char* pStr)
	{
		Warring("%s", pStr);
	}

	template<typename ...Args>
	static void Warring(const char* pformat, Args ... args)
	{
		Echo("Warring", pformat, args...);
	}

	static void Debug(const char* pStr)
	{
		Debug("%s", pStr);
	}

	template<typename ...Args>
	static void Debug(const char* pformat, Args ... args)
	{
		Echo("Debug", pformat, args...);
	}


	static void Info(const char* pStr)
	{
		Info("%s", pStr);
	}

	template<typename ...Args>
	static void Info(const char* pformat, Args ... args)
	{
		Echo("Info", pformat, args...);
	}

	template<typename ...Args>
	static void Echo(const char* type, const char* pformat, Args ... args)
	{
		CELLLog* pLog = &Instance();
		pLog->_taskServer.addTask([=]() {
			if (pLog->_logFile)
			{
				auto t = system_clock::now();
				auto tNow = system_clock::to_time_t(t);
				//fprintf(pLog->_logFile, "%s", ctime(&tNow));
				std::tm* now = std::gmtime(&tNow);
				fprintf(pLog->_logFile, "%s", type);
				fprintf(pLog->_logFile, "[%d-%d-%d %d:%d:%d]", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
				fprintf(pLog->_logFile, pformat, args...);
				fflush(pLog->_logFile);
			}
			printf(pformat, args...);
			});
	}

private:
	FILE* _logFile = nullptr;
	CELLTaskServer _taskServer;
};





#endif // !_CELL_LOG_HPP_

```

