#ifdef WIN32
	#include <windows/libloaderapi.h>
#else
	#include <unistd.h>
	#include <limits>
#endif

#include "include/Util.hpp"

#ifndef PATH_MAX
	#define PATH_MAX 4096
#endif

///
/// Individual Functions
///

std::string Util::getWorkingDir() {
	char buff[PATH_MAX];

	#ifdef WIN32
		DWORD len = GetModuleFileName(NULL, buff, PATH_MAX);
	#else
		ssize_t len = readlink("/proc/self/exe", buff, PATH_MAX-1);
	#endif

	if(len > 0 && len <= PATH_MAX){
		buff[len] = '\0';
		std::string workingDir = std::string(buff);

		#ifdef WIN32
			size_t index = workingDir.find_last_of('\\');
		#else
			size_t index = workingDir.find_last_of('/');
		#endif
			return workingDir.substr(0, index+1);
	}

	return "";
}


///
/// CMDParser
///

std::string_view Util::CMDParser::getOption(const std::string_view& option) {
	for(auto it = tokens.begin(), end = tokens.end(); it != end; it++) {
		if(*it == option){
			if(it + 1 != end){
				return *(it + 1);
			}
		}
	}

	return "";
}

std::string_view Util::CMDParser::getOption(const std::pair<std::string_view, std::string_view>& options) {
	std::string_view longOpt = getOption(options.first);
	if(longOpt.compare("") == 0){
		return getOption(options.second);
	}

	return longOpt;
}

bool Util::CMDParser::hasOption(const std::string_view& option) {
	for(auto it = tokens.begin(), end = tokens.end(); it != end; it++) {
		if(*it == option){
			return true;
		}
	}

	return false;
}
