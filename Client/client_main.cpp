/*
 * client.cpp
 *
 *  Created on: 9 мая 2016 г.
 *      Author: user
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include "client.h"
#include "../Common/read_write.h"

int main(int argc, char** argv) {
  static const std::string kHost = "127.0.0.1";
  if (argc != 5) {
    std::printf(
        "Usage: %s number_of_clusters input_file output_file server_port\n",
        argv[0]);
    return 1;
  }

  const size_t K = std::atoi(argv[1]);
  char* input_file = argv[2];
  std::ifstream input;
  input.open(input_file, std::ifstream::in);
  if (!input) {
    std::cerr << "Error: input file could not be opened" << std::endl;
    return 1;
  }

  Points data = ReadPoints(input);
  const size_t dimensions = data[0].size();
  input.close();

  char* output_file = argv[3];
  std::ofstream output;
  output.open(output_file, std::ifstream::out);
  if (!output) {
    std::cerr << "Error: output file could not be opened" << std::endl;
    return 1;
  }

  std::string message = KMeansClient::MoveDataToMessage(data, K);
  const size_t server_port = std::atoi(argv[4]);
  KMeansClient k_means_client(kHost, server_port);
  Points centroids = k_means_client.DoKMeansWork(message, K, dimensions);

  WriteOutput(centroids, output);
  output.close();

  return 0;
}
