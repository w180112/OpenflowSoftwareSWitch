#include <stdio.h>
#include <stdint.h>
#include <common.h>
#include "ofp_flow.h"

uint8_t zero[ETH_ALEN] = { 0 };

void print_in_port(table_in_port_t in_port)
{
    printf("in_port=");
    for(int i=0; i<MAX_IN_PORT&&in_port.in_port[i]>0; i++)
        printf("%u, ", in_port.in_port[i]);
}

void print_mac(table_mac_t mac)
{
    if (BYTES_CMP(mac.dst_mac, zero, ETH_ALEN) == FALSE)
        printf("dst_mac=%x:%x:%x:%x:%x:%x, ", mac.dst_mac[0], mac.dst_mac[1], mac.dst_mac[2], mac.dst_mac[3], mac.dst_mac[4], mac.dst_mac[5]);
    if (BYTES_CMP(mac.src_mac, zero, ETH_ALEN) == FALSE)
        printf("src_mac=%x:%x:%x:%x:%x:%x, ", mac.src_mac[0], mac.src_mac[1], mac.src_mac[2], mac.src_mac[3], mac.src_mac[4], mac.src_mac[5]);
    if (mac.eth_type > 0)
        printf("eth_type=%u, ", mac.eth_type);
}

void print_ip(table_ip_t ip)
{
    if (ip.dst_ip_addr > 0) {
        printf("dst_ip=%u ", ip.dst_ip_addr);
        if (ip.dst_mask_len < 32)
            printf("/%u", ip.dst_mask_len);
        printf(", ");
    }
    if (ip.src_ip_addr > 0) {
        printf("src_ip=%u ", ip.src_ip_addr);
        if (ip.src_mask_len < 32)
            printf("/%u", ip.src_mask_len);
        printf(", ");
    }
    if (ip.ip_proto > 0)
        printf("ip_proto=%u, ", ip.ip_proto);
}

void print_port(table_port_t port)
{
    if (port.src_ip_port > 0)
        printf("src_port=%u, ", port.src_ip_port);
    if (port.dst_ip_port > 0)
        printf("dst_port=%u, ", port.dst_ip_port);
}

void print_action(action_info_t action)
{
    if (BYTES_CMP(action.dst_mac, zero, sizeof(ETH_ALEN)) == FALSE)
        printf("dst_mac=%x:%x:%x:%x:%x:%x, ", action.dst_mac[0], action.dst_mac[1], action.dst_mac[2], action.dst_mac[3], action.dst_mac[4], action.dst_mac[5]);
    if (BYTES_CMP(action.src_mac, zero, ETH_ALEN) == FALSE)
        printf("src_mac=%x:%x:%x:%x:%x:%x, ", action.src_mac[0], action.src_mac[1], action.src_mac[2], action.src_mac[3], action.src_mac[4], action.src_mac[5]);
    if (action.ether_type > 0)
        printf("eth_type=%u, ", action.ether_type);
    if (action.ip_dst > 0)
        printf("dst_ip=%u %u %u %u, ", (action.ip_dst >> 24) & 0xFF, (action.ip_dst >> 16) & 0xFF, (action.ip_dst >> 8) & 0xFF, action.ip_dst & 0xFF);
    if (action.ip_src > 0)
        printf("src_ip=%u %u %u %u, ", (action.ip_src >> 24) & 0xFF, (action.ip_src >> 16) & 0xFF, (action.ip_src >> 8) & 0xFF, action.ip_src & 0xFF);
    if (action.ip_proto > 0)
        printf("ip_proto=%u, ", action.ip_proto);
    if (action.dst_port > 0)
        printf("dst_port=%u, ", action.dst_port);
    if (action.out_port[0].port > 0)
        printf("out_port=");
    for(int i=0; i<MAX_OUT_PORT && action.out_port[i].port>0; i++)
        printf("%u, ", action.out_port[i].port);
}