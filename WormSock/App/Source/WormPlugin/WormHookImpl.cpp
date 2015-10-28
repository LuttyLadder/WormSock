/*
 * WormHookImpl.cpp
 *
 *  Created on: Aug 25, 2015
 *      Author: dlll
 */

#include "../../Header/WormPlugin/WormHookImpl.h"

#include <chrono>
#include <cstdlib>
#include <ctime>

namespace App {
	namespace WormPlugin {
		WormHookImpl::WormHookImpl() {
			std::thread(std::function<void()>(std::bind(&WormHookImpl::process, this))).detach();
		}

		WormHookImpl::~WormHookImpl() {

		}

		bool WormHookImpl::registerHook(const std::string hookName, std::function<CLEAN_FUNC> cleanFunc) {
			auto hookInfo = hookMap.find(hookName);
			if (hookInfo == hookMap.end()) {
				hookMap[hookName].cleanFunc = cleanFunc;
				return true;
			}
			return false;
		}

		bool WormHookImpl::hook(const std::string hookName, std::function<HOOK_FUNC> hookFunc) {
			auto info = hookMap.find(hookName);
			if (info != hookMap.end()) {
				info->second.list.push_back(hookInfo(0, hookFunc));
				return true;
			}
			return false;
		}

		bool WormHookImpl::haveHook(const std::string hookName) {
			auto info = hookMap.find(hookName);
			return info != hookMap.end();
		}

		bool WormHookImpl::push(bool pushOne, const std::string &hookName, int flag, void* arg, bool asyn) {
			if (needStop) return false;
			bool ret { false };
			if (asyn) {
				auto info = hookMap.find(hookName);
				if (info != hookMap.end()) {
					pushMsg msg { pushOne, info, flag, arg };
					if (taskSnyc.mutexID != std::this_thread::get_id()) {
						std::unique_lock<std::mutex> lock(taskSnyc.taskMutex);
						taskSnyc.mutexID = std::this_thread::get_id();
						ret = asynPush(msg);
						lock.unlock();
					} else {
						ret = asynPush(msg);
					}
				}
				return ret;
			} else {
				if (msgQueue.size() > MAX_HOOK_QUEUE) return false;
				auto info = hookMap.find(hookName);
				if (info != hookMap.end()) {
					msgQueue.push(pushMsg { pushOne, info, flag, arg });
					if (taskSnyc.taskEmpty) {
						if (taskSnyc.mutexID != std::this_thread::get_id()) {
							std::unique_lock<std::mutex> lock(taskSnyc.taskMutex);
							taskSnyc.mutexID = std::this_thread::get_id();
							taskSnyc.reportNotEmpty.notify_all();
							lock.unlock();
						} else {
							taskSnyc.reportNotEmpty.notify_all();
						}
					}
					return true;
				}
				return false;
			}
		}

		bool WormHookImpl::pushAll(const std::string hookName, int flag, void* arg, bool asyn) {
			return push(false, hookName, flag, arg, asyn);
		}

		bool WormHookImpl::pushOne(const std::string hookName, int flag, void* arg, bool asyn) {
			return push(true, hookName, flag, arg, asyn);
		}

		bool WormHookImpl::asynPush(pushMsg& item) {
			bool ret { false };
			int hookListSize = 0, randPos = 0;
			auto &list = item.hookInfo->second.list;
			hookListSize = list.size();
			if (item.pushOne) {
				//只发送给监听者中的一个
				randPos = rand() % hookListSize;
				if (randPos < hookListSize / 2) {
					//离前端比较近
					auto info = list.begin();
					for (int i = 0; i < randPos; ++i)
						++info;
					ret = info->hookFunc(item.flag, item.arg);
				} else {
					//离后端比较近
					randPos = hookListSize - randPos;
					auto info = list.end();
					for (int i = hookListSize; i > randPos; --i)
						--info;
					ret = info->hookFunc(item.flag, item.arg);
				}
			} else {
				//发送给所有监听者
				for (auto info : list) {
					if (info.hookFunc(item.flag, item.arg)) {
						ret = true;
						break;
					}
				}
			}
			if (item.hookInfo->second.cleanFunc != nullptr) item.hookInfo->second.cleanFunc(item.flag, item.arg, ret);
			return ret;
		}

		void WormHookImpl::process() {
			int queueSize = 0;
			srand(time(NULL));
			std::unique_lock<std::mutex> lock(taskSnyc.taskMutex, std::defer_lock);
			while (!needStop) {
				lock.lock();
				taskSnyc.mutexID = std::this_thread::get_id();
				if (msgQueue.size() <= 0) {
					taskSnyc.taskEmpty = true;
					while (msgQueue.size() <= 0 && !needStop) {
						//等待报告队列非空.
						taskSnyc.reportNotEmpty.wait(lock);
					}
					taskSnyc.taskEmpty = false;
				} else {
					taskSnyc.reportNotEmpty.wait_for(lock, std::chrono::microseconds(50));
				}
				lock.unlock();
				if (needStop) break;

				queueSize = msgQueue.size();
				for (int i = 0; i < queueSize; ++i) {
					auto item = msgQueue.front();
					asynPush(item);
					msgQueue.pop();
				}
			}
		}

		void WormHookImpl::stop() {
			std::unique_lock<std::mutex> lock(taskSnyc.taskMutex);
			needStop = true;
			taskSnyc.reportNotEmpty.notify_all();
			lock.unlock();
		}
	}
} /* namespace App */
