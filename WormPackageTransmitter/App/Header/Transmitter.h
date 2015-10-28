/*
 * Transmitter.h
 *
 *  Created on: Sep 26, 2015
 *      Author: Eclipse C++
 */

#ifndef APP_HEADER_TRANSMITTER_H_
#define APP_HEADER_TRANSMITTER_H_

#include <netinet/in.h>
#include <cstdint>
#include <queue>
#include <tr1/unordered_map>

#include "../../Framework/Header/Sock/EpollSocket.h"
#include "../../Framework/Header/Util/AutoBuffer.h"
#include "WormPlugin.h"

struct receiverConnectSock;

struct in_addr;

class Transmitter: public WormPluginInstance {
	private:
		enum runMode {
			SERVER, CLIENT
		};

		enum sockFlag {
			CONNECT, ESTABLISHED, WAIT_FIRST_PACKAGE
		};

		struct sockInfo {
				sockFlag flag { CONNECT };
				int recSockID { 0 };
				Sock::EpollSocketEventIndividual *individual { nullptr };
				bool canWrite { false };

				in_addr connectIP;
				uint16_t connectPort;

				Util::AutoBuffer::Buffer lastSendBuf;
				int lastSendExcess { -1 };
		};
	private:
		WormHook* wHook { nullptr };

		Sock::EpollSocket epoll;
		std::tr1::unordered_map<int, sockInfo*> sockMap;

		std::queue<int> closeQueue;

		Util::AutoBuffer autobuf;

		runMode runMode {CLIENT};

	private:
		uint8_t getDataCRC(uint8_t *data, int length){
			uint8_t crc = data[0];
			for(int i = 1; i < length; i++){
				crc ^= data[i];
			}
			return crc;
		}

	public:
		Transmitter();
		virtual ~Transmitter();

		virtual bool init(float frameVersion, WormBrage *brage);
		virtual bool hook(WormBrage *brage);
		virtual void start();
		virtual void stop();

		bool needPauseSock(int sockID, void* arg);
		bool needResumeSock(int sockID, void* arg);

		bool recConnectSock(int sockID, void* arg);
		bool recReceivingPackage(int sockID, void* arg);
		bool recDisconnectSock(int sockID, void* arg);

	private:
		bool connectRemote(int sockID, in_addr ip, uint16_t port, receiverConnectSock *conInfo);
		void process();
};

REGISTER_PLUGIN("Transmitter", "Lutty", 0.01f, Transmitter);

#endif /* APP_HEADER_TRANSMITTER_H_ */
