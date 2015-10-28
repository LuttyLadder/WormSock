/*
 * CryptoChaCha20.cpp
 *
 *  Created on: Oct 23, 2015
 *      Author: Eclipse C++
 */

#include "../Header/CryptoChaCha20.h"

#include <functional>
#include <string>

#include "../Header/ReceiverAPI.h"
#include "../Header/WormTool.h"

CryptoChaCha20::CryptoChaCha20() {
	
}

CryptoChaCha20::~CryptoChaCha20() {
	
}

bool CryptoChaCha20::init(float frameVersion, WormBrage* brage) {
	if (FRAME_VERSION != frameVersion) return false;
	wHook = brage->hook;

	WormTool &tool = *brage->tool;
	runMode = (tool.getConfig("runAs", "client") == "client") ? CLIENT : SERVER;

	return true;
}

bool CryptoChaCha20::hook(WormBrage* brage) {
	wHook->hook("Transmitter Receiving First Package", REGISTER_HOOK(CryptoChaCha20::traRecvFirstPackage));
	wHook->hook("Transmitter Receiving Data Package", REGISTER_HOOK(CryptoChaCha20::traRecvDataPackage));
	wHook->hook("Transmitter Package First Package", REGISTER_HOOK(CryptoChaCha20::traPackFirstPackage));
	wHook->hook("Transmitter Package Data Package", REGISTER_HOOK(CryptoChaCha20::traPackDataPackage));

	wHook->hook("Receiver Close Sock", REGISTER_HOOK(CryptoChaCha20::sockClose));

	return true;
}

bool CryptoChaCha20::traRecvDataPackage(int sockID, void* arg) {
	//服务器接受客户端消息或者客户端接受客户消息
	encryptor* enc = getEncryptor(sockID);
	receiverPackage *recvInfo = (receiverPackage *) arg;

	switch (runMode) {
		case SERVER: {
			enc->uploadEncryptor.crypt(recvInfo->buf, recvInfo->size);
			break;
		}
		case CLIENT: {
			enc->uploadEncryptor.crypt(recvInfo->buf, recvInfo->size);
			break;
		}
	}
	return false;
}

bool CryptoChaCha20::traPackDataPackage(int sockID, void* arg) {
	//客户端发送消息给服务器或者服务器发送消息给服务
	encryptor* enc = getEncryptor(sockID);
	receiverPackage *recvInfo = (receiverPackage *) arg;

	switch (runMode) {
		case SERVER: {
			enc->downloadEncryptor.crypt(recvInfo->buf, recvInfo->size);
			break;
		}
		case CLIENT: {
			enc->downloadEncryptor.crypt(recvInfo->buf, recvInfo->size);
			break;
		}
	}
	return false;
}

bool CryptoChaCha20::traRecvFirstPackage(int sockID, void* arg) {
	return false;
}

bool CryptoChaCha20::traPackFirstPackage(int sockID, void* arg) {
	return false;
}

bool CryptoChaCha20::sockClose(int sockID, void* arg) {
	auto item = encMap.find(sockID);
	if (item != encMap.end()) {
		delete item->second;
	}
	return false;
}
