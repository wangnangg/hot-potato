#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "msg.h"

void print_help_and_exit()
{
    printf("player <machine_name> <port_num>\n");
    exit(EXIT_FAILURE);
}

int parse_int(const char* buffer, int min, int max)
{
    errno = 0;
    long int val = strtol(buffer, NULL, 10);
    if (errno != 0)
    {
        perror(NULL);
        print_help_and_exit();
    }
    if (val < min || val > max)
    {
        printf("parameter(s) out of range.\n");
        print_help_and_exit();
    }
    return val;
}

int listen_on_any(int* port_num, int wait_queue)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    if (listen(sockfd, wait_queue) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    socklen_t addr_size = sizeof(struct sockaddr_in);
    struct sockaddr_in addr;
    getsockname(sockfd, (struct sockaddr *)&addr, &addr_size);
    *port_num = ntohs(addr.sin_port);
    return sockfd;
}

int connect_master(const char* master_name, int port_num)
{
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo* host_info_list;
    char port_buff[10];
    sprintf(port_buff, "%d", port_num);
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family = AF_INET;
    host_info.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(master_name, port_buff, &host_info, &host_info_list) != 0)
    {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                       host_info_list->ai_protocol);
    if (socket_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    if (connect(socket_fd, host_info_list->ai_addr,
                host_info_list->ai_addrlen) < 0)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    return socket_fd;
}

void* get_prev_fd(void* plisten_fd)
{
    int listen_fd = *(int*)plisten_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int fd;
    if ((fd = accept(listen_fd, (struct sockaddr*)&address,
                     (socklen_t*)&addrlen)) < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

#ifndef NDEBUG
    printf("exiting passive_com\n");
#endif
    int* fd_store = (int*)malloc(sizeof(int));
    *fd_store = fd;
    return fd_store;
}

typedef struct
{
    uint32_t ip;
    uint16_t port;
} player_addr;

void* get_next_fd(void* addr)
{
    player_addr *paddr = (player_addr*) addr;
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(paddr->port);
    serv_addr.sin_addr.s_addr = htonl(paddr->ip);

    if (connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

#ifndef NDEBUG
    printf("exiting active_com\n");
#endif
    int* fd_store = (int*)malloc(sizeof(int));
    *fd_store = fd;
    return fd_store;
}

int randint(int N)
{
    return rand() % N;
}
int my_id;
int master_fd;
int next_fd;
int prev_fd;
int num_players;
int proc_msg(msg_header* msg)
{
    switch(msg->type)
    {
    case MASTER_BYE:
        return 1;
    case POTATO:
    {
        msg_potato* pt_msg = (msg_potato*)msg;
        potato* pt = &pt_msg->the_potato;
        pt->trace[pt->trace_size - pt->remain_hops] = my_id;
        pt->remain_hops -= 1;
        if(pt_msg->the_potato.remain_hops == 0)
        {
            printf("I'm it\n");
            send_msg(master_fd, msg);
        } else{
            int r = randint(2);
            if(r == 0)
            {
                printf("Sending potato to %d\n", (my_id -1 + num_players) % num_players);
                send_msg(prev_fd, msg);
            } else {
                printf("Sending potato to %d\n", (my_id +1) % num_players);
                send_msg(next_fd, msg);
            }
        }
        return 0;
    }
    default:
        printf("Unexpected msg type %d\n", msg->type);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, const char* argv[])
{
    if (argc < 3)
    {
        print_help_and_exit();
    }
    const char* master_name = argv[1];
    int port_num = parse_int(argv[2], 1, 65535);
    int listen_port;
    int listen_fd = listen_on_any(&listen_port, 1);

    master_fd = connect_master(master_name, port_num);
    msg_header* msg = (msg_header*)create_player_hello(listen_port);
    send_msg(master_fd, msg);
    free(msg);
    msg_master_hello* phello_msg = (msg_master_hello*)recv_msg(master_fd);
    my_id = phello_msg->player_id;
    num_players = phello_msg->num_players;
    player_addr next_player_addr;
    next_player_addr.ip = phello_msg->next_player_ip;
    next_player_addr.port = phello_msg->next_player_port;
    free(phello_msg);
    printf("Connected as player %d out of %d total players\n", my_id,
           num_players);
    srand((unsigned int)time(NULL) + my_id);

    pthread_t prev_fd_thread;
    pthread_t next_fd_thread;
    pthread_create(&prev_fd_thread, NULL, get_prev_fd, &listen_fd);
    pthread_create(&next_fd_thread, NULL, get_next_fd, &next_player_addr);

    int* fd_store;
    pthread_join(prev_fd_thread, (void**)&fd_store);
    prev_fd = *fd_store;
    free(fd_store);

    pthread_join(next_fd_thread, (void**)&fd_store);
    next_fd = *fd_store;
    free(fd_store);

    close(listen_fd);

    msg_header *ready_msg =  (msg_header *)create_player_ready();
    send_msg(master_fd, ready_msg);
    free(ready_msg);

    int fds[3] = {master_fd, prev_fd, next_fd};

    msg_loop(3, fds, proc_msg);
    close(master_fd);
    close(prev_fd);
    close(next_fd);
}