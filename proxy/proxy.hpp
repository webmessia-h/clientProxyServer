#include "../client/client.hpp"
#include "../server/server.hpp"
#include "../shared_resources/include/network.hpp"
#include "../shared_resources/include/threadpool.hpp"
#include <cerrno>
#include <iostream>
#include <netinet/in.h>

class Proxy : public Server, public Client {
public:
  Proxy(const std::string &prx_ip, int prx_port, const std::string &server_ip,
        int server_port);
  ~Proxy();

  // Do the funny
  void receive_request(std::string &data) override;
  // TODO: split this into separate methods
  void relay_data();

private:
  std::string prx_ip, srv_ip;
  int srv_port, prx_port;
  std::shared_ptr<ThreadPool> threadPool;
};
