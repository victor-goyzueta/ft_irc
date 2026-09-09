#ifndef UTILS_HPP
# define UTILS_HPP

# include <string>
# include <vector>
# include <cctype>

std::string					toUpper(const std::string& str);
std::string					trim(const std::string& str);
std::vector<std::string>	splitParams(const std::string& str);

#endif
