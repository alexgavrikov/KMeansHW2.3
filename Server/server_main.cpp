/*
 * server_main.cpp
 *
 *  Created on: 10 мая 2016 г.
 *      Author: user
 */

#include <cstdlib>
#include <fstream>
#include "server.h"

int main(int argc, char** argv) {

  if (argc != 5) {
    std::printf("Usage: %s port_for_slaves slaves_num port_for_clients k_means_jobs_num\n",
                argv[0]);
    return 1;
  }
  const size_t slaves_port = std::atoi(argv[1]);
  const size_t slaves_count = std::atoi(argv[2]);
  const size_t clients_port = std::atoi(argv[3]);
  const size_t k_means_jobs_count = std::atoi(argv[4]);

  KMeansHeadServer k_means_head_server;
  k_means_head_server.PrepareCluster(slaves_count, slaves_port, "");
  k_means_head_server.StartListening(clients_port, "");

  for (size_t k_means_job_index = 0; k_means_job_index != k_means_jobs_count; ++k_means_job_index) {
    srand(123);
    k_means_head_server.Run();
  }

  return 0;
}

