/*
 * client.cpp
 *
 *  Created on: 10 мая 2016 г.
 *      Author: user
 */

#include "client.h"
#include "../Common/help_functions.h"

Points KMeansClient::DoKMeansWork(const std::string& data_message,
                                  const size_t K,
                                  const size_t dimensions) const {
  Send(data_message);
  std::stringstream centroids_message_stream(Recieve());
  return ExtractPointsFromMessage(centroids_message_stream, K, dimensions);
}

std::string KMeansClient::MoveDataToMessage(Points& data, const size_t K) {
  std::stringstream message_stream;
  const size_t data_size = data.size();
  const size_t dimensions = data[0].size();

  message_stream.precision(20);
  message_stream << data_size << kComponentsDelimiter << dimensions << kComponentsDelimiter << K
      << kComponentsDelimiter;
  MovePointsToMessage(message_stream, data.begin(), data.end());
  Points().swap(data);

  return message_stream.str();
}

