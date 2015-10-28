/*
 * WormToolImpl.cpp
 *
 *  Created on: Oct 14, 2015
 *      Author: Eclipse C++
 */

#include "../../Header/WormTool/WormToolImpl.h"

#include <iostream>
#include <utility>

#include "../../Header/WormTool/argsParse.h"

namespace App {
	namespace WormTool {
		WormToolImpl::~WormToolImpl() {

		}
		
		WormToolImpl::WormToolImpl() {
		}
		
		void WormToolImpl::initArgs(int argc, char** argv) {
			argsParse(argc, argv, [&](char* name, char* value) -> bool {
				argMap[name] = (value == nullptr ? "": value);
				return true;
			});
		}
		
		void WormToolImpl::initConfig(const char* dirPath, const char* fileWildcard) {
			config.addConfigDir(dirPath, fileWildcard);
		}

		const std::string WormToolImpl::getConfig(const std::string key, const std::string defaultVal) {
			/* hook from args */
			auto arg = argMap.find(key);
			if(arg != argMap.end()){
				std::cout << "   |- " << key << " -- " << arg->second << std::endl;
				return arg->second;
			}
			const std::string tmp = config.get(key, defaultVal);
			std::cout << "   |- " << key << " -> " << tmp << std::endl;
			return tmp;
		}

		void WormToolImpl::getArgs(std::function<bool(const std::string name, const std::string val)> callback) {
			for(auto item : argMap){
				if (!callback(item.first, item.second)){
					return;
				}
			}
		}
		
		const std::string App::WormTool::WormToolImpl::getArgs(const std::string key, const std::string defaultVal) {
			auto item = argMap.find(key);
			if (item != argMap.end()){
				return item->second;
			}
			return defaultVal;
		}

		bool WormToolImpl::setRunTimeConfig(const std::string key, const std::string value, bool cover) {
			if (argMap.find(key) == argMap.end()){
				argMap[key] = value;
				return true;
			}else{
				if (cover) {
					argMap[key] = value;
					return true;
				}
				return false;
			}
		}
	
	}
} /* namespace App */

