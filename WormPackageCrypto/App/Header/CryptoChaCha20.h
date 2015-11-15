/*
 * CryptoChaCha20.h
 *
 *  Created on: Oct 23, 2015
 *      Author: Eclipse C++
 */

#ifndef HEADER_CRYPTOCHACHA20_H_
#define HEADER_CRYPTOCHACHA20_H_

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/rand.h>
#include <cstdint>
#include <tr1/unordered_map>

#include "chacha20.hpp"
#include "WormPlugin.h"

#define NID NID_X9_62_prime256v1

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

		struct CryptoInfo {
				uint8_t ID[8];
				uint8_t DATA_KEY[32];

				BN_CTX *ECC_CTX { nullptr };
				EC_KEY *ECC_KEY;
				const EC_GROUP *ECC_GROUP;

				BIGNUM *RANDOM_MAX, *RANDOM;

				CryptoInfo(const std::string PUK_X, const std::string PUK_Y) {
					ECC_CTX = BN_CTX_new();
					if (ECC_CTX == NULL) /*ERR()*/;

					ECC_KEY = EC_KEY_new_by_curve_name(NID);

					BIGNUM *publicKey_X = BN_new();
					BIGNUM *publicKey_Y = BN_new();

					//TODO: 解密 public Key
					BN_hex2bn(&publicKey_X, PUK_X.c_str());
					BN_hex2bn(&publicKey_Y, PUK_Y.c_str());

					if (!EC_KEY_set_public_key_affine_coordinates(ECC_KEY, publicKey_X, publicKey_Y)) /*ERR()*/;
					BN_free(publicKey_X);
					BN_free(publicKey_Y);

					ECC_GROUP = EC_KEY_get0_group(ECC_KEY);
					RANDOM_MAX = BN_new();
					RANDOM = BN_new();
					BN_hex2bn(&RANDOM_MAX, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
				}

				CryptoInfo(const std::string PRK) {
					ECC_CTX = BN_CTX_new();
					if (ECC_CTX == NULL) /*ERR()*/;

					ECC_KEY = EC_KEY_new_by_curve_name(NID);

					BIGNUM *privateKey = BN_new();

					//TODO: 解密 private Key
					BN_hex2bn(&privateKey, PRK.c_str());

					EC_KEY_set_private_key(ECC_KEY, privateKey);
					BN_free(privateKey);

					ECC_GROUP = EC_KEY_get0_group(ECC_KEY);
					RANDOM_MAX = BN_new();
					RANDOM = BN_new();
					BN_hex2bn(&RANDOM_MAX, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
				}

				~CryptoInfo() {
					BN_CTX_free(ECC_CTX);
					EC_KEY_free(ECC_KEY);
					BN_free(RANDOM_MAX);
					BN_free(RANDOM);
				}

				void GenerationRandom() {
					do {
						if (!BN_rand_range(RANDOM, RANDOM_MAX)) /*ERR()*/;
					} while (BN_is_zero(RANDOM));
				}
		};

	private:
		WormHook* wHook { nullptr };
		CryptoInfo* info { nullptr };
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
		bool initSetting(WormTool &tool);
		int encode_ECC(const EC_GROUP *group, EC_POINT *C1, EC_POINT *C2, uint8_t *to, BN_CTX *ctx);
		bool decode_ECC(const EC_GROUP *group, EC_POINT *C1, EC_POINT *C2, uint8_t *from, BN_CTX *ctx);

		encryptor* getEncryptor(const int sockID, uint8_t *nonce = nullptr) {
			encryptor *enc = nullptr;
			auto item = encMap.find(sockID);
			if (item == encMap.end()) {
				if (nonce == nullptr) {
					return nullptr;
				}
				enc = new encryptor(info->DATA_KEY, nonce);
				encMap[sockID] = enc;
			} else {
				enc = item->second;
			}
			return enc;
		}
};

REGISTER_PLUGIN("Crypto - ChaCha20", "Lutty", 0.01f, CryptoChaCha20);

#endif /* HEADER_CRYPTOCHACHA20_H_ */
