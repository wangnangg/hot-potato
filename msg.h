#pragma once
#include <stdint.h>
#include "potato.h"
enum msg_type
{
    MASTER_HELLO,
    INIT_INFO,
    PLAYER_HELLO,
    PLAYER_READY,
    POTATO,
    MASTER_BYE
};
typedef struct
{
    enum msg_type type;
    int size;
} msg_header;

void send_msg(int fd, const msg_header* msg);
msg_header* recv_msg(int fd);

typedef struct
{
    msg_header header;
    int player_id;
    int num_players;
} msg_master_hello;

msg_master_hello* create_master_hello(int player_id, int num_players);

typedef struct
{
    msg_header header;
    uint32_t next_player_ip;
    uint16_t next_player_port;
} msg_init_info;

msg_init_info* create_init_info(int np_ip, int np_port);

typedef struct
{
    msg_header header;
} msg_master_bye;

msg_master_bye* create_master_bye();

typedef struct
{
    msg_header header;
    int player_id;
    uint16_t listen_port;
} msg_player_hello;

msg_player_hello* create_player_hello(int player_id, uint16_t listen_port);

typedef struct
{
    msg_header header;
    int player_id;
} msg_player_ready;
msg_player_ready* create_player_ready(int pid);

typedef struct
{
    msg_header header;
    potato the_potato;
} msg_potato;
msg_potato* create_msg_potato(int hops);

void msg_loop(int nfd, int fds[], int (*proc_msg)(msg_header* msg));
