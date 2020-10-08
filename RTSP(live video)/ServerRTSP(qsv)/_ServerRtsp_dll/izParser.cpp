#include "izParser.h"
#include <regex>
#include <iostream>
#include <string>
#include <sstream>
#include <windows.h>

IZParser::IZParser(void)
{
}

IZParser::~IZParser(void)
{
}

std::map<std::string, std::string> IZParser::Parse(const std::string& query)
{
#pragma warning (push, 0)
	std::map<std::string, std::string> data;
	std::regex pattern("([\\w+%]+)=([^&]*)");
	auto words_begin = std::sregex_iterator(query.begin(), query.end(), pattern);
	auto words_end = std::sregex_iterator();

	for (std::sregex_iterator i = words_begin; i != words_end; i++)
	{
		std::string key = (*i)[1].str();
		std::string value = (*i)[2].str();
		data[key] = value;
	}
#pragma warning (pop)
	return data;
}