/*
 * PluginManager.h
 *
 *  Created on: Aug 25, 2015
 *      Author: dlll
 */

#ifndef APP_HEADER_WORMPLUGIN_PLUGINMANAGER_H_
#define APP_HEADER_WORMPLUGIN_PLUGINMANAGER_H_

#include <list>

#include "../WormPlugin.h"

namespace App {
	namespace WormPlugin {
		class PluginManager {
			public:
				struct PluginInfo {
						void * libHandle;
						WormPluginInfo info;
				};

			private:
				std::list<PluginInfo> pluginList;
			public:
				PluginManager();
				virtual ~PluginManager();

				bool loadPluginDir(const char* dirPath, const char* fileWildcard);
				bool tryLoadPlugin(const char* filePath);
				bool unloadPlugin(PluginInfo plugin);
				bool unloadAllPlugin();

				int getPluginNum() {
					return pluginList.size();
				}

				std::list<PluginInfo> getPluginList() {
					return pluginList;
				}
		};
	}
} /* namespace App */

#endif /* APP_HEADER_WORMPLUGIN_PLUGINMANAGER_H_ */
