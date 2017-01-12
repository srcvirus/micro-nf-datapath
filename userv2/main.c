#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include <rte_common.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_log.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_ring.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_string_fns.h>

#include "common.h"

#define PKT_READ_SIZE  ((uint16_t)32)

// userv_id tells us which rx ring read and which ring to write to
static uint8_t userv_id = 0;
struct rte_ring *u1_ring, *u2_ring;

struct mbuf_queue{
#define MBQ_CAPACITY 32
	struct rte_mbuf *bufs[MBQ_CAPACITY];
	uint16_t top;
};

/* buffers up a set of packet that are ready to send */
static struct mbuf_queue output_bufs;

static void
usage(const char *progname)
{
    printf("Usage: %s [EAL args] -- -n <client_id>\n\n", progname);
}

static int
parse_userv_num(const char* n){
	char *end = NULL;
    unsigned long temp;

    if (n == NULL || *n == '\0')
        return -1;

    temp = strtoul(n, &end, 10);
    if (end == NULL || *end != '\0')
        return -1;

    userv_id = (uint8_t)temp;
    return 0;

}

static int 
parse_app_args(int argc, char *argv[])
{
    int option_index, opt;
    char **argvopt = argv;
    const char *progname = NULL;
    static struct option lgopts[] = { /* no long options */
        {NULL, 0, 0, 0 }
    };
    progname = argv[0];

    while ((opt = getopt_long(argc, argvopt, "n:", lgopts,
        &option_index)) != EOF){
        switch (opt){
            case 'n':
                if (parse_userv_num(optarg) != 0){
                    usage(progname);
                    return -1;
                }
                break;
            default:
                usage(progname);
                return -1;
        }
    }
    return 0;
}

/*
 * Pass the packets to next usevice via ring
 */
static void
pass_to_ring(struct mbuf_queue *mbq)
{
	struct rte_mbuf **pkts = mbq->bufs;
	uint16_t rx_count = mbq->top;
	uint16_t i, j;

	for(i=0; i<rx_count; i++){
		if(rte_ring_enqueue_bulk(u2_ring, (void **) pkts, rx_count) != 0){
			for(j=0; j<rx_count; j++)
				rte_pktmbuf_free(pkts[j]);    // dropping
			//stat the drop
		}
		//stat the sent
	}
	mbq->top = 0; 
}

static inline void 
enqueue_packet(struct rte_mbuf *buf)
{
	struct mbuf_queue *mbq = &output_bufs;
	mbq->bufs[mbq->top++] = buf;
	
	// pass them to next ring when the stack is full
	if(mbq->top == MBQ_CAPACITY){
		pass_to_ring(mbq);
	}
}

/*
 * Function that get packets from one ring and queue it up for processing
 */
static void 
handle_packet(struct rte_mbuf* buf)
{

	//TODO do packet manipulating
	// userv2 is either l3header parser or directly classifier

	//add to ouputbufs queue (may not be sent yet)
	enqueue_packet(buf);
}

int main(int argc, char *argv[])
{
	const struct rte_memzone *mz;
	struct rte_mempool* mp;
	struct port_info *ports;
	int retval;
    void *pkts[PKT_READ_SIZE];
	int need_flush = 0;	

    if ((retval = rte_eal_init(argc, argv)) < 0)
        return -1;
    argc -= retval;
    argv += retval;

	if (parse_app_args(argc, argv) < 0)
        rte_exit(EXIT_FAILURE, "Invalid command-line arguments\n");

	u1_ring = rte_ring_lookup(get_ring_queue_name(userv_id-1));
    if (u1_ring == NULL)
        rte_exit(EXIT_FAILURE, "Cannot get RX ring - is server process running?\n");

	u2_ring = rte_ring_lookup(get_ring_queue_name(userv_id));
    if (u2_ring == NULL)
        rte_exit(EXIT_FAILURE, "Cannot get RX ring - is server process running?\n");
    
	mp = rte_mempool_lookup(PKTMBUF_POOL_NAME);
    if (mp == NULL)
        rte_exit(EXIT_FAILURE, "Cannot get mempool for mbufs\n");

	mz = rte_memzone_lookup(MZ_PORT_INFO);
    if (mz == NULL)
        rte_exit(EXIT_FAILURE, "Cannot get port info structure\n");
    ports = mz->addr;

	//packet_in(ports);
	for(;;){
		uint16_t i, rx_pkts = PKT_READ_SIZE;
        uint8_t port;

        /* try dequeuing max possible packets first, if that fails, get the
         * most we can. Loop body should only execute once, maximum */
        while (rx_pkts > 0 &&
                unlikely(rte_ring_dequeue_bulk(u1_ring, pkts, rx_pkts) != 0))
            rx_pkts = (uint16_t)RTE_MIN(rte_ring_count(u1_ring), PKT_READ_SIZE);

        if (unlikely(rx_pkts == 0)){
		    if (need_flush)
                for (port = 0; port < ports->num_ports; port++)
                    pass_to_ring(&output_bufs);
            need_flush = 0;
            continue;
        }

        for (i = 0; i < rx_pkts; i++)
            handle_packet(pkts[i]);

        need_flush = 1;

	}

	return 0;
}
