#include <errno.h>
#include <limits.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include "msg.h"

void print_help_and_exit()
{
    printf("ringmaster <port_num> <num_players> <num_hops>\n");
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

typedef struct
{
    int id;
    int fd;
    uint32_t ip;
    uint16_t port;
} player;

int listen_on(int port_num, int wait_queue)
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
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_num);
    if (bind(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(sockfd, wait_queue) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void wait_for_one(int sockfd, player* p)
{
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int fd;
    if ((fd = accept(sockfd, (struct sockaddr*)&address,
                     (socklen_t*)&addrlen)) < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    p->fd = fd;
    p->ip = ntohl(address.sin_addr.s_addr);
}

int randint(int N) { return rand() % N; }

int proc_msg(msg_header* msg)
{
    switch (msg->type)
    {
        case POTATO:
            printf("Trace of potato:");
            msg_potato* pt_msg = (msg_potato*)msg;
            for (int i = 0; i < pt_msg->the_potato.trace_size; i++)
            {
                printf("%d", pt_msg->the_potato.trace[i]);
                if (i == pt_msg->the_potato.trace_size - 1)
                {
                    printf("\n");
                }
                else
                {
                    printf(",");
                }
            }
            return 1;
        default:
            printf("Unexpected msg type %d\n", msg->type);
            exit(EXIT_FAILURE);
    }
}

int main(int argc, const char* argv[])
{
    if (argc < 4)
    {
        print_help_and_exit();
    }
    int port_num = parse_int(argv[1], 1, 65535);
    int num_players = parse_int(argv[2], 1, INT_MAX);
    int num_hops = parse_int(argv[3], 0, 512);
    printf("Potato Ringmaster\n");
    printf("Players = %d\n", num_players);
    printf("Hops = %d\n", num_hops);

    player* player_list;
    player_list = malloc(sizeof(player) * num_players);
    int sockfd = listen_on(port_num, num_players);
    for (int i = 0; i < num_players; i++)
    {
        player_list[i].id = i;
        wait_for_one(sockfd, &player_list[i]);
        msg_player_hello* msg = (msg_player_hello*)recv_msg(player_list[i].fd);
        player_list[i].port = msg->listen_port;
        free(msg);
#ifndef NDEBUG
        printf("Player %d is connected, listening on port %d\n", i,
               player_list[i].port);
#endif
    }
    close(sockfd);
    for (int i = 0; i < num_players; i++)
    {
        player* np = &player_list[(i + 1) % num_players];
        msg_header* msg = (msg_header*)create_master_hello(
            player_list[i].id, num_players, np->ip, np->port);
        send_msg(player_list[i].fd, msg);
        free(msg);
    }
    for (int i = 0; i < num_players; i++)
    {
        msg_header* msg = recv_msg(player_list[i].fd);
        free(msg);
        printf("Player %d is ready to play\n", i);
    }

    if (num_hops > 0)
    {
        srand((unsigned int)time(NULL) + num_players);
        int first_id = randint(num_players);
        printf("Ready to start the game, sending potato to player %d\n",
               first_id);
        msg_header* pt_msg = (msg_header*)create_msg_potato(num_hops);
        send_msg(player_list[first_id].fd, pt_msg);
        free(pt_msg);

        int* fds = (int*)malloc(sizeof(int) * num_players);
        for (int i = 0; i < num_players; i++)
        {
            fds[i] = player_list[i].fd;
        }
        msg_loop(num_players, fds, proc_msg);
    }

    for (int i = 0; i < num_players; i++)
    {
        msg_header* msg = (msg_header*)create_master_bye();
        send_msg(player_list[i].fd, msg);
        free(msg);
        close(player_list[i].fd);
    }
}
