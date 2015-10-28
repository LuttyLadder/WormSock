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
#else
#	define PLUGIN_PATH "/home/dlll/workspace/Test/WormSocks/DebugPlugin"
#endif

bool run = true;

void testSigInt(int cc) {
	std::cout << "\n\n#### STOP ####\n\n";
	run = false;
}

void initWormTool(int argc, char **argv, App::WormTool::WormToolImpl &wormTool) {
	wormTool.initConfig("/mnt/WormSock/Config", "*.conf");
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
		std::cout << " |- try load module : " << item.info.pluginName << ", version=" << item.info.pluginVersion << std::endl;
		if (!item.info.instance->init(FRAME_VERSION, &brage)) {
			std::cerr << "Module : " << item.info.pluginName << " init error" << std::endl;
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
			std::cerr << "Module : " << item.info.pluginName << " hook error" << std::endl;
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
