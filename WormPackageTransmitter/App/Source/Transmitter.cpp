/*
 * Transmitter.cpp
 *
 *  Created on: Sep 26, 2015
 *      Author: Eclipse C++
 */

#include "../Header/Transmitter.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <tr1/unordered_set>
#include <utility>

#include "../../Framework/Header/Debug/Debug.h"
#include "../../Framework/Header/Sock/BaseSocket.h"
#include "../../Framework/Header/Sock/ClientSocket.h"
#include "../../Framework/Header/Sock/EpollSocketEvent.h"
#include "../../Framework/Header/Sock/EpollSocketEventIndividual.h"
#include "../Header/ReceiverAPI.h"
#include "../Header/WormTool.h"

Transmitter::Transmitter() :
		epoll(64), autobuf(2048) {
}

Transmitter::~Transmitter() {

}

bool Transmitter::init(float frameVersion, WormBrage *brage) {
	if (FRAME_VERSION != frameVersion) return false;
	wHook = brage->hook;

	wHook->registerHook("Transmitter Resume Sock", REGISTER_EMPTY_CLEAN);
	wHook->registerHook("Transmitter Pause Sock", REGISTER_EMPTY_CLEAN);

	wHook->registerHook("Transmitter Receiving First Package", REGISTER_EMPTY_CLEAN);
	wHook->registerHook("Transmitter Package First Package", REGISTER_EMPTY_CLEAN);
	wHook->registerHook("Transmitter Receiving Data Package", REGISTER_EMPTY_CLEAN);
	wHook->registerHook("Transmitter Package Data Package", REGISTER_EMPTY_CLEAN);

	WormTool &tool = *brage->tool;
	runMode = (tool.getConfig("runAs", "client") == "client") ? CLIENT : SERVER;
	return true;
}

bool Transmitter::hook(WormBrage *brage) {
	wHook->hook("Receiver Connect Sock", REGISTER_HOOK(Transmitter::recConnectSock));
	wHook->hook("Receiver Receiving Package", REGISTER_HOOK(Transmitter::recReceivingPackage));
	wHook->hook("Receiver Disconnect Sock", REGISTER_HOOK(Transmitter::recDisconnectSock));

	wHook->hook("Transmitter Resume Sock", REGISTER_HOOK(Transmitter::needResumeSock));
	wHook->hook("Transmitter Pause Sock", REGISTER_HOOK(Transmitter::needPauseSock));
	return true;
}

void Transmitter::start() {
	if (state == Stop) {
		state = Start;
		std::thread(std::function<void(void)>(std::bind(&Transmitter::process, this))).detach();
	}
}

void Transmitter::stop() {
	if (state != Stop) {
		state = Stop;
	}
}

bool Transmitter::needPauseSock(int sockID, void* arg) {
	auto item = sockMap.find(sockID);
	if (item == sockMap.end()) return false;
	auto sock = item->second->individual->getSocket();
	return epoll.changeRemove(sock, Sock::EpollSocket::CanRead) & epoll.changeRemove(sock, Sock::EpollSocket::CanWrite);;
}

bool Transmitter::needResumeSock(int sockID, void* arg) {
	auto item = sockMap.find(sockID);
	if (item == sockMap.end()) return false;
	auto sock = item->second->individual->getSocket();
	return epoll.changeAdd(sock, Sock::EpollSocket::CanRead) & epoll.changeAdd(sock, Sock::EpollSocket::CanWrite);
}

bool Transmitter::recConnectSock(int sockID, void* arg) {
	switch (runMode) {
		case CLIENT: {
			receiverConnectSock *conInfo = (receiverConnectSock*) arg;
			return connectRemote(sockID, conInfo->ipAddr, conInfo->port, conInfo);
		}
		case SERVER:
			sockInfo *si = new sockInfo;
			si->flag = WAIT_FIRST_PACKAGE;
			sockMap[sockID] = si;
			break;
	}
	return true;
}

bool Transmitter::connectRemote(int sockID, in_addr ip, uint16_t port, receiverConnectSock *conInfo) {
	using SE = Sock::EpollSocket;
	Sock::ClientSocket cs;
	cs.create(AF_INET);
	cs.setNonBlocking(true);
	cs.connect(ip.s_addr, port);

	sockInfo *si;

	auto item = sockMap.find(sockID);
	if (item != sockMap.end()) {
		si = item->second;
	} else {
		si = new sockInfo;
		sockMap[sockID] = si;
	}

	Sock::EpollSocketEventIndividual *individual = Sock::EpollSocketEvent::createIndividual(cs);

	si->individual = individual;
	si->flag = CONNECT;
	si->recSockID = sockID;
	if (conInfo != nullptr) {
		si->connectIP = conInfo->serverAddr;
		si->connectPort = conInfo->serverPort;
	} else {
		si->connectIP = ip;
		si->connectPort = port;
	}

	individual->setFlag(sockID);
	individual->setPointer(si);

	epoll.add(cs, SE::Disconnect | SE::CanReadAndWrite, SE::LevelTriggered, individual);
	std::cout << sockID << " connect -> " << inet_ntoa(ip) << ":" << ntohs(port) << std::endl;
	return true;
}

bool Transmitter::recReceivingPackage(int sockID, void* arg) {
	receiverPackage *recvInfo = (receiverPackage *) arg;

	auto item = sockMap.find(sockID);
	if (item != sockMap.end()) {
		sockInfo &si = *item->second;
		if (si.flag == WAIT_FIRST_PACKAGE) {
			//发送 收到第一包 消息
			wHook->pushAll("Transmitter Receiving First Package", sockID, recvInfo, true);

			/*
			 *    第一包
			 * 	----------------------------------------------------
			 * 	| Random Length | Random Content | IP | PORT | CRC |
			 * 	----------------------------------------------------
			 * 	|       1       |      1 ~ 4     | 4  |   2  |  1  |
			 *    ----------------------------------------------------
			 */
			if (recvInfo->size < 8 || recvInfo->size > 12 || recvInfo->buf[0] > 4) {
				DEBUG("First Package Error : size " << recvInfo->size);
				closeQueue.push(sockID);
				return true;
			}

			if (getDataCRC(recvInfo->buf, recvInfo->size - 1) != recvInfo->buf[recvInfo->size - 1]) {
				DEBUG("First Package CRC Error");
				closeQueue.push(sockID);
				return true;
			}

			uint8_t *offset = &recvInfo->buf[recvInfo->buf[0] + 1];
			in_addr ip = *(in_addr*) offset;
			uint16_t port = *(uint16_t*) (offset + 4);

			connectRemote(sockID, ip, port, nullptr);

			DEBUG("firstPackage ok!");
			return true;
		}

		//发送收到数据包消息
		wHook->pushAll("Transmitter Receiving Data Package", sockID, recvInfo, true);

		if (si.canWrite) {
			auto sock = si.individual->getSocket();
			int sendSize = sock.send(recvInfo->buf, recvInfo->size);
			if (sendSize < 0) {
				DEBUG("sendSize < 0");
			} else if (sendSize < recvInfo->size) {
				si.lastSendBuf = autobuf;
				memcpy(si.lastSendBuf, recvInfo->buf + sendSize, recvInfo->size - sendSize);
				si.lastSendExcess = recvInfo->size - sendSize;

				si.canWrite = false;
				epoll.changeAdd(sock, Sock::EpollSocket::CanWrite);

				//暂停另一边的工作
				DEBUG("===============暂停 " << sockID << " " << si.lastSendExcess);

				wHook->pushAll("Receiver Pause Sock", sockID, nullptr, true);
			}
		} else {
			if (si.lastSendExcess != -1) {
				DEBUG("sock " << sockID << " have some data");
				return false;
			}
			si.lastSendBuf = autobuf;
			memcpy(si.lastSendBuf, recvInfo->buf, recvInfo->size);
			si.lastSendExcess = recvInfo->size;

			//暂停另一边的工作
			DEBUG("暂停 " << sockID);
			wHook->pushAll("Receiver Pause Sock", sockID, nullptr, true);
		}
	} else {
		DEBUG("sock not find");
	}
	return true;
}

bool Transmitter::recDisconnectSock(int sockID, void* arg) {
	std::cout << sockID << " disconnect" << std::endl;
	auto item = sockMap.find(sockID);
	if (item == sockMap.end()) return false;
	closeQueue.push(sockID);
	return true;
}

void Transmitter::process() {
	using SE = Sock::EpollSocket;

	while (state != Stop) {
		if (!closeQueue.empty()) {
			int size = closeQueue.size();
			for (int i = 0; i < size; ++i) {
				int sockID = closeQueue.front();
				auto item = sockMap.find(sockID);
				if (item != sockMap.end()) {
					wHook->pushAll("Receiver Close Sock", sockID, nullptr, true);
					if (item->second->flag != WAIT_FIRST_PACKAGE) item->second->individual->getSocket().close();
					delete item->second;
					sockMap.erase(item);
				}
				closeQueue.pop();
			}
		}

		auto &events = epoll.wait(10);
		for (int i = 0; i < events.size(); i++) {
			auto &event = *events[i];
			auto sock = event.getSocket();
			sockInfo &si = *(sockInfo *) event.getPointer();

			if (si.flag == CONNECT) {
				if (event.haveEvents(SE::Disconnect)) {
					closeQueue.push(si.recSockID);
					DEBUG("Can not connect to remote sock");
					continue;
				} else {
					si.flag = ESTABLISHED;

					if (runMode == CLIENT) {
						receiverPackage *rpack = new receiverPackage(si.recSockID, autobuf);

						char randomSize = rand() % 4 + 1;

						rpack->size = randomSize + 8;
						rpack->buf[0] = randomSize;

						for (int i = 1; i <= randomSize; ++i)
							rpack->buf[i] = rand() % 255;

						memcpy(rpack->buf + randomSize + 1, &si.connectIP, 4);
						memcpy(rpack->buf + randomSize + 5, &si.connectPort, 2);

						rpack->buf[rpack->size - 1] = getDataCRC(rpack->buf, randomSize + 7);
						DEBUG(si.recSockID << " Send First Package");

						//发送打包第一包消息
						wHook->pushAll("Transmitter Package First Package", si.recSockID, rpack, true);

						si.lastSendExcess = rpack->size;
						si.lastSendBuf = rpack->buf;

						delete rpack;
					}

					DEBUG(si.recSockID << " ESTABLISHED");
				}
			}

			if (event.haveEvents(SE::Disconnect)) {
				receiverPackage *recvInfo = new receiverPackage(0, autobuf);

				recvInfo->size = sock.recv(recvInfo->buf, 1500);
				if (recvInfo->size > 0) {
					wHook->pushAll("Transmitter Package Data Package", si.recSockID, recvInfo, true);
					wHook->pushAll("Receiver Sending Package", si.recSockID, recvInfo, true);
				} else {
					delete recvInfo;
				}

				closeQueue.push(si.recSockID);
				DEBUG("remote sock Disconnect");
			}

			if (event.haveEvents(SE::CanRead)) {
				receiverPackage *recvInfo = new receiverPackage(0, autobuf);

				recvInfo->size = sock.recv(recvInfo->buf, 1500);
				if (recvInfo->size > 0) {
					wHook->pushAll("Transmitter Package Data Package", si.recSockID, recvInfo, true);
					wHook->pushAll("Receiver Sending Package", si.recSockID, recvInfo, true);
				} else {
					delete recvInfo;
				}
			} else if (event.haveEvents(SE::CanWrite)) {
				if (si.lastSendExcess >= 0) {
					//有一些没发送完的
					int sendSize = sock.send(si.lastSendBuf, si.lastSendExcess);
					DEBUG("发送上次暂停的内容 "<< sendSize);
					if (sendSize < 0) {
						DEBUG("sendSize < 0");
					} else if (sendSize < si.lastSendExcess) {
						DEBUG("还没发送完毕，剩余 "<< (si.lastSendExcess - sendSize));
						si.lastSendBuf.setReservedAreaSize(si.lastSendBuf.getReservedAreaSize() + sendSize);
						si.lastSendExcess -= sendSize;
					} else {
						DEBUG("发送完毕");
						si.lastSendExcess = -1;
						si.lastSendBuf.retrocede();
						si.canWrite = true;

						DEBUG("恢复 " << si.recSockID);
						wHook->pushAll("Receiver Resume Sock", si.recSockID, nullptr, true);
						epoll.changeRemove(sock, SE::CanWrite);
					}
				} else {
					si.canWrite = true;
					epoll.changeRemove(sock, SE::CanWrite);
					DEBUG("恢复 " << si.recSockID);
					wHook->pushAll("Receiver Resume Sock", si.recSockID, nullptr, true);
				}
				DEBUG("CanWrite");
			}
		}
	}

	for (auto item : sockMap) {
		if (item.second->flag != WAIT_FIRST_PACKAGE) item.second->individual->getSocket().close();
		delete item.second;
	}

	std::cout << "Transmitter are Stopped." << std::endl;
}
