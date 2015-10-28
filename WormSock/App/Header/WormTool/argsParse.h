/*
 * argsParse.h
 *
 *  Created on: Oct 14, 2015
 *      Author: Eclipse C++
 */

#ifndef APP_HEADER_WORMTOOL_ARGSPARSE_H_
#define APP_HEADER_WORMTOOL_ARGSPARSE_H_

#include <cstring>
#include <functional>


namespace App{
	namespace WormTool{
		bool argsParse(int argc, char **argv, std::function<bool(char* name, char* value)> callback) {
			char *name {nullptr}, *value {nullptr};
			for (int i = 1; i < argc; ++i) {
				if (argv[i][0] == '-' && argv[i][1] != '-') {
					name = argv[i] + 1;
					value = nullptr;
					if (argv[i + 1][0] != '-') {
						value = argv[i + 1];
						i++;
					}
				} else if (argv[i][0] == '-' && argv[i][1] == '-') {
					char *pos = strstr(argv[i], "=");
					if (pos == nullptr) continue;
					name = argv[i] + 2;
					*pos = '\0';
					value = pos + 1;
				} else{
					name = argv[i];
					value = nullptr;
				}
				if (!callback(name, value)){
					return true;
				}
			}
			return true;
		}
	}
}


#endif /* APP_HEADER_WORMTOOL_ARGSPARSE_H_ */
