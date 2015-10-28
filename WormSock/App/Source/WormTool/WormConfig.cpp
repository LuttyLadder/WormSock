/*
 * WormConfig.cpp
 *
 *  Created on: Oct 14, 2015
 *      Author: Eclipse C++
 */

#include "../../Header/WormTool/WormConfig.h"

#include <dirent.h>
#include <stddef.h>
#include <fstream>
#include <iostream>
#include <tr1/unordered_set>

#include "../../Header/WormTool/Tool.h"

namespace App {
	namespace WormTool {
		WormConfig::WormConfig() {
			// TODO Auto-generated constructor stub

		}
		
		WormConfig::~WormConfig() {
			// TODO Auto-generated destructor stub
		}

		bool WormConfig::addConfigFile(const std::string configPath, const std::string prefix) {
			using namespace std;
			if (!fileExists(configPath.c_str())) {
				return false;
			}
			fstream cfgFile;
			cfgFile.open(configPath, ios_base::in);
			if (!cfgFile.is_open()) {
				return false;
			}
			char tmp[1024];
			string line, key, value;
			size_t pos;
			while (!cfgFile.eof()) {
				cfgFile.getline(tmp, 1024);
				line = tmp;
				trim(line);
				if (line[0] != '#') {
					pos = line.find('=');
					if (pos == string::npos) continue;
					key = prefix + "." + line.substr(0, pos);
					value = line.substr(pos + 1);
					trim(key);
					trim(value);
					if (value[0] == '"' && value[value.size() - 1] == '"') value = value.substr(1, value.size() - 2);
					cfg[key] = value;
				}
			};
			cfgFile.close();
			return true;
		}

		bool WormConfig::addConfigDir(const char* dirPath, const char* fileWildcard) {
			std::string dPath = std::string(dirPath) + "/", filePath, fileName;
			App::WormTool::traverseFolder(dirPath, [&](struct dirent& ent) -> bool {
				switch(ent.d_type) {
					case DT_LNK:
					//no break
					case DT_REG:
					if (App::WormTool::match(fileWildcard, ent.d_name)) {
						fileName = ent.d_name;
						fileName = fileName.substr(0, fileName.find_last_of("."));
						filePath = dPath + std::string(ent.d_name);
						addConfigFile(filePath, fileName);
					}
					break;
					default:
					break;
				}
				return true;
			});
			return true;
		}

		const std::string WormConfig::get(const std::string key, const std::string defVal) {
			auto item = cfg.find(key);
			if (item == cfg.end()) return defVal;
			return item->second;
		}
	}
} /* namespace App */
