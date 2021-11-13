#ifndef _CELL_CONFIG_HPP_
#define	_CELL_CONFIG_HPP_
/*
	专门用于读取配置数据
	目前我们的配置参数主要来源于main函数的args传入
*/
#include"Cell.hpp"
#include"CELLLog.hpp"
#include<string>
#include<map>

class CELLConfig
{
private:
	CELLConfig()
	{

	}
public:
	~CELLConfig()
	{

	}

	static CELLConfig& Instance()
	{
		static CELLConfig obj;
		return obj;
	}

	void Init(int argc, char* args[])
	{
		_exePath = args[0];
		for (int n = 1; n < argc; n++)
		{
			madeCmd(args[n]);
		}
	}

	void madeCmd(char* cmd)
	{
		//cmd: strIP=127.0.0.1
		char* val = strchr(cmd, '=');
		//若查找成功，返回从 '='开始的字符串
		if (val)
		{
			//将等号换为字符串结束符'\0'
			//cmd:strIP\0127.0.0.1
			*val = '\0';
			//val:127.0.0.1
			//cmd:strIP
			val++;

			_kv[cmd] = val;
			CELLLog_Debug("madeCmd k<%s> v<%s>", cmd, val);
		}else{
			_kv[cmd] = "";
			CELLLog_Debug("madeCmd k<%s> ", cmd);
		}
	}

	const char* getStr(const char* argName, const char* def)
	{
		auto iter = _kv.find(argName);
		if (iter == _kv.end())
		{
			CELLLog_Error("CELLConfig::getStr not found <%s>", argName);
		}
		else {
			def = iter->second.c_str();
		}

		CELLLog_Info("CELLConfig::getStr %s=%s", argName, def);
		return def;
	}

private:
	//保存当前可执行程序的路径
	std::string _exePath;
	//配置传入的key-value型数据
	std::map<std::string, std::string> _kv;
};




#endif // !_CELL_CONFIG_HPP_
