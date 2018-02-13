#include "msg.h"
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "errno.h"

void send_msg(int fd, const msg_header* msg)
{
    int count = 0;
    while (count < msg->size)
    {
        int num_write = write(fd, (const char*)msg + count, msg->size - count);
        if (num_write == -1)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }
        count += num_write;
    }
#ifndef NDEBUG
    printf("sent msg %d of length %d\n", msg->type, msg->size);
#endif
}
msg_header* recv_msg(int fd)
{
    int count = 0;
    int header_size = (int)sizeof(msg_header);
    int msg_size = header_size;
    msg_header* msg = malloc(header_size);
    while (count < msg_size)
    {
        int num_read = read(fd, (char*)msg + count, msg_size - count);
        if (num_read == 0)
        {
            // remote is closed
            free(msg);
            msg = NULL;
            break;
        }
        if (num_read == -1)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }
        count += num_read;
        if (count == header_size)
        {
            msg_size = msg->size;
            msg = realloc(msg, msg_size);
        }
    }
#ifndef NDEBUG
    if (msg)
    {
        printf("received msg %d of length %d\n", msg->type, msg->size);
    }
    else
    {
        printf("received EOF.\n");
    }
#endif
    return msg;
}

msg_master_hello* create_master_hello(int player_id, int num_players)
{
    msg_master_hello* msg = (msg_master_hello*)malloc(sizeof(msg_master_hello));
    msg->header.type = MASTER_HELLO;
    msg->header.size = sizeof(msg_master_hello);
    msg->player_id = player_id;
    msg->num_players = num_players;
    return msg;
}

msg_master_bye* create_master_bye()
{
    msg_master_bye* msg = (msg_master_bye*)malloc(sizeof(msg_master_bye));
    msg->header.type = MASTER_BYE;
    msg->header.size = sizeof(msg_master_bye);
    return msg;
}

msg_player_hello* create_player_hello(int player_id, uint16_t listen_port)
{
    msg_player_hello* msg = (msg_player_hello*)malloc(sizeof(msg_player_hello));
    msg->header.type = PLAYER_HELLO;
    msg->header.size = sizeof(msg_player_hello);
    msg->listen_port = listen_port;
    msg->player_id = player_id;
    return msg;
}

msg_player_ready* create_player_ready(int player_id)
{
    msg_player_ready* msg = (msg_player_ready*)malloc(sizeof(msg_player_ready));
    msg->header.type = PLAYER_READY;
    msg->header.size = sizeof(msg_player_ready);
    msg->player_id = player_id;
    return msg;
}

msg_potato* create_msg_potato(int hops)
{
    size_t msg_size = offsetof(msg_potato, the_potato) + potato_size(hops);
    msg_potato* msg = (msg_potato*)malloc(msg_size);
    memset(msg, 0, msg_size);
    msg->header.type = POTATO;
    msg->header.size = msg_size;
    msg->the_potato.remain_hops = hops;
    msg->the_potato.trace_size = hops;
    return msg;
}

msg_init_info* create_init_info(int np_ip, int np_port)
{
    msg_init_info* msg = (msg_init_info*)malloc(sizeof(msg_init_info));
    msg->header.type = INIT_INFO;
    msg->header.size = sizeof(msg_init_info);
    msg->next_player_ip = np_ip;
    msg->next_player_port = np_port;
    return msg;
}

void msg_loop(int nfd, int fds[], int (*proc_msg)(msg_header* msg))
{
    struct pollfd* pollfds = malloc(sizeof(struct pollfd) * nfd);
    for (int i = 0; i < nfd; i++)
    {
        pollfds[i].fd = fds[i];
        pollfds[i].events = POLLIN;
    }

    msg_header* new_msg = NULL;
    do
    {
        if (poll(pollfds, nfd, -1) < 0)
        {
            perror("poll");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < nfd; i++)
        {
            if (pollfds[i].revents & POLLIN)
            {
                new_msg = recv_msg(pollfds[i].fd);
                if (new_msg)
                {
                    int stop = proc_msg(new_msg);
                    free(new_msg);
                    if (stop)
                    {
                        goto out;
                    }
                }
            }
        }

    } while (1);
out:
    free(pollfds);
}
