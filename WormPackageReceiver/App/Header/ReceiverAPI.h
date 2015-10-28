/*
 * ReceiverAPI.h
 *
 *  Created on: Aug 31, 2015
 *      Author: dlll
 */

#ifndef APP_HEADER_RECEIVERAPI_H_
#define APP_HEADER_RECEIVERAPI_H_

#include "../../Framework/Header/Util/AutoBuffer.h"

extern "C" {

	struct receiverConnectSock{
			in_addr ipAddr;
			uint16_t port;

			in_addr serverAddr;
			uint16_t serverPort;
	};

	struct receiverPackage{
			int sockID;
			Util::AutoBuffer::Buffer buf;
			int size {0};

			bool canNext {true};

			receiverPackage(int sockID, Util::AutoBuffer &autoBuf) :
					sockID(sockID), buf(autoBuf){
			}
	};

	struct receiverWritePackage{
			Util::AutoBuffer::Buffer buf;
			int size {0};
			int sentSize {0};
	};

	struct receiverSockIndividualData {
			bool canWrite {false};
			bool canRead {false};

			receiverWritePackage *lastWrite {nullptr};
	};
}

#endif /* APP_HEADER_RECEIVERAPI_H_ */
