/*
 * Receiver.h
 *
 *  Created on: Aug 31, 2015
 *      Author: dlll
 */

#ifndef RECEIVER_H_
#define RECEIVER_H_

#include <netinet/in.h>
#include <cstdint>
#include <queue>
#include <tr1/unordered_map>

#include "../../Framework/Header/Sock/EpollSocket.h"
#include "../../Framework/Header/Sock/EpollSocketEventIndividual.h"
#include "../../Framework/Header/Sock/ServerSocket.h"
#include "../../Framework/Header/Util/AutoBuffer.h"
#include "WormPlugin.h"

#define HOOK_ADDRESS

class Receiver: public WormPluginInstance {
	private:
		enum runMode {
			SERVER, CLIENT
		};

		struct receiverConfig {
				union {
						struct {
								int listenPort;

								// server
								in_addr serverIP;
								uint16_t serverPort;
#							ifdef HOOK_ADDRESS
								// hook
								in_addr hookIP;
								uint16_t hookPort;
#							endif
						} client;

						struct {
								int listenPort;
						} server;
				};

		} config;
	private:
		WormHook* wHook { nullptr };
		std::tr1::unordered_map<int, Sock::EpollSocketEventIndividual *> sockMap;

		int sockID { 0 };
		std::queue<int> closeQueue;

		Sock::ServerSocket ss;

		Sock::EpollSocket epoll;
		Util::AutoBuffer autobuf;

		runMode runMode {CLIENT};

	public:
		Receiver();
		virtual ~Receiver() = default;

		virtual bool init(float frameVersion, WormBrage *brage);
		virtual bool hook(WormBrage *brage);
		virtual void start();
		virtual void stop();

		bool needPauseSock(int sockID, void* arg);
		bool needResumeSock(int sockID, void* arg);
		bool needCloseSock(int sockID, void* arg);

		bool sendPackage(int sockID, void* arg);

	private:
		void initSetting(WormTool &tool);
		void initHook(WormHook &hook);

		void process();
		bool cleanSock(int sockID);
};

REGISTER_PLUGIN("Receiver", "Lutty", 0.02f, Receiver);

#endif /* RECEIVER_H_ */
