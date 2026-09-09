#include "../inc/Utils.hpp"

std::string	toUpper(const std::string& str)
{
    std::string	result(str);

    for (size_t i = 0; i < result.size(); ++i)
		result[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[i])));
	return result;
}

std::string	trim(const std::string& str)
{
    size_t	first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
		return "";
	size_t	last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

std::vector<std::string>	splitParams(const std::string& str)
{
	std::vector<std::string>	params;
	size_t	i = 0;

	while (i < str.size())
	{
		while (i < str.size() && str[i] == ' ')
			++i;
		if (i >= str.size())
			break;
		if (str[i] == ':')
		{
			params.push_back(str.substr(i + 1));
			break;
		}
		size_t	end = str.find(' ', i);
		if (end == std::string::npos)
		{
			params.push_back(str.substr(i));
			break;
		}
		params.push_back(str.substr(i, end - i));
		i = end + 1;
	}
	return params;
}
