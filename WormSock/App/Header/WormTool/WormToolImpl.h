/*
 * WormToolImpl.h
 *
 *  Created on: Oct 14, 2015
 *      Author: Eclipse C++
 */

#ifndef APP_HEADER_WORMTOOL_WORMTOOLIMPL_H_
#define APP_HEADER_WORMTOOL_WORMTOOLIMPL_H_

#include <functional>
#include <string>
#include <tr1/unordered_map>

#include "../WormTool.h"
#include "WormConfig.h"

namespace App {
	namespace WormTool {
		class WormToolImpl: public ::WormTool {
			private:
				std::tr1::unordered_map<std::string, std::string> argMap;
				App::WormTool::WormConfig config;

			public:
				WormToolImpl();
				virtual ~WormToolImpl();

				void initArgs(int argc, char **argv);
				void initConfig(const char* dirPath, const char* fileWildcard);

				virtual const std::string getConfig(const std::string key, const std::string defaultVal);
				virtual void getArgs(std::function<bool(const std::string name, const std::string val)> callback);
				virtual const std::string getArgs(const std::string key, const std::string defaultVal);

				bool setRunTimeConfig(const std::string key, const std::string value, bool cover = false);
		};
	}
} /* namespace App */

#endif /* APP_HEADER_WORMTOOL_WORMTOOLIMPL_H_ */
