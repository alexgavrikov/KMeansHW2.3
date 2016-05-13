/*
 * client_server.cpp
 *
 *  Created on: 9 ��� 2016 �.
 *      Author: user
 */

#include "client_server.h"
#include <cstdlib>
#include <memory>
#include <sstream>

std::string ClientServerBase::Recieve(int socket) const {
  std::string answer;
  char buf[1000000] = "";
  int res = recv(socket, buf, sizeof(buf), 0);
  std::string message_first_package(buf);
  const size_t length_info_end_pos = message_first_package.find(
      kDelimiterBetweenContentLengthAndContent);
  const size_t whole_message_length = std::atoi(
      message_first_package.substr(0, length_info_end_pos).c_str()) + length_info_end_pos
      + kDelimiterLength;
  answer.append(buf + length_info_end_pos + kDelimiterLength);

  for (size_t size_to_recv = whole_message_length - res; size_to_recv > 0; size_to_recv -= res) {
    char buf[1000000] = "";
    res = recv(socket, buf, sizeof(buf), 0);
    answer.append(buf);
  }

  return answer;
}

bool ClientServerBase::Send(const std::string& message, int socket) const {
  std::stringstream content_length_stream;
  content_length_stream << message.size() << kDelimiterBetweenContentLengthAndContent;
  std::string whole_message(content_length_stream.str() + message);
  const char* data = whole_message.c_str();
  for (size_t sz = whole_message.size(); sz > 0;) {
    int res = send(socket, data, sz, 0);
    if (res == 0) {
      return false;
    }
    if (res < 0) {
      throw std::runtime_error("error in send");
    }
    data += res;
    sz -= res;
  }
  return true;
}

bool ClientServerBase::ResolveHost(const std::string &host, int &addr) {
  hostent *ent = gethostbyname(host.c_str());
  if (ent == nullptr)
    return false;
  for (size_t i = 0; ent->h_addr_list[i]; ++i) {
    addr = *reinterpret_cast<int**>(ent->h_addr_list)[i];
    return true;
  }
  return false;
}

void LocalClient::Connect(const std::string &host, int port) {
  int addr;
  if (!ResolveHost(host, addr))
    throw std::runtime_error("can't resolve host");
  sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = addr;
  if (connect(server_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0)
    throw std::runtime_error("can't connect");
}

