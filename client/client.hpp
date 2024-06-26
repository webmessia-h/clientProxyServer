// client
#pragma once
#include "../shared_resources/include/network.hpp"

class Client {
public:
  Client(const std::string s_ip, const std::string ip, const int port);
  virtual ~Client();

  bool connect();
  // TODO: maybe implement some authentication
  //  so the proxy ain't meaningless
  void send_request(const std::string &data);
  virtual void receive_response(std::string &data);

protected:
  int client_sockfd;
  struct sockaddr_in srv_addr;
  struct sockaddr_in clt_addr;

private:
  std::string self_ip;
  std::string ip;
  int port;

  uint32_t seq_num, ack_num = 0;
};
