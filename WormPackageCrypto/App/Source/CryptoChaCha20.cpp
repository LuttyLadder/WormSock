/*
 * CryptoChaCha20.cpp
 *
 *  Created on: Oct 23, 2015
 *      Author: Eclipse C++
 */

#include "../Header/CryptoChaCha20.h"

#include <openssl/ossl_typ.h>
#include <sys/time.h>
#include <cstring>
#include <functional>
#include <string>
#include <tr1/unordered_set>

#include "../../Framework/Header/Debug/Debug.h"
#include "../../Framework/Header/Crypto/Hex.h"
#include "../../Framework/Header/Util/AutoBuffer.h"
#include "../Header/ReceiverAPI.h"
#include "../Header/WormTool.h"

CryptoChaCha20::CryptoChaCha20() {

}

CryptoChaCha20::~CryptoChaCha20() {
	delete info;
}

bool CryptoChaCha20::init(float frameVersion, WormBrage* brage) {
	if (FRAME_VERSION != frameVersion) return false;
	wHook = brage->hook;

	WormTool &tool = *brage->tool;

	return initSetting(tool);
}

bool CryptoChaCha20::hook(WormBrage* brage) {
	wHook->hook("Transmitter Receiving First Package", REGISTER_HOOK(CryptoChaCha20::traRecvFirstPackage));
	wHook->hook("Transmitter Receiving Data Package", REGISTER_HOOK(CryptoChaCha20::traRecvDataPackage));
	wHook->hook("Transmitter Package First Package", REGISTER_HOOK(CryptoChaCha20::traPackFirstPackage));
	wHook->hook("Transmitter Package Data Package", REGISTER_HOOK(CryptoChaCha20::traPackDataPackage));

	wHook->hook("Receiver Close Sock", REGISTER_HOOK(CryptoChaCha20::sockClose));

	return true;
}

bool CryptoChaCha20::initSetting(WormTool& tool) {
	runMode = (tool.getConfig("runAs", "client") == "client") ? CLIENT : SERVER;

	std::string ID, DATA_KEY;
	ID = tool.getConfig("crypto.Client.ID", "");
	DATA_KEY = tool.getConfig("crypto.Client.DATA_KEY", "");
	if (ID == "" || DATA_KEY == "") return false;

	if (runMode == CLIENT) {
		std::string PUBLIC_KEY_X, PUBLIC_KEY_Y;
		PUBLIC_KEY_X = tool.getConfig("crypto.ChaCha20.PUBLIC_KEY.X", "");
		PUBLIC_KEY_Y = tool.getConfig("crypto.ChaCha20.PUBLIC_KEY.Y", "");
		if (PUBLIC_KEY_X == "" || PUBLIC_KEY_Y == "") return false;

		info = new CryptoInfo(PUBLIC_KEY_X, PUBLIC_KEY_Y);
		if(!EC_KEY_check_key(info->ECC_KEY)) return false;
	} else {
		std::string PRIVATE_KEY;
		PRIVATE_KEY = tool.getConfig("crypto.ChaCha20.PRIVATE_KEY", "");
		if (PRIVATE_KEY == "") return false;

		info = new CryptoInfo(PRIVATE_KEY);
	}

	if (Crypto::Hex::toBin(ID, info->ID) != 8) return false;
	if (Crypto::Hex::toBin(DATA_KEY, info->DATA_KEY) != 32) return false;

	//重置随机数种子
	timeval time;
	gettimeofday(&time, NULL);
	RAND_seed(&time, sizeof(time));

	return true;
}

bool CryptoChaCha20::traRecvDataPackage(int sockID, void* arg) {
	//服务器接受客户端消息或者客户端接受客户消息
	encryptor* enc = getEncryptor(sockID);
	if (enc == nullptr) return false; //已被释放
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
	if (enc == nullptr) return false; //已被释放
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

int CryptoChaCha20::encode_ECC(const EC_GROUP* group, EC_POINT* C1, EC_POINT* C2, uint8_t* to, BN_CTX* ctx) {
	BIGNUM *x = BN_new();
	BIGNUM *y = BN_new();
	int length = sizeof(int), tmp;

	for (int i = 0; i < 2; i++) {
		EC_POINT *point = (i == 0 ? C1 : C2);
		EC_POINT_get_affine_coordinates_GFp(group, point, x, y, ctx);
		for (int j = 0; j < 2; j++) {
			BIGNUM *num = (j == 0 ? x : y);
			tmp = BN_bn2bin(num, to + length + 1);
			if (tmp > 255) return -1;
			to[length] = tmp;
			length += tmp + 1;
		}
	}

	BN_free(x);
	BN_free(y);
	memcpy(to, &length, sizeof(int));
	return length;
}

bool CryptoChaCha20::decode_ECC(const EC_GROUP* group, EC_POINT* C1, EC_POINT* C2, uint8_t* from, BN_CTX* ctx) {
	BIGNUM *x = BN_new();
	BIGNUM *y = BN_new();
	int length = sizeof(int), max;

	memcpy(&max, from, sizeof(int));

	if (max > 140) {
		return false;
	}

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			BIGNUM *num = (j == 0 ? x : y);
			BN_bin2bn(from + length + 1, from[length], num);
			length += from[length] + 1;
		}
		EC_POINT *point = (i == 0 ? C1 : C2);
		EC_POINT_set_affine_coordinates_GFp(group, point, x, y, ctx);
	}

	BN_free(x);
	BN_free(y);
	return true;
}

bool CryptoChaCha20::traRecvFirstPackage(int sockID, void* arg) {
	receiverPackage *pack = (receiverPackage *) arg;

	EC_POINT *C1 = EC_POINT_new(info->ECC_GROUP), *C2 = EC_POINT_new(info->ECC_GROUP);
	if (!decode_ECC(info->ECC_GROUP, C1, C2, pack->buf, info->ECC_CTX)) {
		return false;
	}

	//计算解密后的点 M = C1 - C2 * private(b)
	EC_POINT *tmp = EC_POINT_new(info->ECC_GROUP), *DM = EC_POINT_new(info->ECC_GROUP);
	EC_POINT_mul(info->ECC_GROUP, tmp, NULL, C2, EC_KEY_get0_private_key(info->ECC_KEY), info->ECC_CTX);
	EC_POINT_invert(info->ECC_GROUP, tmp, info->ECC_CTX);
	EC_POINT_add(info->ECC_GROUP, DM, C1, tmp, info->ECC_CTX);

	EC_POINT_free(tmp);
	EC_POINT_free(C1);
	EC_POINT_free(C2);


	//解码 DM
	int length;
	uint8_t buf[255];
	BIGNUM *x = BN_new();
	BIGNUM *y = BN_new();

	EC_POINT_get_affine_coordinates_GFp(info->ECC_GROUP, DM, x, y, info->ECC_CTX);

	length = BN_bn2bin(x, buf);
	if (length != 16) return false;
	//初始化 encryptor
	getEncryptor(sockID, buf + 8);

	pack->size = BN_bn2bin(y, pack->buf);

	EC_POINT_free(DM);
	BN_free(x);
	BN_free(y);
	return false;
}

bool CryptoChaCha20::traPackFirstPackage(int sockID, void* arg) {
	receiverPackage *pack = (receiverPackage *) arg;
	info->GenerationRandom();

	/*
	 *    信息包
	 * 	-----------------------
	 * 	|  User ID  |  Nonce  |
	 * 	-----------------------
	 * 	|     8     |    8    |
	 *    -----------------------
	 */
	uint8_t tmp[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
	RAND_bytes(tmp + 8, 8);
	memcpy(tmp, info->ID, 8);

	//将明文编码到M上
	EC_POINT *M = EC_POINT_new(info->ECC_GROUP);
	BIGNUM *Mx = BN_new(), *My = BN_new();
	BN_bin2bn(tmp, 16, Mx);
	BN_bin2bn(pack->buf, pack->size, My);
	if (!EC_POINT_set_affine_coordinates_GFp(info->ECC_GROUP, M, Mx, My, info->ECC_CTX)) /* ERR()*/;

	//计算加密后的点 C1=M+rK；C2=rG
	EC_POINT *C1 = EC_POINT_new(info->ECC_GROUP), *C2 = EC_POINT_new(info->ECC_GROUP);
	//C1 = r * public_key(b)
	EC_POINT_mul(info->ECC_GROUP, C1, NULL, EC_KEY_get0_public_key(info->ECC_KEY), info->RANDOM, info->ECC_CTX);
	//C1 = C1 + M
	EC_POINT_add(info->ECC_GROUP, C1, M, C1, info->ECC_CTX);
	//C2 = r * generator
	EC_POINT_mul(info->ECC_GROUP, C2, info->RANDOM, NULL, NULL, info->ECC_CTX);

	pack->size = encode_ECC(info->ECC_GROUP, C1, C2, pack->buf, info->ECC_CTX);

	//初始化 encryptor
	getEncryptor(sockID, tmp + 8);

	EC_POINT_free(M);
	BN_free(Mx);
	BN_free(My);
	EC_POINT_free(C1);
	EC_POINT_free(C2);

	return false;
}

bool CryptoChaCha20::sockClose(int sockID, void* arg) {
	auto item = encMap.find(sockID);
	if (item != encMap.end()) {
		delete item->second;
	}
	return false;
}
