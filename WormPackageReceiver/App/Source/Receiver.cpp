/*
 * Receiver.cpp
 *
 *  Created on: Aug 31, 2015
 *      Author: dlll
 */

#include "../Header/Receiver.h"

#include <arpa/inet.h>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <tr1/unordered_set>
#include <utility>

#include <netinet/in.h>
#include <linux/netfilter_ipv4.h>
#include <unistd.h>
#include <cstdint>
#include <iostream>
#include <string>

#include "../../Framework/Header/Crypto/Dec.h"
#include "../../Framework/Header/Debug/Debug.h"
#include "../../Framework/Header/Sock/BaseSocket.h"
#include "../../Framework/Header/Sock/EpollSocketEvent.h"
#include "../Header/ReceiverAPI.h"
#include "../Header/WormTool.h"

Receiver::Receiver() :
		epoll(64), autobuf(2048) {
}

bool Receiver::init(float frameVersion, WormBrage* brage) {
	if (FRAME_VERSION != frameVersion) return false;
	/* 初始化 Hook */
	wHook = brage->hook;
	initHook(*wHook);

	/* 读取设置 */
	WormTool &tool = *brage->tool;
	initSetting(tool);

	return true;
}

void Receiver::initHook(WormHook &hook) {
	/* 由我发送的消息 */
	hook.registerHook("Receiver Connect Sock", [](int flag, void* arg, bool ret) -> bool {
		delete (receiverSockIndividualData *) arg;
		return true;
	});
	hook.registerHook("Receiver Receiving Package", [](int flag, void* arg, bool ret) -> bool {
		delete (receiverPackage *) arg;
		return true;
	});
	hook.registerHook("Receiver Disconnect Sock", REGISTER_EMPTY_CLEAN);

	/* 由其他人发送的消息 */
	hook.registerHook("Receiver Sending Package", [](int flag, void* arg, bool ret) -> bool {
		delete (receiverPackage *) arg;
		return true;
	});
	hook.registerHook("Receiver Pause Sock", REGISTER_EMPTY_CLEAN);
	hook.registerHook("Receiver Resume Sock", REGISTER_EMPTY_CLEAN);
	hook.registerHook("Receiver Close Sock", REGISTER_EMPTY_CLEAN);
}

void Receiver::initSetting(WormTool &tool) {
	runMode = (tool.getConfig("runAs", "client") == "client") ? CLIENT : SERVER;

	if (runMode == CLIENT) {
#		ifdef HOOK_ADDRESS
		/* hook info */
		inet_aton(tool.getConfig("receiver.hook.ip", "127.0.0.1").c_str(), &config.client.hookIP);
		config.client.hookPort = htons(Crypto::Dec::format(tool.getConfig("receiver.hook.port", "80")));
#		endif

		/* listen port */
		config.client.listenPort = Crypto::Dec::format(tool.getConfig("receiver.listenPort", "8979"));

		/* server info */
		inet_aton(tool.getConfig("receiver.server.ip", "127.0.0.1").c_str(), &config.client.serverIP);
		config.client.serverPort = htons(Crypto::Dec::format(tool.getConfig("receiver.server.port", "8910")));
	} else {
		config.server.listenPort = Crypto::Dec::format(tool.getConfig("receiver.listenPort", "8979"));
	}
}

bool Receiver::hook(WormBrage* brage) {
	wHook->hook("Receiver Pause Sock", REGISTER_HOOK(Receiver::needPauseSock));
	wHook->hook("Receiver Resume Sock", REGISTER_HOOK(Receiver::needResumeSock));
	wHook->hook("Receiver Close Sock", REGISTER_HOOK(Receiver::needCloseSock));
	wHook->hook("Receiver Sending Package", REGISTER_HOOK(Receiver::sendPackage));
	return true;
}

void Receiver::start() {
	if (state == Stop) {
		state = Start;
		std::thread(std::function<void(void)>(std::bind(&Receiver::process, this))).detach();
	}
}

void Receiver::stop() {
	if (state != Stop) {
		state = Stop;
	}
}

bool Receiver::needPauseSock(int sockID, void* arg) {
	auto item = sockMap.find(sockID);
	if (item == sockMap.end()) return false;
	auto sock = item->second->getSocket();

	return epoll.changeRemove(sock, Sock::EpollSocket::CanRead) & epoll.changeRemove(sock, Sock::EpollSocket::CanWrite);;
}

bool Receiver::needResumeSock(int sockID, void* arg) {
	auto item = sockMap.find(sockID);
	if (item == sockMap.end()) return false;
	auto sock = item->second->getSocket();
	return epoll.changeAdd(sock, Sock::EpollSocket::CanRead) & epoll.changeAdd(sock, Sock::EpollSocket::CanWrite);
}

bool Receiver::needCloseSock(int sockID, void* arg) {
	auto item = sockMap.find(sockID);
	if (item == sockMap.end()) return false;
	closeQueue.push(sockID);
	DEBUG("close it --");
	return true;
}

bool Receiver::sendPackage(int sockID, void* arg) {
	receiverPackage *recvInfo = (receiverPackage *) arg;
	auto item = sockMap.find(sockID);

	if (item != sockMap.end()) {
		Sock::EpollSocketEventIndividual * individual = item->second;
		receiverSockIndividualData &data = *(receiverSockIndividualData *) individual->getPointer();
		if (data.canWrite) {
			auto sock = individual->getSocket();
			int sendSize = sock.send(recvInfo->buf, recvInfo->size);
			if (sendSize < 0) {
				perror("sendSize < 0");
			} else if (sendSize < recvInfo->size) {
				if (data.lastWrite == nullptr) {
					data.lastWrite = new receiverWritePackage;
				} else {
					DEBUG("sock " << sockID << " have some data");
					return false;
				}
				data.lastWrite->buf = autobuf;
				memcpy(data.lastWrite->buf, recvInfo->buf, recvInfo->size);
				data.lastWrite->sentSize = sendSize;
				data.lastWrite->size = recvInfo->size;

				data.canWrite = false;
				epoll.changeAdd(sock, Sock::EpollSocket::CanWrite);

				//暂停另一边的工作
				DEBUG("=====================================暂停 " << sockID);
				wHook->pushAll("Transmitter Pause Sock", sockID, nullptr, true);
			}
		} else {
			if (data.lastWrite == nullptr) {
				data.lastWrite = new receiverWritePackage;
			} else {
				DEBUG("sock " << sockID << " have some data");
				return false;
			}
			data.lastWrite->buf = autobuf.getBuffer();
			memcpy(data.lastWrite->buf, recvInfo->buf, recvInfo->size);
			data.lastWrite->sentSize = 0;
			data.lastWrite->size = recvInfo->size;

			//暂停另一边的工作
			DEBUG("暂停 " << sockID);
			wHook->pushAll("Transmitter Pause Sock", sockID, nullptr, true);
		}
	}
	//DEBUG_HEX(recvInfo->buf, recvInfo->size, "receiver");
	return true;
}

bool Receiver::cleanSock(int sockID) {
	auto item = sockMap.find(sockID);
	if (item != sockMap.end()) {
		//find
		receiverSockIndividualData *data = (receiverSockIndividualData *) item->second->getPointer();
		delete data->lastWrite;
		delete data;
		item->second->getSocket().close();
		sockMap.erase(item);
		return true;
	}
	return false;
}

void Receiver::process() {
	using SE = Sock::EpollSocket;

	Sock::BaseSocket newSock;

	if (!ss.createAndBind(runMode == CLIENT ? config.client.listenPort : config.server.listenPort)) {
		std::cerr << "can't bind port " << config.client.listenPort << std::endl;
		return;
	}
	ss.listen();
	epoll.add(ss, Sock::EpollSocket::CanReadAndWrite, Sock::EpollSocket::EdgeTriggered);
	while (state != Stop) {
		if (!closeQueue.empty()) {
			int size = closeQueue.size();
			for (int i = 0; i < size; ++i) {
				cleanSock(closeQueue.front());
				closeQueue.pop();
			}
		}

		auto &events = epoll.wait(10);
		for (int i = 0; i < events.size(); i++) {
			auto &event = *events[i];
			auto sock = event.getSocket();
			int flag = event.getFlag();
			if (event == ss) {
				while (ss.accept(newSock)) {
					std::cout << "find a new socks = " << sockID << std::endl;
					auto inv = Sock::EpollSocketEvent::createIndividual(newSock);
					receiverConnectSock *data = nullptr;

					if (runMode == CLIENT) {
						data = new receiverConnectSock;
						data->ipAddr = config.client.serverIP;
						data->port = config.client.serverPort;
#						ifdef HOOK_ADDRESS
						data->serverAddr = config.client.hookIP;
						data->serverPort = config.client.hookPort;
#						else
						struct sockaddr_in destaddr;
						socklen_t socklen = sizeof(sockaddr_in);
						if (newSock.getSockOpt(SOL_IP, SO_ORIGINAL_DST, &destaddr, &socklen)) {
							data->serverAddr = destaddr.sin_addr;
							data->serverPort = destaddr.sin_port;
							DEBUG("ORG IP=" << inet_ntoa(destaddr.sin_addr) << ", PORT=" << ntohs(destaddr.sin_port));
						} else {
							std::cout << "can not get original address!!" << std::endl;
							perror("SO_ORIGINAL_DST ");
							newSock.close();
							continue;
						}
#						endif
					}

					if (wHook->pushAll("Receiver Connect Sock", sockID, data, true)) {
						inv->setFlag(sockID);
						inv->setPointer(new receiverSockIndividualData);
						sockMap[sockID] = inv;
						if (runMode == SERVER)
							epoll.add(newSock, SE::CanReadAndWrite | SE::Disconnect, SE::LevelTriggered, inv);
						else
							epoll.add(newSock, SE::Disconnect, SE::LevelTriggered, inv);
						sockID++;
						DEBUG("add it");
					} else {
						newSock.close();
						std::cout << "close it" << std::endl;
					}
				}
			} else if (event.haveEvents(SE::Disconnect)) {
				receiverPackage *recvInfo = new receiverPackage(0, autobuf);
				recvInfo->size = sock.recv(recvInfo->buf, 1500);
				if (recvInfo->size > 0) {
					wHook->pushAll("Receiver Receiving Package", flag, recvInfo, true);
				} else {
					delete recvInfo;
				}

				wHook->pushAll("Receiver Disconnect Sock", flag, &events, true);
				closeQueue.push(flag);
				// add to closeMap
			} else if (event.haveEvents(SE::CanRead)) {
				receiverPackage *recvInfo = new receiverPackage(0, autobuf);
				recvInfo->size = sock.recv(recvInfo->buf, 1500);
				wHook->pushAll("Receiver Receiving Package", flag, recvInfo, true);
			} else if (event.haveEvents(SE::CanWrite)) {
				receiverSockIndividualData &data = *(receiverSockIndividualData *) event.getPointer();
				DEBUG("Can Write " << flag);
				if (!data.canWrite) {
					if (data.lastWrite == nullptr) {
						data.canWrite = true;
						epoll.changeRemove(sock, SE::CanWrite);
						DEBUG("set Write " << flag);
					} else {
						receiverWritePackage &wp = *data.lastWrite;
						wp.sentSize += sock.send(wp.buf + wp.sentSize, wp.size - wp.sentSize);
						if (wp.sentSize >= wp.size - wp.sentSize) {
							delete data.lastWrite;
							data.lastWrite = nullptr;
							data.canWrite = true;
							epoll.changeRemove(sock, SE::CanWrite);

							//恢复工作
							DEBUG("恢复 " << flag);
							wHook->pushAll("Transmitter Resume Sock", flag, nullptr, true);
						}
					}
				} else {
					epoll.changeRemove(sock, SE::CanWrite);
				}
			}
		}
	}

	for (auto item : sockMap) {
		receiverSockIndividualData *data = (receiverSockIndividualData *) item.second->getPointer();
		delete data->lastWrite;
		delete data;
		item.second->getSocket().close();
	}

	ss.close();

	std::cout << "Receive are Stopped." << std::endl;
}
