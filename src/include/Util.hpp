#pragma once

#include <vector>
#include <string>
#include <utility>

namespace Util {
	class CMDParser {
		public:
			CMDParser(const int& argc, char** argv) : tokens(argv, argv + argc) {}
			/// @brief Gets the value passed after the given option
			std::string_view getOption(const std::string_view& option);
			/// @brief Gets the value passed after either given option
			/// @note The first member of the pair takes precedent
			std::string_view getOption(const std::pair<std::string_view, std::string_view>& options);
			/// @brief returns if the given flag is set or option present
			bool hasOption(const std::string_view& option);
		private:
			const std::vector<std::string_view> tokens;
	};

	/// @brief Gets the full path to the executable
	std::string getWorkingDir();
};
