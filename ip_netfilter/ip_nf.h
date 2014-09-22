#ifndef _NET_IP_NET_FLOW_H
#define _NET_IP_NET_FLOW_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/list.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>
#include <linux/jhash.h>
#include <linux/prefetch.h>
#include <linux/version.h>
#include <linux/sysctl.h>
#include <linux/random.h>

struct ip_nf_conn
{
    struct list_head list;

    __u32 s_addr;               /*real server ip */
    __u32 d_addr;               /*load balance ip */
    __u32 v_addr;               /*virtual ip */
    __u16 v_port;               /*virtual ip's port */
    __u8  protocol;

    unsigned long long bytes;
    unsigned long long pkts;

    spinlock_t lock;            /* lock for state transition */

    volatile unsigned long	timeout;	/* timeout */
    struct timer_list	timer;		    /* Expiration timer */
};

extern struct ip_nf_conn*
ip_nf_conn_get(__u32 s_addr, __u32 d_addr,
               __u32 v_addr, __u16 v_port, __u8 protocol);

extern int  ip_nf_conn_init(void);
extern void ip_nf_conn_cleanup(void);
extern void ip_nf_conn_put(struct ip_nf_conn *cp);

#endif

