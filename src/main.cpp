#include "pong_client.h"
#include "pong_server.h"

#include <cstdio>
#include <cstring>

using namespace pong;

int main(int argc, char* argv[]) {
  if (argc == 2 && strncmp(argv[1], "--server", 8) == 0) {
    printf("Starting the application as a server.\n");
    Server server;
    server.start();
  } else if (argc == 3 && strncmp(argv[1], "--client", 8) == 0) {
    printf("Starting the application as a client.\n");
    Client client(argv[2]);
    client.start();
  } else {
    printf("Network Pong version v0.1\n");
    printf("usage: pong.exe [options]\n");
    printf("options:\n");
    printf("  --server\t\t run the application as a server.\n");
    printf("  --client <host>\t run the application as a client by connecting to the given host.\n");
  }
  return 0;
}
