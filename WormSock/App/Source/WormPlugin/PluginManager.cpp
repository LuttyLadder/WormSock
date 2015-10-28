/*
 * PluginManager.cpp
 *
 *  Created on: Aug 25, 2015
 *      Author: dlll
 */

#include "../../Header/WormPlugin/PluginManager.h"

#include <dirent.h>
#include <dlfcn.h>
#include <iostream>
#include <string>

#include "../../Header/WormTool/Tool.h"

namespace App {
	namespace WormPlugin {
		PluginManager::PluginManager() {

		}

		PluginManager::~PluginManager() {
			unloadAllPlugin();
		}
		
		bool PluginManager::loadPluginDir(const char* dirPath, const char* fileWildcard) {
			std::string dPath = std::string(dirPath) + "/", filePath;
			App::WormTool::traverseFolder(dirPath, [&](struct dirent& ent) -> bool {
				switch(ent.d_type){
					case DT_LNK:
						//no break
					case DT_REG:
						if (App::WormTool::match(fileWildcard, ent.d_name)){
							filePath = dPath + std::string(ent.d_name);
							tryLoadPlugin(filePath.c_str());
						}
						break;
					default:
						break;
				}
				return true;
			});
			return getPluginNum() > 0;
		}

		bool PluginManager::tryLoadPlugin(const char* filePath) {
			PluginInfo pi;
			pi.libHandle = dlopen(filePath, RTLD_LAZY);
			if (!pi.libHandle) {

				std::cout << "can't load library '" << filePath << "': " << dlerror() << std::endl;
				return false;
			}

			WormPluginInfo (*getPluginInfo)(void);
			getPluginInfo = (WormPluginInfo (*)(void)) dlsym(pi.libHandle, "getPluginInfo");
			if (!getPluginInfo){
				std::cout << "can't get library info '" << filePath << "': "<< dlerror() << std::endl;
				dlclose(pi.libHandle);
				return false;
			}

			pi.info = (*getPluginInfo)();

			pluginList.push_back(pi);
			return true;
		}

		bool PluginManager::unloadPlugin(PluginInfo plugin) {
			for(auto pi = pluginList.begin(); pi != pluginList.end(); ++pi){
				if (pi->libHandle == plugin.libHandle){
					pluginList.erase(pi);
					return true;
				}
			}
			return false;
		}

		bool PluginManager::unloadAllPlugin() {
			for(auto pi: pluginList){
				delete pi.info.instance;
				dlclose(pi.libHandle);
			}
			pluginList.clear();
			return true;
		}
	}
} /* namespace App */

