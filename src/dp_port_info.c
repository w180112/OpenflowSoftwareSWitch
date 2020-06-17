#include <stdint.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include "ofpd.h"

int dp_port_init(uint16_t port);

#define RX_RING_SIZE 128
#define TX_RING_SIZE 512

static uint16_t 				nb_rxd = RX_RING_SIZE;
static uint16_t 				nb_txd = TX_RING_SIZE;

static struct rte_eth_conf port_conf_default = {
	.rxmode = { .max_rx_pkt_len = RTE_ETHER_MAX_LEN, }, 
	.txmode = { .offloads = DEV_TX_OFFLOAD_IPV4_CKSUM | 
							DEV_TX_OFFLOAD_UDP_CKSUM | 
							DEV_TX_OFFLOAD_TCP_CKSUM, },
};

int dp_port_init(uint16_t port)
{
	struct rte_eth_conf port_conf = port_conf_default;
	struct rte_eth_dev_info dev_info;
	const uint16_t rx_rings = 1, tx_rings = 4;
	int retval;
	uint16_t q;

	if (!rte_eth_dev_is_valid_port(port))
		return -1;
	rte_eth_dev_info_get(port, &dev_info);
	if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
		port_conf.txmode.offloads |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;

	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;
	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd,&nb_txd);
	if (retval < 0)
		rte_exit(EXIT_FAILURE,"Cannot adjust number of descriptors: err=%d, ""port=%d\n", retval, port);

	/* Allocate and set up 1 RX queue per Ethernet port. */
	for(q=0; q<rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(port,q,nb_rxd,rte_eth_dev_socket_id(port),NULL,mbuf_pool);
		if (retval < 0)
			return retval;
	}

	/* Allocate and set up 4 TX queue per Ethernet port. */
	for(q=0; q<tx_rings; q++) {
		retval = rte_eth_tx_queue_setup(port,q,nb_txd,rte_eth_dev_socket_id(port), NULL);
		if (retval < 0)
			return retval;
	}

	/* Start the Ethernet port. */
	retval = rte_eth_dev_start(port);
	if (retval < 0)
		return retval;
	rte_eth_promiscuous_enable(port);
	return 0;
}
