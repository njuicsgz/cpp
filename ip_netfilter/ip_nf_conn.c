#include "ip_nf.h"

#ifndef CONFIG_IP_NF_TAB_BITS
#define CONFIG_IP_NF_TAB_BITS	12
#endif

#ifndef CONFIG_IP_NF_MAX_CONN
#define CONFIG_IP_NF_MAX_CONN	4096
#endif

/*
 * Connection hash size. Default is what was selected at compile time.
*/
int ip_nf_conn_tab_bits = CONFIG_IP_NF_TAB_BITS;

/* size and mask values */
int ip_nf_conn_tab_size;
int ip_nf_conn_tab_mask;

/* ip_nf_conn hash table:for input and output lookups of ip_nf_conn */
static struct list_head *ip_nf_conn_tab;

/* random value for ip_nf_conn hash */
static unsigned int ip_nf_conn_rnd;

/*  SLAB cache for ip_nf_conn connections */
static struct kmem_cache *ip_nf_conn_cachep __read_mostly;

/* counter for current ip_nf_conn */
static atomic_t ip_nf_conn_count = ATOMIC_INIT(0);

/*
 *  Fine locking granularity for big connection hash table
 */
#define NT_LOCKARRAY_BITS  4
#define NT_LOCKARRAY_SIZE  (1<<NT_LOCKARRAY_BITS)
#define NT_LOCKARRAY_MASK  (NT_LOCKARRAY_SIZE-1)

struct ip_nf_aligned_lock
{
    rwlock_t	lock;
} __attribute__((__aligned__(SMP_CACHE_BYTES)));

/* lock array for ip_nf_conn table */
static struct ip_nf_aligned_lock
        __ip_nf_conntbl_lock_array[NT_LOCKARRAY_SIZE] __cacheline_aligned;

static inline void nt_read_lock(unsigned key)
{
    read_lock(&__ip_nf_conntbl_lock_array[key&NT_LOCKARRAY_MASK].lock);
}

static inline void nt_read_unlock(unsigned key)
{
    read_unlock(&__ip_nf_conntbl_lock_array[key&NT_LOCKARRAY_MASK].lock);
}

static inline void nt_write_lock(unsigned key)
{
    write_lock(&__ip_nf_conntbl_lock_array[key&NT_LOCKARRAY_MASK].lock);
}

static inline void nt_write_unlock(unsigned key)
{
    write_unlock(&__ip_nf_conntbl_lock_array[key&NT_LOCKARRAY_MASK].lock);
}

static inline void nt_read_lock_bh(unsigned key)
{
    read_lock_bh(&__ip_nf_conntbl_lock_array[key&NT_LOCKARRAY_MASK].lock);
}

static inline void nt_read_unlock_bh(unsigned key)
{
    read_unlock_bh(&__ip_nf_conntbl_lock_array[key&NT_LOCKARRAY_MASK].lock);
}

static inline void nt_write_lock_bh(unsigned key)
{
    write_lock_bh(&__ip_nf_conntbl_lock_array[key&NT_LOCKARRAY_MASK].lock);
}

static inline void nt_write_unlock_bh(unsigned key)
{
    write_unlock_bh(&__ip_nf_conntbl_lock_array[key&NT_LOCKARRAY_MASK].lock);
}

/*
 * get_proto_name
 */
static const char* get_proto_name(__u8 proto)
{
    static char buf[20];

    switch (proto)
    {
        case IPPROTO_IP:
            return "IP";
        case IPPROTO_UDP:
            return "UDP";
        case IPPROTO_TCP:
            return "TCP";
        case IPPROTO_ICMP:
            return "ICMP";
        default:
            sprintf(buf, "IP_%d", proto);
            return buf;
    }
}

void ip_nf_conn_put(struct ip_nf_conn *cp)
{
    /* reset it expire in its timeout */
    mod_timer(&cp->timer, jiffies + cp->timeout);

    return;
}

/*Returns hash value for ip_nf_conn entry*/
static inline unsigned
ip_nf_conn_hashkey(__u32 s_addr, __u32 v_addr, __u16 v_port)
{
    return jhash_3words(s_addr, v_addr, (__u32)v_port << 16,
                        ip_nf_conn_rnd ) & ip_nf_conn_tab_mask;
}

static inline int ip_nf_conn_unhash(struct ip_nf_conn *cp)
{
    unsigned hash = ip_nf_conn_hashkey(cp->s_addr, cp->v_addr, cp->v_port);

    nt_write_lock(hash);

    list_del(&cp->list);

    nt_write_unlock(hash);

    return 1;
}

static void ip_nf_conn_expire(unsigned long data)
{
    struct ip_nf_conn *cp = (struct ip_nf_conn *)data;

    if( ip_nf_conn_unhash(cp) )
    {
        atomic_dec(&ip_nf_conn_count);

        kmem_cache_free(ip_nf_conn_cachep, cp);
    }

    return;
}

void ip_nf_conn_expire_now(struct ip_nf_conn *cp)
{
    if (del_timer(&cp->timer))
    {
        mod_timer(&cp->timer, jiffies);
    }
}

/*
 * Hashes ip_nf_conn in ip_nf_conn_tab by s_addr, d_addr, d_port
 * return bool success.
 */
static inline void ip_nf_conn_hash(struct ip_nf_conn *cp, unsigned hash)
{
    nt_write_lock(hash);

    list_add(&cp->list, &ip_nf_conn_tab[hash]);

    nt_write_unlock(hash);

    return;
}

static inline struct ip_nf_conn *
__ip_nf_conn_get(__u32 s_addr, __u32 d_addr, __u32 v_addr,
                 __u16 v_port, __u8 protocol, unsigned hash)
{
    struct ip_nf_conn *cp;

    nt_read_lock(hash);

    list_for_each_entry(cp, &ip_nf_conn_tab[hash], list)
    {
        if( s_addr == cp->s_addr && d_addr == cp->d_addr &&
                v_addr == cp->v_addr && v_port == cp->v_port &&
                protocol == cp->protocol )
        {
            nt_read_unlock(hash);

            return cp;
        }
    }

    nt_read_unlock(hash);

    return NULL;
}

static struct ip_nf_conn*
ip_nf_conn_new(__u32 s_addr, __u32 d_addr, __u32 v_addr,
               __u16 v_port, __u8 protocol, unsigned hash)
{
    struct ip_nf_conn *cp;

    if ( atomic_read(&ip_nf_conn_count) >= CONFIG_IP_NF_MAX_CONN )
    {
        pr_err("[IP_NF] ip_nf_conn has reached max.\n");

        return NULL;
    }

    cp = kmem_cache_zalloc(ip_nf_conn_cachep, GFP_ATOMIC);

    if ( NULL == cp )
    {
        pr_err("[IP_NF] kmem_cache_zalloc return NULL. no memory.\n");

        return NULL;
    }

    INIT_LIST_HEAD(&cp->list);

    setup_timer(&cp->timer, ip_nf_conn_expire, (unsigned long)cp);

    cp->s_addr = s_addr;
    cp->d_addr = d_addr;
    cp->v_addr = v_addr;
    cp->v_port = v_port;
    cp->protocol = protocol;
    cp->bytes = 0;
    cp->pkts  = 0;
    spin_lock_init(&cp->lock);
    cp->timeout = 300 * HZ;

    atomic_inc(&ip_nf_conn_count);

    /* Hash it in the ip_nf_conn_tab finally */
    ip_nf_conn_hash(cp, hash);

    return cp;
}

struct ip_nf_conn*
ip_nf_conn_get(__u32 s_addr, __u32 d_addr, __u32 v_addr,
               __u16 v_port, __u8 protocol)
{
    struct ip_nf_conn *cp;
    unsigned hash = ip_nf_conn_hashkey(s_addr, v_addr, v_port);

    cp = __ip_nf_conn_get(s_addr, d_addr, v_addr, v_port, protocol, hash);

    if( likely(cp) )
    {
        prefetchw(&(cp->pkts));
        prefetchw(&(cp->bytes));
    }
    else
    {
        cp = ip_nf_conn_new(s_addr, d_addr, v_addr, v_port, protocol, hash);
    }

    return cp;
}

static void ip_nf_conn_flush(void)
{
    int idx;
    struct ip_nf_conn *cp;

flush_again:
    for (idx = 0; idx < ip_nf_conn_tab_size; idx++)
    {
        nt_write_lock_bh(idx);

        list_for_each_entry(cp, &ip_nf_conn_tab[idx], list)
        {
            ip_nf_conn_expire_now(cp);
        }

        nt_write_unlock_bh(idx);
    }

    /* the counter may be not NULL, because maybe some conn entries
    are run by slow timer handler or unhashed but still referred */
    if (atomic_read(&ip_nf_conn_count) != 0)
    {
        schedule();

        goto flush_again;
    }

    return;
}

#ifdef CONFIG_PROC_FS

static void *ip_nf_conn_array(struct seq_file *seq, loff_t pos)
{
    int idx;
    struct ip_nf_conn *cp;

    for( idx = 0; idx < ip_nf_conn_tab_size; ++idx)
    {
        nt_read_lock_bh(idx);

        list_for_each_entry(cp, &ip_nf_conn_tab[idx], list)
        {
            if (pos-- == 0)
            {
                seq->private = &ip_nf_conn_tab[idx];

                return cp;
            }
        }

        nt_read_unlock_bh(idx);
    }

    return NULL;
}

static void *ip_nf_conn_seq_start(struct seq_file *seq, loff_t *pos)
{
    seq->private = NULL;
    return *pos ? ip_nf_conn_array(seq, *pos - 1) :SEQ_START_TOKEN;
}

static void *ip_nf_conn_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
    struct ip_nf_conn *cp = v;
    struct list_head *e, *l = seq->private;
    int idx;

    ++*pos;

    if ( v == SEQ_START_TOKEN )
    {
        return ip_nf_conn_array(seq, 0);
    }

    /* more on same hash chain? */
    if ((e = cp->list.next) != l)
    {
        return list_entry(e, struct ip_nf_conn, list);
    }

    idx = l - ip_nf_conn_tab;

    nt_read_unlock_bh(idx);

    while (++idx < ip_nf_conn_tab_size)
    {
        nt_read_lock_bh(idx);

        list_for_each_entry(cp, &ip_nf_conn_tab[idx], list)
        {
            seq->private = &ip_nf_conn_tab[idx];

            return cp;
        }

        nt_read_unlock_bh(idx);
    }

    seq->private = NULL;

    return NULL;
}

static void ip_nf_conn_seq_stop(struct seq_file *seq, void *v)
{
    struct list_head *l = seq->private;

    if (l)
    {
        nt_read_unlock_bh(l - ip_nf_conn_tab);
    }
}

static int ip_nf_conn_seq_show(struct seq_file *seq, void *v)
{
    if ( SEQ_START_TOKEN == v )
    {
        seq_printf(seq, "%-15s %-15s %-15s %-8s %-8s %-10s %-8s %-3s\n",
                   "rsip", "invip", "vipip", "vport", "proto",
                   "bytes", "pkts", "expires");
    }
    else
    {
        const struct ip_nf_conn *cp = v;
        char s_addr[20];
        char d_addr[20];
        char v_addr[20];
        char v_port[20];

        snprintf(s_addr, sizeof(s_addr) -1, "%u.%u.%u.%u", NIPQUAD(cp->s_addr) );
        snprintf(d_addr, sizeof(d_addr) -1, "%u.%u.%u.%u", NIPQUAD(cp->d_addr) );
        snprintf(v_addr, sizeof(v_addr) -1, "%u.%u.%u.%u", NIPQUAD(cp->v_addr) );
        snprintf(v_port, sizeof(v_port) -1, "%u", ntohs(cp->v_port) );

        seq_printf(seq, "%-15s %-15s %-15s %-8s %-8s %-10llu %-8llu %-3lu\n",
                   s_addr, d_addr, v_addr, v_port,
                   get_proto_name(cp->protocol), cp->bytes, cp->pkts,
                   (cp->timer.expires-jiffies)/HZ );
    }

    return 0;
}

static const struct seq_operations ip_nf_conn_seq_ops =
{
    .start = ip_nf_conn_seq_start,
    .next  = ip_nf_conn_seq_next,
    .stop  = ip_nf_conn_seq_stop,
    .show  = ip_nf_conn_seq_show,
};

static int ip_nf_conn_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &ip_nf_conn_seq_ops);
}

static const struct file_operations ip_nf_conn_fops =
{
    .owner	 = THIS_MODULE,
    .open    = ip_nf_conn_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = seq_release,
};

#endif

int ip_nf_conn_init(void)
{
    int idx;

    /* Compute size and mask */
    ip_nf_conn_tab_size = 1 << ip_nf_conn_tab_bits;
    ip_nf_conn_tab_mask = ip_nf_conn_tab_size - 1;

    pr_info("[IP_NF] ip_nf_conn_tab_size=%d ip_nf_max_conn=%d \n",
            ip_nf_conn_tab_size, CONFIG_IP_NF_MAX_CONN);

    /* Allocate the hash table and initialize its list heads*/
    ip_nf_conn_tab = vmalloc(ip_nf_conn_tab_size * sizeof(struct list_head) );

    if( !ip_nf_conn_tab )
    {
        pr_err("[IP_NF] vmalloc ip_nf_conn_tab return NULL.\n");

        return -ENOMEM;
    }

    /* Allocate ip_nf_conn slab cache */
    ip_nf_conn_cachep = kmem_cache_create("ip_nf_conn", sizeof(struct ip_nf_conn),
                                          0, SLAB_HWCACHE_ALIGN, NULL);

    if ( !ip_nf_conn_cachep )
    {
        pr_err("[IP_NF] kmem_cache_create ip_nf_conn_cachep return NULL.\n");

        vfree(ip_nf_conn_tab);

        return -ENOMEM;
    }

    for( idx=0; idx < ip_nf_conn_tab_size; ++idx)
    {
        INIT_LIST_HEAD(&ip_nf_conn_tab[idx]);
    }

    for( idx = 0; idx < NT_LOCKARRAY_SIZE; ++idx)
    {
        rwlock_init(&(__ip_nf_conntbl_lock_array[idx].lock));
    }

    if( !proc_net_fops_create(&init_net, "ip_nf_conn", 0, &ip_nf_conn_fops) )
    {
        kmem_cache_destroy(ip_nf_conn_cachep);

        vfree(ip_nf_conn_tab);

        pr_err("[IP_NF] proc_net_fops_create create /proc/net/ip_nf_conn failed.\n");

        return -ENOMEM;
    }

    /* calculate the random value for netflow hash */
    get_random_bytes(&ip_nf_conn_rnd, sizeof(ip_nf_conn_rnd));

    return 0;
}

void ip_nf_conn_cleanup(void)
{
    /* flush all the connection entries first */
    ip_nf_conn_flush();

    kmem_cache_destroy(ip_nf_conn_cachep);

    proc_net_remove(&init_net, "ip_nf_conn");

    vfree(ip_nf_conn_tab);

    return;
}

