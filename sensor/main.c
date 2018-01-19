/*
 * Copyright (C) 2018 Hamburg University of Applied Sciences
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     demo
 * @{
 *
 * @file
 * @brief       IoThon 2018
 *
 * @author      Peter Kietzmann <peter.kietzmann@haw-hamburg.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "xtimer.h"
#include "thread.h"
#include "shell.h"
#include "shell_commands.h"
#include "board.h"

#include "tlsf-malloc.h"
#include "ccnl-pkt-ndntlv.h"
#include "ccn-lite-riot.h"
#include "ccnl-pkt-builder.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/netapi.h"
#include "net/gnrc.h"
#include "net/gnrc/pktdump.h"

#include "saul_reg.h"
#include "periph/gpio.h"
#include "dht.h"

/* main thread's message queue */
#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

/* 10kB buffer for the heap should be enough for everyone */
#define TLSF_BUFFER     (10240 / sizeof(uint32_t))
static uint32_t _tlsf_heap[TLSF_BUFFER];

#define CREATE_CONTENT_DELAY (5)

char stack[THREAD_STACKSIZE_MAIN];

saul_reg_t *saul_temp;
dht_t dev_out;
int16_t temp_out;
int16_t hum_out;
dht_t dev;
#define ACT_SW_PIN GPIO_PIN(1,13)

#ifndef PREFIX
#define PREFIX              "p"
#endif
#ifndef NODE_NAME
#define NODE_NAME           "R2"
#endif
#ifndef SENSOR_TYPE_TEMP
#define SENSOR_TYPE_TEMP    "hum"
#endif
#ifndef ACTOR_TYPE_SWITCH
#define ACTOR_TYPE_SWITCH   "sw"
#endif
#ifndef CTRL_PREFIX
#define CTRL_PREFIX         "ctl"
#endif

#define SWITCH_CMD_ON       "on"
#define SWITCH_CMD_OFF      "off"

#if PUBLISHER
static unsigned char _out[CCNL_MAX_PACKET_SIZE];
static bool _pub(struct ccnl_relay_s *ccnl)
{
    //puts("NEW CONT");
    char name[32], name2[32];
    int suite = CCNL_SUITE_NDNTLV;
    int offs = CCNL_MAX_PACKET_SIZE;

    phydat_t r;
    saul_reg_read(saul_temp, &r);
    int len = 5;
    char buffer[len];

    dht_read(&dev,&temp_out,&hum_out);
    
    snprintf(buffer, len, "%d", hum_out);

    printf("temperature: %d \n",temp_out);
    printf("humidity: %d \n",hum_out);
   
    unsigned time = (unsigned)xtimer_now_usec();
    int name_len = sprintf(name, "/%s/%s/%s/%u", PREFIX, NODE_NAME, SENSOR_TYPE_TEMP, time);
    name[name_len]='\0';
    memcpy(name2, name, name_len);
    name2[name_len]='\0';


    struct ccnl_prefix_s *prefix = ccnl_URItoPrefix(name, suite, NULL, NULL);
    int arg_len = ccnl_ndntlv_prependContent(prefix, (unsigned char*) buffer,
        len, NULL, NULL, &offs, _out);
/*
    char sensor_reading[64];
    int16_t reading = time;
    int sensor_reading_len = sprintf(sensor_reading, "%i", reading);
    sensor_reading[sensor_reading_len]='\0';
    struct ccnl_prefix_s *prefix = ccnl_URItoPrefix(name, suite, NULL, NULL);
    int arg_len = ccnl_ndntlv_prependContent(prefix, (unsigned char*) sensor_reading,
        sensor_reading_len, NULL, NULL, &offs, _out);
*/
    ccnl_prefix_free(prefix);

    unsigned char *olddata;
    unsigned char *data = olddata = _out + offs;

    unsigned typ;

    if (ccnl_ndntlv_dehead(&data, &arg_len, (int*) &typ, &len) || typ != NDN_TLV_Data) {
        puts("ERROR in _pub");
        return false;
    }

    struct ccnl_content_s *c = 0;
    struct ccnl_pkt_s *pk = ccnl_ndntlv_bytes2pkt(typ, olddata, &data, &arg_len);
    c = ccnl_content_new(&pk);
    ccnl_content_add2cache(ccnl, c);
    c->flags |= CCNL_CONTENT_FLAGS_STALE;

    return true;
}


void *publisher(void *arg)
{
    while (1) {
        struct ccnl_relay_s *ccnl = (struct ccnl_relay_s *) arg;
        _pub(ccnl);
        xtimer_sleep(CREATE_CONTENT_DELAY);
    }
    return NULL;
}

int producer_func(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
                   struct ccnl_pkt_s *pkt){
    (void)from;
/*
    printf("Local producer incoming interest=<%s>%s from=%s\n",
                  (ccnl_prefix_to_path((pkt)->pfx)),
                  ccnl_suite2str((pkt)->suite),
                  ccnl_addr2ascii(from ? &from->peer : NULL));
    printf("Check for this sequence: %.*s\n", pkt->pfx->complen[1], pkt->pfx->comp[1]);
    printf("comcnt is %i\n", pkt->pfx->compcnt);
*/
    if(pkt->pfx->compcnt > 3) { /* /PREFIX/NODE_NAME/ACTOR_TYPE/ctrl/on */
        if (!memcmp(pkt->pfx->comp[3], CTRL_PREFIX, pkt->pfx->complen[3]) ) {
            if (!memcmp(pkt->pfx->comp[0], PREFIX, pkt->pfx->complen[0]) &&
                !memcmp(pkt->pfx->comp[1], NODE_NAME, pkt->pfx->complen[1])) {
                if(!memcmp(pkt->pfx->comp[2], ACTOR_TYPE_SWITCH, pkt->pfx->complen[2])) {
                    //printf("switch command is: %.*s\n", pkt->pfx->complen[4], pkt->pfx->comp[4]);
                    if(!memcmp(pkt->pfx->comp[4], SWITCH_CMD_ON, pkt->pfx->complen[4])) {
                        gpio_set(ACT_SW_PIN);
                    }
                    else if(!memcmp(pkt->pfx->comp[4], SWITCH_CMD_OFF, pkt->pfx->complen[4])) {
                        gpio_clear(ACT_SW_PIN);
                    }
                    else {
                        puts("can not process actor command");
                    }
                    int len = 4;
                    char buffer[len];
                    snprintf(buffer, len, "ACK");
                    unsigned char *b = (unsigned char *)buffer;
                    struct ccnl_content_s *c = ccnl_mkContentObject(pkt->pfx, b, len);
                    c->last_used -= CCNL_CONTENT_TIMEOUT + 5;
                    if (c) {
                        ccnl_content_add2cache(relay, c);
                    }
                }
            }
        }
    }
    return 0;
}
#endif

int main(void)
{

	dev.pin = GPIO_PIN(PORT_A,9);
	dev.type = DHT11;
	dev.in_mode = GPIO_IN;

	if(dht_init(&dev_out,&dev) == 0){
    	printf("Init successfully \n");
    }

    gpio_init(ACT_SW_PIN,GPIO_OUT);


    tlsf_create_with_pool(_tlsf_heap, sizeof(_tlsf_heap));
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    puts("IoThon Demo");

    ccnl_core_init();

    ccnl_start();


#ifndef CONSUMER
    ccnl_set_local_producer(producer_func);
    saul_temp = saul_reg_find_nth(9);
#endif
    /* get the default interface */
    gnrc_netif_t *netif;

    /* set the relay's PID, configure the interface to use CCN nettype */
    if (((netif = gnrc_netif_iter(NULL)) == NULL) ||
        (ccnl_open_netif(netif->pid, GNRC_NETTYPE_CCN) < 0)) {
        puts("Error registering at network interface!");
        return -1;
    }
    uint16_t src_len = 8U;
    gnrc_netapi_set(netif->pid, NETOPT_SRC_LEN, 0, &src_len, sizeof(src_len));

#ifdef MODULE_NETIF
    gnrc_netreg_entry_t dump = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL,
                                                          gnrc_pktdump_pid);
    gnrc_netreg_register(GNRC_NETTYPE_CCN_CHUNK, &dump);
#endif
#ifdef PUBLISHER
    thread_create(stack, sizeof(stack), THREAD_PRIORITY_MAIN - 1,
              THREAD_CREATE_STACKTEST, publisher, &ccnl_relay, "pub");

#endif
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);
    return 0;
}
