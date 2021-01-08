#pragma once
#ifndef COMMON_H
#define COMMON_H

#define MQ_DATA_TO_DEAMON "/mq_data_to_deamon"
#define MQ_DATA_FROM_DEAMON "/mq_data_from_deamon"
#define MQ_MODE         0666
#define MQ_MSG_SZ       100
#define MQ_MAX_MSG      50

#define MQ_TYPE_MODE    0
#define MQ_TYPE_FREQ    1
#define MQ_TYPE_EXIT    2

#define MODE_AUTO 1
#define MODE_MANUAL 0       

const char* modes[] = {"Manual", "Automatic"};

/* mqueue msg structure */
struct msg_to_deamon {
    int type;
    int val;
};

struct data_from_deamon {
    int mode;
    int freq;
    float temp;
};

/* init mq attributs */
static struct mq_attr to_deamon_attr = {
    .mq_flags = 0,
	.mq_maxmsg = MQ_MAX_MSG,
	.mq_msgsize = sizeof(struct msg_to_deamon),
};

static struct mq_attr from_deamon_attr = {
    .mq_flags = 0,
	.mq_maxmsg = MQ_MAX_MSG,
	.mq_msgsize = sizeof(struct data_from_deamon),
};

#endif
