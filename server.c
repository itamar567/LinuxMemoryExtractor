#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <endian.h>

#define MAX_PENDING_CONN_QUEUE_LENGTH 10
#define MAX_BYTES_FOR_FILE_SIZE 2

int main(int argc, char const* argv[]){
  // Handle command-line args
  if (argc < 2) {
    printf("LinuxMemoryExtractor Client\n");
    printf("Usage: %s <port>\n", argv[0]);
    printf("\nERROR: port not specified\n");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(atoi(argv[1]));

  socklen_t address_size = sizeof(address);

  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  if (bind(socket_fd, (struct sockaddr*) &address, address_size) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  if (listen(socket_fd, MAX_PENDING_CONN_QUEUE_LENGTH) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  printf("Server started\n");

  while (1){
    int connected_socket = accept(socket_fd, (struct sockaddr*) &address, &address_size);
    if (connected_socket < 0) {
      perror("accept");
      continue;
    }

    printf("Accepted new connection\n");

    int data_fd = memfd_create("client_data", 0);
    if (data_fd < 0) {
      perror("memfd_create");
      continue;
    }

    int finished = 0;
    while (!finished){
      unsigned char header_buffer[MAX_BYTES_FOR_FILE_SIZE + 1];
      if (read(connected_socket, header_buffer, MAX_BYTES_FOR_FILE_SIZE + 1) < 0) { // +1 for the 'last packet' flag
        perror("read");
        break;
      }

      int data_size = 0;
      for (int i = 0;i < MAX_BYTES_FOR_FILE_SIZE;i++) {
        data_size += header_buffer[i] << (8 * i);
      }

      int last_packet = header_buffer[MAX_BYTES_FOR_FILE_SIZE];

      printf("Receiving packet (Length = %d, Last = %d)\n", data_size, last_packet);

      void* data_buffer[data_size];

      if (read(connected_socket, data_buffer, data_size) < 0) { // +1 for the 'last packet' flag
        perror("read");
        break;
      }

      if (write(data_fd, data_buffer, data_size) < 0) {
        perror("write");
        break;
      }

      if (last_packet) {
        finished = 1;
      }
    }
    
    if (!finished) { // An error occured
      continue;
    }

    printf("Received all packets, executing program\n");

    const char * const data_argv[] = {NULL};
    const char * const data_envp[] = {NULL};

    if (fork() == 0) {
      fexecve(data_fd, (char * const *) data_argv, (char * const *) data_envp);
    }
  }
}
