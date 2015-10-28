/*
 * Tool.h
 *
 *  Created on: Aug 25, 2015
 *      Author: dlll
 */

#ifndef APP_HEADER_WORMPLUGIN_TOOL_H_
#define APP_HEADER_WORMPLUGIN_TOOL_H_

#include <dirent.h>
#include <stddef.h>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iterator>
#include <string>

namespace App {
	namespace WormTool {
		bool match(std::string::const_iterator patb, std::string::const_iterator pate, const std::string& input);

		bool match(const std::string& pattern, const std::string& input);

		int traverseFolder(const char *path, std::function<bool(struct dirent& ent)> func);
	}
} /* namespace App */

#endif /* APP_HEADER_WORMPLUGIN_TOOL_H_ */
