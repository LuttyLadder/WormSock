/*
 * WormTool.h
 *
 *  Created on: Oct 14, 2015
 *      Author: Eclipse C++
 */

#ifndef APP_HEADER_WORMTOOL_H_
#define APP_HEADER_WORMTOOL_H_

#include <functional>
#include <string>

extern "C" {
	class WormTool {
		public:
			virtual ~WormTool() = default;
			virtual const std::string getConfig(const std::string key, const std::string defaultVal) = 0;
			virtual void getArgs(std::function<bool(std::string name, std::string val)> callback) = 0;
			virtual const std::string getArgs(const std::string key, const std::string defaultVal) = 0;
	};
}

#endif /* APP_HEADER_WORMTOOL_H_ */
