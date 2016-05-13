/*
 * server.cpp
 *
 *  Created on: 10 ��� 2016 �.
 *      Author: user
 */

#include "server.h"
#include <iostream>
#include <memory>
#include <random>
#include "../Common/help_functions.h"

void KMeansHeadServer::PrepareCluster(const size_t slaves_count,
                                      int slaves_port,
                                      const std::string& host) {
  int slaves_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
  Bind(slaves_listen_socket, slaves_port, host);
  slave_clients_sockets.reserve(slaves_count);
  for (size_t slave_index = 0; slave_index != slaves_count; ++slave_index) {
    slave_clients_sockets.push_back(accept(slaves_listen_socket, nullptr, nullptr));
  }
}

void KMeansHeadServer::StartListening(int port, const std::string& host) {
  clients_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
  Bind(clients_listen_socket, port, host);
}

void KMeansHeadServer::Run() const {
  std::cout << "READY" << std::endl;
  int kmeans_client_socket = accept(clients_listen_socket, nullptr, nullptr);
  std::cout << "BUSY" << std::endl;
  std::string initial_data_message = Recieve(kmeans_client_socket);
  Points centroids = DoKMeansJob(initial_data_message);
  std::stringstream centroids_message_stream;
  centroids_message_stream.precision(20);
  InsertPointsToMessage(centroids_message_stream, centroids.cbegin(), centroids.cend());
  Send(centroids_message_stream.str(), kmeans_client_socket);

  close(kmeans_client_socket);
}

void KMeansHeadServer::Bind(int listen_socket, int port, const std::string &host) const {
  sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  if (!host.empty()) {
    int addr;
    if (!ResolveHost(host, addr))
      throw std::runtime_error("can't resolve host");
    address.sin_addr.s_addr = addr;
  } else {
    address.sin_addr.s_addr = INADDR_ANY;
  }
  if (bind(listen_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0)
    throw std::runtime_error("can't bind");
  if (listen(listen_socket, 1) < 0)
    throw std::runtime_error("can't start listening");
}

Points KMeansHeadServer::DoKMeansJob(const std::string& data_str) const {
  // Here will be some routine with std::string instead of std::strinstream. Yes, I know
  // std::stringstream is good, but it is quite slow and data might be quite big. So
  // I use std::stringstream very carefully only on small data. The thing is that we
  // received string from client so we don't need to wrap data into string using
  // stringstream because it has been already wrapped. We may need extracting data from string
  // because we need to initialize centroids randomly using random data points. And extracting is
  // not as slow as inserting into stringstream. But still it is slow. And we can do without it.
  // Yes, without proper naive extracting data from stringstream InitCentroidsRandomly looks complicated.
  // But it's worth it. GiveJobsToSlaves is much faster.
  const size_t intro_info_length = data_str.find(kNewPointIndicator);
  std::stringstream intro_info_stream(data_str.substr(0, intro_info_length));
  size_t data_size, dimensions, K;
  char ch;
  intro_info_stream >> data_size >> ch >> dimensions >> ch >> K >> ch;
  Points centroids = InitCentroidsRandomly(data_str, data_size, dimensions, K);
  GiveJobsToSlaves(data_str, data_size, dimensions, K);
  for (bool converged = false; !converged;) {
    SendCentroidsToSlaves(centroids);
    converged = CheckIfConverged();
    std::string message = converged ? "BYE" : "OK";
    for (const auto& slave_socket : slave_clients_sockets) {
      Send(message, slave_socket);
    }
    if (!converged) {
      centroids = GetRefreshedCentroids(dimensions, K);
    }
  }

  return centroids;
}

size_t KMeansHeadServer::FindCurrentPortionEndPos(const std::string& data_str,
                                                  const size_t portion_beg_pos,
                                                  const size_t portion_size) {
  size_t current_pos = portion_beg_pos;
  for (size_t points_counter = 0; points_counter != portion_size; ++points_counter) {
    current_pos = data_str.find(kNewPointIndicator, current_pos + 1);
  }

  return current_pos;
}

void KMeansHeadServer::GiveJobsToSlaves(const std::string& data_str,
                                        const size_t data_size,
                                        const size_t dimensions,
                                        const size_t K) const {
  const double portion_size = static_cast<double>(data_size) / slave_clients_sockets.size();
  size_t current_portion_beg_pos = data_str.find(kNewPointIndicator);
  for (size_t slave_index = 0; slave_index != slave_clients_sockets.size(); ++slave_index) {
    const size_t portion_begin_index = slave_index * portion_size;
    const size_t portion_end_index = (slave_index + 1) * portion_size;
    const size_t current_slave_portion_size = portion_end_index - portion_begin_index;
    const size_t current_portion_end_pos =
        (portion_end_index == data_size) ?
            data_str.size() :
            FindCurrentPortionEndPos(data_str, current_portion_beg_pos, portion_size);
    std::stringstream intro_info_stream;
    intro_info_stream << current_slave_portion_size << kComponentsDelimiter << dimensions
        << kComponentsDelimiter << K << kComponentsDelimiter;
    std::string final_message = intro_info_stream.str()
        + data_str.substr(current_portion_beg_pos,
                          current_portion_end_pos - current_portion_beg_pos);
    Send(final_message, slave_clients_sockets[slave_index]);
    Recieve(slave_clients_sockets[slave_index]);
    current_portion_beg_pos = current_portion_end_pos;
  }
}

void KMeansHeadServer::SendCentroidsToSlaves(const Points& centroids) const {
  std::stringstream centroids_message_stream;
  centroids_message_stream.precision(20);
  InsertPointsToMessage(centroids_message_stream, centroids.cbegin(), centroids.cend());
  for (const auto& slave_socket : slave_clients_sockets) {
    Send(centroids_message_stream.str(), slave_socket);
  }
}

bool KMeansHeadServer::CheckIfConverged() const {
  bool converged = true;
  for (const auto& slave_socket : slave_clients_sockets) {
    std::stringstream message(Recieve(slave_socket));
    bool slave_portion_converged;
    message >> slave_portion_converged;
    converged = converged && slave_portion_converged;
  }

  return converged;
}

Points KMeansHeadServer::GetRefreshedCentroids(const size_t dimensions, const size_t K) const {
  std::vector<size_t> clusters_sizes(K);
  Points centroids(K, Point(dimensions));

  for (const auto& slave_socket : slave_clients_sockets) {
    std::stringstream message(Recieve(slave_socket));
    Points centroids_parts = ExtractPointsFromMessage(message, K, dimensions);
    CombineCentroids(centroids, centroids_parts);

    std::vector<size_t> clusters_subsizes = ExtractClustersSubsizesFromMessage(message, K);
    CombineClustersSizes(clusters_sizes, clusters_subsizes);
  }

  for (size_t i = 0; i < K; ++i) {
    if (clusters_sizes[i] != 0) {
      for (size_t d = 0; d < dimensions; ++d) {
        centroids[i][d] /= clusters_sizes[i];
      }
    }
  }

  for (size_t i = 0; i < K; ++i) {
    if (clusters_sizes[i] == 0) {
      centroids[i] = GetRandomPosition(centroids);
    }
  }

  return centroids;
}

// Gives random number in range [0..max_value]
size_t KMeansHeadServer::UniformRandom(const size_t max_value) {
  unsigned int rnd = ((static_cast<unsigned int>(rand()) % 32768) << 17)
      | ((static_cast<unsigned int>(rand()) % 32768) << 2) | rand() % 4;
  return ((max_value + 1 == 0) ? rnd : rnd % (max_value + 1));
}

// Calculates new centroid position as mean of positions of 3 random centroids
Point KMeansHeadServer::GetRandomPosition(const Points& centroids) {
  size_t K = centroids.size();
  int c1 = rand() % K;
  int c2 = rand() % K;
  int c3 = rand() % K;
  size_t dimensions = centroids[0].size();
  Point new_position(dimensions);
  for (size_t d = 0; d < dimensions; ++d) {
    new_position[d] = (centroids[c1][d] + centroids[c2][d] + centroids[c3][d]) / 3;
  }
  return new_position;
}

// Initialize centroids randomly at data points. Quite complicated because I
// refused to use std::stringstream here.
Points KMeansHeadServer::InitCentroidsRandomly(const std::string& data_str,
                                               const size_t data_size,
                                               const size_t dimensions,
                                               const size_t K) {
  Points centroids(K, Point(dimensions));
  for (size_t i = 0; i < K; ++i) {
    size_t random_point_beg_position = std::string::npos;
    while (random_point_beg_position == std::string::npos) {
      const size_t data_index = UniformRandom(data_size - 1);
      // Roughly, but it is okay.
      const size_t approximate_random_point_area = static_cast<double>(data_index) / data_size
          * data_str.size();
      random_point_beg_position = data_str.find(kNewPointIndicator, approximate_random_point_area);
    }
    size_t random_point_end_position = data_str.find(kNewPointIndicator,
                                                     random_point_beg_position + 1);
    const size_t len =
        (random_point_end_position == std::string::npos) ?
            std::string::npos : (random_point_end_position - random_point_beg_position);
    std::stringstream random_point_components(data_str.substr(random_point_beg_position, len));
    char delimiter;
    random_point_components >> delimiter;
    for (double& component : centroids[i]) {
      random_point_components >> component >> delimiter;
    }
  }

  return centroids;
}

std::vector<size_t> KMeansHeadServer::ExtractClustersSubsizesFromMessage(std::stringstream& stream,
                                                                         const size_t K) {
  std::vector<size_t> clusters_subsizes(K);
  char delimiter;
  for (size_t index = 0; index != K; ++index) {
    stream >> clusters_subsizes[index] >> delimiter;
  }

  return clusters_subsizes;
}

void KMeansHeadServer::CombineCentroids(Points& centroids, const Points& centroids_parts) {
  for (size_t point_index = 0; point_index != centroids.size(); ++point_index) {
    auto& centroid = centroids[point_index];
    const auto& centroid_to_add = centroids_parts[point_index];
    for (size_t component_index = 0; component_index != centroid.size(); ++component_index) {
      centroid[component_index] += centroid_to_add[component_index];
    }
  }
}

void KMeansHeadServer::CombineClustersSizes(std::vector<size_t>& clusters_sizes,
                                            const std::vector<size_t>& clusters_subsizes) {
  for (size_t index = 0; index != clusters_sizes.size(); ++index) {
    clusters_sizes[index] += clusters_subsizes[index];
  }
}
