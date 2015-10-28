/*
 * WormPlugin.h
 *
 *  Created on: Aug 24, 2015
 *      Author: dlll
 */

#ifndef WORMSOCK_WORMPLUGIN_H_
#define WORMSOCK_WORMPLUGIN_H_

#include <functional>
#include <string>
#include "WormTool.h"


#define FRAME_VERSION 0.01f
#define WormPluginClass

typedef bool (HOOK_FUNC)(int flag, void* arg);
typedef bool (CLEAN_FUNC)(int flag, void* arg, bool ret);

extern "C" {

	class WormHook {
		public:
			virtual ~WormHook() = default;
			virtual bool registerHook(const std::string hookName, std::function<CLEAN_FUNC> cleanFunc) = 0;
			virtual bool hook(const std::string hookName, std::function<HOOK_FUNC> hookFunc) = 0;
			virtual bool haveHook(const std::string hookName) = 0;
			virtual bool pushAll(const std::string hookName, int flag, void* arg, bool asyn = false) = 0;
			virtual bool pushOne(const std::string hookName, int flag, void* arg, bool asyn = false) = 0;
	};

	struct WormBrage{
			WormHook *hook;
			WormTool *tool;
	};

	class WormPluginInstance {
		protected:
			enum PluginState {
				Stop = 0, Start, Busy
			};

			PluginState state { Stop };
		public:
			virtual ~WormPluginInstance() = default;
			virtual bool init(float frameVersion, WormBrage *brage) = 0;
			virtual bool hook(WormBrage *brage) = 0;

			virtual void start() {
				if (state == Stop) state = Start;
			}

			virtual void stop() {
				if (state != Stop) state = Stop;
			}
	};

	struct WormPluginInfo {
			const char * pluginName;
			const char * pluginAuthor;
			float pluginVersion;
			WormPluginInstance * instance;
	};
}

#define REGISTER_PLUGIN(name, author, version, object) \
		extern "C" {WormPluginInfo getPluginInfo(){return WormPluginInfo {name,author,version,new object};}}

#define REGISTER_HOOK(func) \
		std::function<HOOK_FUNC>(std::bind(&func, this, std::placeholders::_1, std::placeholders::_2))

#define REGISTER_CLEAN_HOOK(func) \
		std::function<CLEAN_FUNC>(std::bind(&func, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))

#define REGISTER_EMPTY_CLEAN nullptr
#endif /* WORMSOCK_WORMPLUGIN_H_ */
