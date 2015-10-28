/*
 * CryptoChaCha20.h
 *
 *  Created on: Oct 23, 2015
 *      Author: Eclipse C++
 */

#ifndef HEADER_CRYPTOCHACHA20_H_
#define HEADER_CRYPTOCHACHA20_H_

#include <tr1/unordered_map>

#include "chacha20.hpp"
#include "WormPlugin.h"

class CryptoChaCha20: public WormPluginInstance {
	private:
		enum runMode {
			SERVER, CLIENT
		};

		struct encryptor {
				Chacha20 uploadEncryptor;
				Chacha20 downloadEncryptor;

				encryptor(const uint8_t key[32], const uint8_t nonce[8]) :
						uploadEncryptor(key, nonce), downloadEncryptor(key, nonce) {
				}
		};

	private:
		WormHook* wHook { nullptr };
		runMode runMode { CLIENT };

		std::tr1::unordered_map<int, encryptor*> encMap;

	public:
		CryptoChaCha20();
		virtual ~CryptoChaCha20();

		virtual bool init(float frameVersion, WormBrage *brage);
		virtual bool hook(WormBrage *brage);

		bool traRecvDataPackage(int sockID, void* arg);
		bool traRecvFirstPackage(int sockID, void* arg);

		bool traPackDataPackage(int sockID, void* arg);
		bool traPackFirstPackage(int sockID, void* arg);

		bool sockClose(int sockID, void* arg);

	private:
		encryptor* getEncryptor(int sockID) {
			uint8_t key[32] = { 1, 2, 3, 4, 5, 6 };
			uint8_t nonce[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };

			encryptor *enc = nullptr;
			auto item = encMap.find(sockID);
			if (item == encMap.end()) {
				enc = new encryptor(key, nonce);
				encMap[sockID] = enc;
			}else{
				enc = item->second;
			}
			return enc;
		}
};

REGISTER_PLUGIN("Crypto - ChaCha20", "Lutty", 0.01f, CryptoChaCha20);

#endif /* HEADER_CRYPTOCHACHA20_H_ */
