/*
 * client.h
 *
 *  Created on: 10 мая 2016 г.
 *      Author: user
 */

#ifndef CLIENT_CLIENT_H_
#define CLIENT_CLIENT_H_

#include "../Common/client_server.h"
#include "../Common/types.h"

class KMeansClient : private LocalClient {
public:
  KMeansClient(const std::string& host, int port)
      : LocalClient(host, port) {
  }

  Points DoKMeansWork(const std::string& data_message,
                      const size_t K,
                      const size_t dimensions) const;
  static std::string MoveDataToMessage(Points& data, const size_t K);
};

#endif /* CLIENT_CLIENT_H_ */
