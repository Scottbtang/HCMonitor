#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <sys/types.h>
#include <string.h>
#include <sys/queue.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/param.h>
#include <pthread.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_malloc.h>
#include <rte_debug.h>
#include <rte_distributor.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <rte_string_fns.h>
#include <rte_lpm.h>
#include <rte_ring.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_fbk_hash.h>
#include <rte_hash.h>
#include <rte_jhash.h>

#include <pthread.h>
#include "config.h"

#include "http_parse.h"

/* For packet metadata hash and delay buffered*/
#define DEFAULT_HASH_FUNC  rte_jhash

//params for initial hash table
struct rte_hash_parameters ipv4_req_hash_params = {
       .name = NULL,
       .entries = REQ_HASH_ENTRIES,
       //.bucket_entries = REQ_HASH_BUCKET,
       .key_len = sizeof(struct http_tuple),
       .hash_func = DEFAULT_HASH_FUNC,
       .hash_func_init_val = 0,
};


uint8_t http_parse(struct ipv4_hdr *ip_hdr,struct node_data *data,struct timespec ts_now)
{
	struct tcp_hdr  *tcp;
	char ch,sh;
	data->status = -1;

    tcp = (struct tcp_hdr *)((unsigned char *) ip_hdr + sizeof(struct ipv4_hdr));
    unsigned char *p = (unsigned char *) tcp + (tcp->data_off >> 2); 

	ch = *p;	

	switch(ch){
		case 'G':
			data->type = REQ;
			data->status = GET;
			break;
		case 'H':
			sh = *(p + 1);
			switch(sh){
				case 'T':
					sh = *(p + 9);
					switch(sh){
						case '2':
						if((*(p + 10) == '0') && (*(p + 11) == '0')){	
						    data->status = GET;
							data->type = REP;
						}
						break;
					}
				break;
				case 'E':
                      data->status = HEAD;
                      data->type = REQ;
                break;
			}
			break;
		case 'P':
     	      sh = *(p+1);
              data->type = REQ;
              switch(sh){
                  case 'O':data->status = POST;break;
                  case 'U':data->status = PUT;break;
              }
              break;
        case 'D':
              data->status = DELETE;
              data->type = REQ;
              break;
		case 'T':
			data->status = TRACE;
			data->type = REQ;
			break;
	}

	if (likely(data->type == REQ))
    { 
        data->key.ip_src = rte_be_to_cpu_32(ip_hdr->src_addr);
                
        data->key.port_src = rte_be_to_cpu_16(tcp->src_port);

        data->total_len = rte_be_to_cpu_16(ip_hdr->total_length);

        data->sent_seq = rte_be_to_cpu_32(tcp->sent_seq);

        data->ts.tv_sec = ts_now.tv_sec;

        data->ts.tv_nsec = ts_now.tv_nsec;

        return 1;
    }else if(data->type == REP)
    {
        
        data->key.ip_src = rte_be_to_cpu_32(ip_hdr->dst_addr);
            
        data->key.port_src = rte_be_to_cpu_16(tcp->dst_port);

        data->total_len = rte_be_to_cpu_16(ip_hdr->total_length);

        data->sent_seq = rte_be_to_cpu_32(tcp->recv_ack);

        data->ts.tv_sec = ts_now.tv_sec;

        data->ts.tv_nsec = ts_now.tv_nsec;

        return 1;
    }else{
		printf("status:%d\n",data->status);
        return 0;
	}
	
}



