/*
 * server.cpp
 *
 *  Created on: Aug 24, 2015
 *      Author: dlll
 */

#include <chrono>
#include <csignal>
#include <iostream>
#include <list>
#include <thread>

#include "App/Header/WormPlugin/PluginManager.h"
#include "App/Header/WormPlugin/WormHookImpl.h"
#include "App/Header/WormPlugin.h"
#include "App/Header/WormTool/WormToolImpl.h"

#ifdef _RELEASE
#	define PLUGIN_PATH "/mnt/WormSock/Plugin"
#	define CONFIG_PATH "/mnt/WormSock/Config"
#else
#	ifdef _X86_RELEASE
#		define PLUGIN_PATH "/home/lutty/workspace/Test/WormSocks/ReleasePlugin"
#		define CONFIG_PATH "/home/lutty/workspace/Test/WormSocks/Config"
#	else
#		define PLUGIN_PATH "/home/lutty/workspace/Test/WormSocks/DebugPlugin"
#		define CONFIG_PATH "/home/lutty/workspace/Test/WormSocks/Config"
#	endif
#endif

bool run = true;

void testSigInt(int cc) {
	std::cout << "\n\n#### STOP ####\n\n";
	run = false;
}

void initWormTool(int argc, char **argv, App::WormTool::WormToolImpl &wormTool) {
	wormTool.initConfig(CONFIG_PATH, "*.conf");
	wormTool.initArgs(argc, argv);

	if (wormTool.getArgs("runAs", "client") == "server") {
		wormTool.setRunTimeConfig("runAs", "server");
		std::cout << "run as: server" << std::endl;
	} else {
		wormTool.setRunTimeConfig("runAs", "client");
		std::cout << "run as: client" << std::endl;
	}

	wormTool.getConfig("main.version", "0.1");
}

void pluginLoad(App::WormPlugin::PluginManager &pm, WormBrage &brage) {
	std::list<App::WormPlugin::PluginManager::PluginInfo> unloadList;

	std::cout << "try load module dir: " << PLUGIN_PATH << std::endl;
	pm.loadPluginDir(PLUGIN_PATH, "*.so");
	for (auto item : pm.getPluginList()) {
		std::cout << " ─ \033[1;34mtry load module\033[0m : \033[1;36m" << item.info.pluginName << ", version=" << item.info.pluginVersion << "\033[0m" << std::endl;
		if (!item.info.instance->init(FRAME_VERSION, &brage)) {
			std::cerr << "   └─\033[1;31m init error \033[0m" << std::endl;
			unloadList.push_back(item);
		}
	}
	for (auto item : unloadList) {
		pm.unloadPlugin(item);
	}
}

void pluginHook(App::WormPlugin::PluginManager &pm, WormBrage &brage) {
	std::list<App::WormPlugin::PluginManager::PluginInfo> unloadList;

	for (auto item : pm.getPluginList()) {
		if (!item.info.instance->hook(&brage)) {
			std::cerr << "   └─\033[1;31m hook error \033[0m" << std::endl;
			unloadList.push_back(item);
		}
	}
	for (auto item : unloadList) {
		pm.unloadPlugin(item);
	}
}

void pluginStart(App::WormPlugin::PluginManager &pm) {
	for (auto item : pm.getPluginList()) {
		item.info.instance->start();
	}

	std::cerr << "all module are running..." << std::endl;
}

int main(int argc, char **argv) {
	App::WormPlugin::PluginManager pm;
	App::WormPlugin::WormHookImpl wormHook;
	App::WormTool::WormToolImpl wormTool;

	WormBrage brage { &wormHook, &wormTool };

	initWormTool(argc, argv, wormTool);

	pluginLoad(pm, brage);
	pluginHook(pm, brage);
	pluginStart(pm);

	signal(SIGINT, testSigInt);

	/*  ++ Main Loop ++ */
	while (run) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	/*  -- Main Loop -- */

	wormHook.stop();
	for (auto item : pm.getPluginList()) {
		std::cerr << "Module : " << item.info.pluginName << " stopping..." << std::endl;
		item.info.instance->stop();
	}

	std::this_thread::sleep_for(std::chrono::seconds(1));
	return 0;
}
