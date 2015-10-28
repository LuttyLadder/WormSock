/*
 * WormHookImpl.h
 *
 *  Created on: Aug 25, 2015
 *      Author: dlll
 */

#ifndef APP_HEADER_WORMHOOK_WORMHOOKIMPL_H_
#define APP_HEADER_WORMHOOK_WORMHOOKIMPL_H_

#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <tr1/unordered_map>
#include <tr1/unordered_set>

#include "../WormPlugin.h"

#define MAX_HOOK_QUEUE		64

namespace App {
	namespace WormPlugin {

		class WormHookImpl: public WormHook {
			private:

				struct hookInfo {
						int priority { 0 };
						std::function<HOOK_FUNC> hookFunc { nullptr };

						hookInfo(int priority, std::function<HOOK_FUNC> hookFunc) :
								priority(priority), hookFunc(hookFunc) {
						}
				};

				struct hookList {
						std::list<hookInfo> list;
						std::function<CLEAN_FUNC> cleanFunc;
				};

				typedef std::tr1::unordered_map<std::string, hookList> hookListMap;

				struct pushMsg {
						bool pushOne;
						hookListMap::iterator hookInfo;
						int flag;
						void* arg;
				};

				struct taskSnyc {
						std::mutex taskMutex;
						std::condition_variable reportNotEmpty;
						std::thread::id mutexID;
						bool taskEmpty { false };
				} taskSnyc;

			private:
				hookListMap hookMap;
				std::queue<pushMsg> msgQueue;
				bool needStop { false };

			public:
				WormHookImpl();
				virtual ~WormHookImpl();

				virtual bool registerHook(const std::string, std::function<CLEAN_FUNC> cleanFunc);
				virtual bool hook(const std::string, std::function<HOOK_FUNC> hookFunc);
				virtual bool haveHook(const std::string);
				virtual bool pushAll(const std::string, int flag, void* arg, bool asyn = false);
				virtual bool pushOne(const std::string, int flag, void* arg, bool asyn = false);

				void stop();

			private:
				void process();
				bool push(bool pushOne, const std::string &hookName, int flag, void* arg, bool asyn);
				bool asynPush(pushMsg &msg);
		};
	}
} /* namespace App */

#endif /* APP_HEADER_WORMHOOK_WORMHOOKIMPL_H_ */
