#include "../include/network.hpp"

// Create connection request SYN packet
void Network::create_syn_packet(struct sockaddr_in *src,
                                struct sockaddr_in *dst,
                                std::unique_ptr<unsigned char[]> &packet,
                                int *packet_size) {
  uint16_t datagram_size =
      sizeof(struct iphdr) + sizeof(struct tcphdr) + OPT_SIZE;
  auto datagram = std::make_unique<unsigned char[]>(datagram_size);
  memset(datagram.get(), 0, datagram_size);

  // Ip, tcp headers
  struct iphdr *iph = reinterpret_cast<struct iphdr *>(datagram.get());
  struct tcphdr *tcph =
      reinterpret_cast<struct tcphdr *>(datagram.get() + sizeof(struct iphdr));

  // Ip header configuration
  iph->ihl = 5;
  iph->version = 4;
  iph->tos = 0;
  iph->tot_len = datagram_size;
  iph->id = htons(rand() & 65535);
  iph->frag_off = 0;
  iph->ttl = 64;
  iph->protocol = IPPROTO_TCP;
  iph->check = 0;
  iph->saddr = src->sin_addr.s_addr;
  iph->daddr = dst->sin_addr.s_addr;

  // TCP header configuration
  tcph->source = src->sin_port;
  tcph->dest = dst->sin_port;
  tcph->seq = htonl(100);
  tcph->ack_seq = htonl(0);
  tcph->doff = 10; // tcp header size
  tcph->fin = 0;
  tcph->syn = 1;
  tcph->rst = 0;
  tcph->psh = 0;
  tcph->ack = 0;
  tcph->urg = 0;
  tcph->check = 0;            // correct calculation follows later
  tcph->window = htons(5840); // window size
  tcph->urg_ptr = 0;

  // pseudo header to calculate checksum
  Network::pseudo_header psh;

  psh.src_addr = iph->saddr;
  psh.dst_addr = iph->daddr;
  psh.placeholder = 0;
  psh.protocol = IPPROTO_TCP;
  psh.tcp_length = htons(sizeof(struct tcphdr) + OPT_SIZE);

  uint16_t psize =
      sizeof(Network::pseudo_header) + sizeof(struct tcphdr) + OPT_SIZE;

  // fill pseudo packet
  std::vector<unsigned char> pseudogram(psize);

  memcpy(pseudogram.data(), &psh, sizeof(Network::pseudo_header));

  memcpy(pseudogram.data() + sizeof(Network::pseudo_header), tcph,
         sizeof(struct tcphdr) + OPT_SIZE);

  /* // TCP options
   unsigned char *options =
       datagram.get() + sizeof(struct iphdr) + sizeof(struct tcphdr);

   options[0] = 0x02;                           // MSS Option Kind
   options[1] = 0x04;                           // MSS Option Length
   uint16_t mss = htons(48);                    // MSS Value
   memcpy(options + 2, &mss, sizeof(uint16_t)); // Copy MSS value to options
   options[4] = 0x04;                           // SACK Permitted Option Kind
   options[5] = 0x02;                           // SACK Permitted Option Length

   // Do the same for pseudo header

   unsigned char *ps_options = pseudogram.data() +
                               sizeof(Network::pseudo_header) +
                               sizeof(struct tcphdr);
   ps_options[0] = 0x02;
   ps_options[1] = 0x04;
   memcpy(options + 2, &mss, sizeof(uint16_t));
   options[4] = 0x04;
   options[5] = 0x02;
 */
  uint16_t ipchk =
      Network::checksum(reinterpret_cast<unsigned short *>(iph), iph->tot_len);
  iph->check = htons(ipchk);

  uint16_t tcpchk = Network::checksum(
      reinterpret_cast<unsigned short *>(pseudogram.data()), psize);
  tcph->check = htons(tcpchk);

  packet = std::move(datagram);
  *packet_size = iph->tot_len;
}
// Create ACK packet
void Network::create_ack_packet(struct sockaddr_in *src,
                                struct sockaddr_in *dst, uint32_t seq,
                                uint32_t ack_seq,
                                std::unique_ptr<unsigned char[]> &packet,
                                int *packet_size) {
  uint16_t datagram_size =
      sizeof(struct iphdr) + sizeof(struct tcphdr) + OPT_SIZE;
  auto datagram = std::make_unique<unsigned char[]>(datagram_size);
  memset(datagram.get(), 0, datagram_size);

  // Ip, tcp headers
  struct iphdr *iph = reinterpret_cast<struct iphdr *>(datagram.get());
  struct tcphdr *tcph =
      reinterpret_cast<struct tcphdr *>(datagram.get() + sizeof(struct iphdr));

  // Ip header configuration
  iph->ihl = 5;
  iph->version = 4;
  iph->tos = 0;
  iph->tot_len = datagram_size;
  iph->id = htons(rand() & 65535);
  iph->frag_off = 0;
  iph->ttl = 64;
  iph->protocol = IPPROTO_TCP;
  iph->check = 0;
  iph->saddr = src->sin_addr.s_addr;
  iph->daddr = dst->sin_addr.s_addr;

  // TCP header configuration
  tcph->source = src->sin_port;
  tcph->dest = dst->sin_port;
  tcph->seq = htonl(seq);
  tcph->ack_seq = htonl(ack_seq);
  tcph->doff = 10; // tcp header size (data offset)
  tcph->fin = 0;
  tcph->syn = 0;
  tcph->rst = 0;
  tcph->psh = 0;
  tcph->ack = 1;
  tcph->urg = 0;
  tcph->check = 0;            // correct calculation follows later
  tcph->window = htons(5840); // window size
  tcph->urg_ptr = 0;

  // pseudo header to calculate checksum
  Network::pseudo_header psh;

  psh.src_addr = iph->saddr;
  psh.dst_addr = iph->daddr;
  psh.placeholder = 0;
  psh.protocol = IPPROTO_TCP;
  psh.tcp_length = htons(sizeof(struct tcphdr) + OPT_SIZE);

  size_t psize =
      sizeof(Network::pseudo_header) + sizeof(struct tcphdr) + OPT_SIZE;
  std::vector<unsigned char> pseudogram(psize);
  memcpy(pseudogram.data(), &psh, sizeof(Network::pseudo_header));
  memcpy(pseudogram.data() + sizeof(Network::pseudo_header), tcph,
         sizeof(struct tcphdr) + OPT_SIZE);

  uint16_t ipchk =
      Network::checksum(reinterpret_cast<unsigned short *>(iph), iph->ihl << 2);
  iph->check = htons(ipchk);

  uint16_t tcpchk = Network::checksum(
      reinterpret_cast<unsigned short *>(pseudogram.data()), psize);
  tcph->check = htons(tcpchk);

  packet = std::move(datagram);
  *packet_size = iph->tot_len;
}

// Create a data-filled packet
void Network::create_data_packet(struct sockaddr_in *src,
                                 struct sockaddr_in *dst, uint32_t seq,
                                 uint32_t ack_seq, const std::string &data,
                                 std::unique_ptr<unsigned char[]> &packet,
                                 int *packet_size) {
  uint16_t datagram_size =
      sizeof(struct iphdr) + sizeof(struct tcphdr) + data.size();
  auto datagram = std::make_unique<unsigned char[]>(datagram_size);
  memset(datagram.get(), 0, datagram_size);

  // Ip, tcp headers
  struct iphdr *iph = reinterpret_cast<struct iphdr *>(datagram.get());
  struct tcphdr *tcph =
      reinterpret_cast<struct tcphdr *>(datagram.get() + sizeof(struct iphdr));

  // Set payload
  std::copy(data.begin(), data.end(),
            datagram.get() + sizeof(struct iphdr) + sizeof(struct tcphdr));
  // IP header configuration
  iph->ihl = 5;
  iph->version = 4;
  iph->tos = 0;
  iph->tot_len = datagram_size;
  iph->id = htons(rand() % 65535); // id of this packet
  iph->frag_off = 0;
  iph->ttl = 64;
  iph->protocol = IPPROTO_TCP;
  iph->check = 0;                    // calculation follows later
  iph->saddr = src->sin_addr.s_addr; // source adress
  iph->daddr = dst->sin_addr.s_addr;

  // TCP header configuration
  tcph->source = src->sin_port;
  tcph->dest = dst->sin_port;
  tcph->seq = htonl(seq);
  tcph->ack_seq = htonl(ack_seq);
  tcph->doff = 5; // tcp header size
  tcph->fin = 0;
  tcph->syn = 0;
  tcph->rst = 0;
  tcph->psh = 1;
  tcph->ack = 1;
  tcph->urg = 0;
  tcph->check = 0;            // calculation follows later
  tcph->window = htons(5840); // window size
  tcph->urg_ptr = 0;

  // pseudo header to calculate checksum
  Network::pseudo_header psh;

  psh.src_addr = iph->saddr;
  psh.dst_addr = iph->daddr;
  psh.placeholder = 0;
  psh.protocol = IPPROTO_TCP;
  psh.tcp_length = htons(sizeof(struct tcphdr) + data.size());

  size_t psize =
      sizeof(Network::pseudo_header) + sizeof(struct tcphdr) + data.size();

  std::vector<unsigned char> pseudogram(psize);

  memcpy(pseudogram.data(), &psh, sizeof(Network::pseudo_header));
  memcpy(pseudogram.data() + sizeof(Network::pseudo_header), tcph,
         sizeof(struct tcphdr));
  memcpy(pseudogram.data() + sizeof(Network::pseudo_header) +
             sizeof(struct tcphdr),
         datagram.get() + sizeof(struct iphdr) + sizeof(struct tcphdr),
         data.size());

  uint16_t ipchk =
      Network::checksum(reinterpret_cast<unsigned short *>(iph), iph->tot_len);
  iph->check = htons(ipchk);

  uint16_t tcpchk = Network::checksum(
      reinterpret_cast<unsigned short *>(pseudogram.data()), psize);
  tcph->check = htons(tcpchk);

  packet = std::move(datagram);
  *packet_size = iph->tot_len;
}

int Network::create_socket(int domain, int type, int protocol) {
  int sockfd = socket(domain, type, protocol);
  if (sockfd < 0) {
    std::cerr << "Error: Failed to create socket" << strerror(errno)
              << std::endl;
    close_socket(sockfd);
    return -1;
  }
  return sockfd;
}

// Create server socket and set address struct
bool Network::create_server_socket(int &server_sockfd,
                                   struct sockaddr_in &server_addr,
                                   const char *ip, int port) {

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(ip);
  server_addr.sin_port = htons(port);
  if (server_sockfd > 0) {
    Network::close_socket(server_sockfd);
  }

  server_sockfd = Network::create_socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
  if (server_sockfd < 0) {
    std::cerr << "Error: Failed to create server socket " << strerror(errno)
              << std::endl;
    Network::close_socket(server_sockfd);
    return false;
  }
  // tell the kernel that headers are included in the packet
  int one = 1;
  const int *val = &one;
  if (setsockopt(server_sockfd, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) ==
      -1) {
    std::cerr << "setsockopt(IP_HDRINCL, 1) failed" << strerror(errno)
              << std::endl;
    return -1;
  }
  std::cout << "\n\nSelf adress: " << ip << ":" << port << std::endl;
  return true;
}

// Create client socket
int Network::create_client_socket(int &client_sockfd,
                                  struct sockaddr_in &client_addr,
                                  const char *ip) {
  memset(&client_addr, 0, sizeof(client_addr));
  client_addr.sin_family = AF_INET;
  client_addr.sin_addr.s_addr = inet_addr(ip);
  srand(time(nullptr));
  int port = rand() % 65535;
  client_addr.sin_port = htons(port);

  std::cout << "\n\nSelf adress: " << ip << ":" << port << std::endl;

  if (client_sockfd != 0) {
    close_socket(client_sockfd);
  }

  client_sockfd = create_socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
  if (client_sockfd < 0) {
    std::cerr << "Error: Failed to create client socket " << strerror(errno)
              << std::endl;
    return -1;
  }
  // Bind the client socket to a local address
  bind_to_port(port, client_sockfd, client_addr);
  // tell the kernel that headers are included in the packet
  int one = 1;
  const int *val = &one;
  if (setsockopt(client_sockfd, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) ==
      -1) {
    std::cerr << "setsockopt(IP_HDRINCL, 1) failed" << std::endl;
    return -1;
  }
  return client_sockfd;
}

// bind to a port
bool Network::bind_to_port(int port, int &sockfd, struct sockaddr_in &addr) {
  addr.sin_port = htons(port);
  try {
    if (bind(sockfd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) <
        0) {
      // Check if the error is "Address already in use"
      if (errno == EADDRINUSE) {
        std::cerr << "Error: Address already in use" << std::endl;
        Network::close_socket(sockfd);
        return false;
      }
      // For other errors, throw an exception
      throw std::runtime_error(std::string("Error: Failed to bind: ") +
                               strerror(errno));
    }
    return true;
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    Network::close_socket(sockfd);
    return false;
  }
}

/**
 * @brief Write source address to passed source param
 * check flags and sequences numbers
 * parse payload, it will then be assigned to initial passed packet param
 * recalculate and compare checksums
 * log into console
 */
void Network::parse_packet(std::unique_ptr<unsigned char[]> &packet,
                           uint32_t *seq, uint32_t *ack,
                           struct sockaddr_in &source) {
  /*------------------------- READ PACKET -------------------------*/
  struct iphdr *iph = reinterpret_cast<struct iphdr *>(packet.get());
  unsigned short iphdrlen = iph->ihl * 4;
  struct tcphdr *tcph =
      reinterpret_cast<struct tcphdr *>(packet.get() + iphdrlen);
  unsigned short tcphdrlen = tcph->doff * 4;
  source.sin_port = tcph->source;
  source.sin_addr.s_addr = iph->saddr;

  char source_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(source.sin_addr), source_ip, INET_ADDRSTRLEN);
  std::cout << "\n\tsource address: " << source_ip << ":" << ntohs(tcph->source)
            << std::endl;
  // Read sequence number
  uint32_t seq_num = tcph->seq;
  // Read acknowledgement number
  uint32_t ack_num = tcph->ack_seq;
  // Check if it's SYN
  bool syn = tcph->syn;
  // Check delivery completeness
  *seq = ntohl(seq_num);
  *ack = ntohl(ack_num);
  /*---------------------------------------------------------------*/

  //-------------------------------------------------------------------|

  /*------------------------ PARSE PAYLOAD ------------------------*/
  unsigned int payload_size = ntohs(iph->tot_len) - (iphdrlen + tcphdrlen);
  if (payload_size > 0 && payload_size < DATAGRAM_SIZE) {
    auto payload = std::make_unique<unsigned char[]>(payload_size);
    memcpy(payload.get(), packet.get() + (iphdrlen + tcphdrlen), payload_size);
    packet = std::move(payload);
  }
  /*--------------------------------------------------------------*/

  //-------------------------------------------------------------------|

  /*---------------------- COMPARE CHECKSUMS ---------------------*/

  /*------------------------- TCP CHECKSUM -----------------------*/
  uint16_t recv_tcp_chk =
      ntohs(tcph->check); // Convert received checksum to host byte order
  tcph->check = 0;        // Set to 0 for checksum calculation

  // Pseudo header to calculate checksum
  Network::pseudo_header psh;
  psh.src_addr = iph->saddr;
  psh.dst_addr = iph->daddr;
  psh.placeholder = 0;
  psh.protocol = IPPROTO_TCP;
  psh.tcp_length =
      htons(tcphdrlen + payload_size); // Length of TCP header + payload

  size_t psize = sizeof(Network::pseudo_header) + tcphdrlen + payload_size;

  std::vector<unsigned char> pseudogram(psize);
  memcpy(pseudogram.data(), &psh, sizeof(Network::pseudo_header));
  memcpy(pseudogram.data() + sizeof(Network::pseudo_header), tcph, tcphdrlen);
  memcpy(pseudogram.data() + sizeof(Network::pseudo_header) + tcphdrlen,
         packet.get() + iphdrlen + tcphdrlen, payload_size);

  uint16_t calc_tcp_chk = Network::checksum(
      reinterpret_cast<unsigned short *>(pseudogram.data()), psize);

  tcph->check = htons(
      recv_tcp_chk); // Restore the original checksum in network byte order
  bool tcp_chk_match = (recv_tcp_chk == calc_tcp_chk);

  /*------------------------- IP CHECKSUM ------------------------*/
  uint16_t recv_ip_chk =
      (iph->check); // Convert received checksum to host byte order
  iph->check = 0;   // Clear the checksum field for calculation
  uint16_t calc_ip_chk =
      Network::checksum(reinterpret_cast<unsigned short *>(iph), iphdrlen);

  iph->check =
      (recv_ip_chk); // Restore the original checksum in network byte order
  bool ip_chk_match = (recv_ip_chk == calc_ip_chk);
  /*--------------------------------------------------------------*/

  // If either checksum don't match -> log
  if (!tcp_chk_match || !ip_chk_match) {
    std::cout << "\tPacket checksums don't match, malformed" << std::endl;
    std::cout << "\tip->" << recv_ip_chk << ":" << calc_ip_chk << std::endl;
    std::cout << "\ttcp->" << recv_tcp_chk << ":" << calc_tcp_chk << std::endl;
  }

  std::cout << "\tSYN: " << syn << std::endl;
  std::cout << "\tSEQ: " << *seq << std::endl;
  std::cout << "\tACK: " << *ack << std::endl << std::endl;
}

// calculate checksum
unsigned short Network::checksum(void *buffer, unsigned len) {
  // credits: Addison Wesley: UNIX Network Programming
  unsigned short *buf = (unsigned short *)buffer;
  unsigned int sum = 0;
  unsigned short result;

  for (sum = 0; len > 1; len -= 2)
    sum += *buf++;

  if (len == 1)
    sum += ((*buf) & htons(0xFF00));

  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }
  sum = ~sum;
  return ((unsigned short)sum);
}

// Receive and parse SYN
bool Network::listen_client(int &server_sockfd, int numcl,
                            struct sockaddr_in &server_addr,
                            std::vector<struct sockaddr_in> &clients) {
  std::cout << "\n\nListening for incoming connection..." << std::endl;
  auto syn_req = std::make_unique<unsigned char[]>(REQUEST_SIZE);
  uint32_t seq_num, ack_num;
  struct iphdr *ip_header;
  struct tcphdr *tcp_header;
  bool syn = false;

  sockaddr_in client;
  memset(&client, 0, sizeof(client));
  clients.push_back(client);

  // TODO: Define the duration for which the loop should run
  do {
    // here by default we're receiving all packets designated to us
    ssize_t bytes_recv = Network::receive_packet(server_sockfd, syn_req.get(),
                                                 REQUEST_SIZE, server_addr);
    // and here we check if it's a new client
    // or just communication on another  thread
    ip_header = reinterpret_cast<struct iphdr *>(syn_req.get());
    tcp_header = reinterpret_cast<struct tcphdr *>(
        (reinterpret_cast<char *>(syn_req.get()) + (ip_header->ihl * 4)));
    // check if source port is equal to already known client
    syn = (tcp_header->syn);
  } while (!syn);

  std::cout << "\n\nSYN-RECEIVED:" << syn << std::endl;

  // Parse packet to acknowledge new client adress
  clients.back().sin_port = (tcp_header->source);
  clients.back().sin_addr.s_addr = (ip_header->saddr);

  //  Parse packet contents
  Network::parse_packet(syn_req, &seq_num, &ack_num, clients.back());
  return true;
}

// Send SYN wait for SYN-ACK
bool Network::connect_to_server(int &client_sockfd,
                                struct sockaddr_in &client_addr,
                                struct sockaddr_in &server_addr, const char *ip,
                                int port, uint32_t *seq_num,
                                uint32_t *ack_num) {
  //  Set server's ip and port number
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  if (inet_pton(AF_INET, ip, &server_addr.sin_addr) != 1) {
    std::cerr << "destination IP configuration failed" << strerror(errno)
              << std::endl;
    return -1;
  }
  std::cout << "\n\nConnection adress: " << ip << ":" << port << std::endl;
  // Try to connect to server
  // TODO: send SYN until received SYN-ACK or TIMEOUT
  //  sleep_for = 5; // in seconds
  std::unique_ptr<unsigned char[]> SYN;
  auto response = std::make_unique<unsigned char[]>(REQUEST_SIZE);
  int packet_size{0};
  Network::create_syn_packet(&client_addr, &server_addr, SYN, &packet_size);

  Network::send_packet(client_sockfd, SYN.get(), packet_size, server_addr);
  packet_size = 0;

  std::cout << "\n\nSYN-SENT" << std::endl;

  Network::receive_packet(client_sockfd, response.get(), DATAGRAM_SIZE,
                          client_addr);
  std::cout << "\n\nESTABLISHED:" << std::endl;
  Network::parse_packet(response, seq_num, ack_num, server_addr);

  std::unique_ptr<unsigned char[]> ACK;

  Network::create_ack_packet(&client_addr, &server_addr, 101, 201, ACK,
                             &packet_size);
  Network::send_packet(client_sockfd, ACK.get(), packet_size, server_addr);
  return true;
}

// Send SYN-ACK
int Network::accept_connection(int &server_sockfd,
                               struct sockaddr_in &server_addr,
                               std::vector<struct sockaddr_in> &clients) {
  // send ACK packet
  std::unique_ptr<unsigned char[]> ACK;
  int packet_size{0};
  uint32_t seq_num, ack_num;
  struct iphdr *ip_header;
  struct tcphdr *tcp_header;
  int src_port{0};
  uint32_t src_ack;
  Network::create_ack_packet(&server_addr, &clients.back(), 200, 101, ACK,
                             &packet_size);

  /*char source_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(client_addr.sin_addr), source_ip, INET_ADDRSTRLEN);
  std::cout << "\n\nclient address: " << source_ip << ":"
            << ntohs(client_addr.sin_port) << std::endl;*/

  Network::send_packet(server_sockfd, ACK.get(), packet_size, clients.back());
  std::cout << "\n\nSYN-ACK " << std::endl;

  auto established = std::make_unique<unsigned char[]>(REQUEST_SIZE);

  do {
    Network::receive_packet(server_sockfd, established.get(), REQUEST_SIZE,
                            server_addr);
    ip_header = reinterpret_cast<struct iphdr *>(established.get());
    tcp_header = reinterpret_cast<struct tcphdr *>(
        (reinterpret_cast<char *>(established.get() + ip_header->ihl * 4)));

    src_port = tcp_header->source;
    src_ack = tcp_header->ack;
  } while (src_ack != 1);

  std::cout << "\n\nESTABLISHED: " << std::endl;
  Network::parse_packet(established, &seq_num, &ack_num, clients.back());
  return true;
}

ssize_t Network::send_packet(int sockfd, void *packet, size_t packet_len,
                             struct sockaddr_in &dest) {
  ssize_t bytes_sent = sendto(sockfd, packet, packet_len, 0,
                              reinterpret_cast<struct sockaddr *>(&dest),
                              sizeof(struct sockaddr_in));
  if (bytes_sent < 0) {
    std::cerr << "Error sending  packet: " << strerror(errno) << std::endl;
  }
  return bytes_sent;
}

/**
 * @brief Listen for all packets, filter by caller's port
 */
ssize_t Network::receive_packet(int sockfd, void *buffer, size_t buffer_len,
                                struct sockaddr_in &dest) {
  struct iphdr *ip_header;
  struct tcphdr *tcp_header;
  ssize_t bytes_recv;
  uint16_t dst_port{0};

  do {
    bytes_recv = recvfrom(sockfd, buffer, buffer_len, 0, NULL, NULL);
    if (bytes_recv < 0) {
      std::cerr << "Error receiving packet: " << strerror(errno) << std::endl;
      return -1;
    }

    ip_header = reinterpret_cast<struct iphdr *>(buffer);
    tcp_header = reinterpret_cast<struct tcphdr *>(
        (reinterpret_cast<char *>(buffer)) + (ip_header->ihl * 4));
    dst_port = ntohs(tcp_header->dest);

  } while (dst_port != ntohs(dest.sin_port));
  return bytes_recv;
}

void Network::close_socket(int socket_fd) {
  close(socket_fd);
  // std::cout << "Socket was closed" << std::endl;
}
