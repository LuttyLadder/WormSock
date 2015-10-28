/*
 * WormConfig.h
 *
 *  Created on: Oct 14, 2015
 *      Author: Eclipse C++
 */

#ifndef APP_HEADER_WORMTOOL_WORMCONFIG_H_
#define APP_HEADER_WORMTOOL_WORMCONFIG_H_

#include <unistd.h>
#include <string>
#include <tr1/unordered_map>

namespace App {
	namespace WormTool {
		class WormConfig {
			private:
				std::tr1::unordered_map<std::string, std::string> cfg;

				static bool fileExists(const char *pathname) {
					return access(pathname, F_OK) == 0;
				}

				static void trim(std::string &s) {
					static const char whitespace[] = " \n\t\v\r\f";
					s.erase(0, s.find_first_not_of(whitespace));
					s.erase(s.find_last_not_of(whitespace) + 1u);
				}

			public:
				WormConfig();
				virtual ~WormConfig();

				bool addConfigDir(const char* dirPath, const char* fileWildcard);
				bool addConfigFile(const std::string configPath, const std::string prefix);

				const std::string get(const std::string key, const std::string defVal);
		};
	}
} /* namespace App */

#endif /* APP_HEADER_WORMTOOL_WORMCONFIG_H_ */
