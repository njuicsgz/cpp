#include "ip_nf.h"

__u32 is_running = 0;

static struct proc_dir_entry* ip_nf_root;  // dir of proc_fs

// sysctl params
static struct ctl_table svr_nf_params[] =
{
    {
        .ctl_name     = CTL_UNNUMBERED,
        .procname     = "start",
        .data         = &is_running,
        .maxlen       = sizeof(int),
        .mode         = 0644,
        .proc_handler = &proc_dointvec,
        .strategy     = &sysctl_intvec,
    },
    {}
};

static struct ctl_table svr_nf_sys_ctl_table[] =
{
    {
        .ctl_name  = CTL_UNNUMBERED,
        .procname  = "ip_nf",
        .mode      = 0555,
        .child     = svr_nf_params
    },
    {}
};

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26))
static unsigned int
ip_nf_in(unsigned int hooknum, struct sk_buff *skb,
         const struct net_device *in, const struct net_device *out,
         int (*okfn)(struct sk_buff *))
#else
static unsigned int
ip_nf_in(unsigned int hooknum, struct sk_buff **pskb,
         const struct net_device *in, const struct net_device *out,
         int (*okfn)(struct sk_buff *))
#endif
{
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,26))
    struct sk_buff* skb = *pskb;
#endif

    struct ip_nf_conn *cp;
    struct iphdr *iph = ip_hdr(skb);
    struct iphdr _iph;
    struct iphdr *in_iph;
    struct tcphdr _tcph;
    struct tcphdr *tcph;
    __u16 s_port = 0;
    __u16 d_port = 0;
    __u32 tot_len;

    if( !is_running )
    {
        goto out;
    }

    if( !iph || IPPROTO_IPIP != iph->protocol)
    {
        goto out;
    }

    in_iph = (struct iphdr *)skb_header_pointer(skb, iph->ihl * 4,
             sizeof(_iph), &_iph);

    if( !in_iph )
    {
        goto out;
    }

    if( IPPROTO_TCP == in_iph->protocol )
    {
        tcph = (struct tcphdr *)skb_header_pointer(skb, iph->ihl * 4 +
                in_iph->ihl * 4, sizeof(_tcph), &_tcph);

        if( !tcph )
        {
            goto out;
        }

        s_port = tcph->source;
        d_port = tcph->dest;
    }



    cp = ip_nf_conn_get(iph->saddr, iph->daddr, in_iph->saddr, s_port, in_iph->protocol);

    if (unlikely(!cp))
    {
        goto out;
    }

    tot_len = iph->ihl * 4 + ntohs(in_iph->tot_len);

    spin_lock_bh(&cp->lock);
    cp->bytes += tot_len;
    cp->pkts  += 1;
    spin_unlock_bh(&cp->lock);

    ip_nf_conn_put(cp);

    pr_debug("%u.%u.%u.%u->%u.%u.%u.%u=>%u.%u.%u.%u:%u->%u.%u.%u.%u:%u "
             "bytes:%lld pkts:%lld\n",
             NIPQUAD(iph->saddr), NIPQUAD(iph->daddr),
             NIPQUAD(in_iph->saddr), ntohs(s_port),
             NIPQUAD(in_iph->daddr), ntohs(d_port),
             cp->bytes, cp->pkts);

out:
    return NF_ACCEPT;
}

static struct nf_hook_ops ip_nf_ops[] __read_mostly =
{
    {
        .hook     = ip_nf_in,
        .owner    = THIS_MODULE,
        .pf       = PF_INET,

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26))
        .hooknum  = NF_INET_LOCAL_IN,
#else
        .hooknum  = NF_IP_LOCAL_IN,
#endif

        .priority = 100,
    },
    {
        .hook     = ip_nf_in,
        .owner    = THIS_MODULE,
        .pf       = PF_INET,

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26))
        .hooknum  = NF_INET_LOCAL_OUT,
#else
        .hooknum  = NF_IP_LOCAL_OUT,
#endif

        .priority = 100,
    },
};

#ifdef CONFIG_PROC_FS

static int int_read_proc(char *page, char **start, off_t off,
                         int count, int *eof, void *data)
{
    count = sprintf(page, "%d\n", *(int *)data);

    return count;
}

static int int_write_proc(struct file *file, const char __user *buffer,
                          unsigned long count, void *data)
{
    int * temp = (int *)data;
    char c;

    if( count > 2 )
    {
        return -EINVAL;
    }

    if( __get_user(c, buffer) )
    {
        return -EFAULT;
    }

    if( c != '0' && c != '1')
    {
        return -EINVAL;
    }

    *temp = c - '0';

    return count;
}

#endif

static struct ctl_table_header * ip_nf_ctl_header = NULL;

static __init int ip_nf_init(void)
{
    struct proc_dir_entry * entry;
    int ret;

    ip_nf_ctl_header = register_sysctl_table(svr_nf_sys_ctl_table);

    if( !ip_nf_ctl_header )
    {
        pr_err("[IP_NF] register_sysctl_table failed.\n");

        ret = -ENOMEM;
        goto clean_nothing;
    }

    ip_nf_root = proc_mkdir("ip_nf", NULL);

    if( !ip_nf_root)
    {
        pr_err("[IP_NF] proc_mkdir /proc/ip_nf failed.\n");

        ret = -ENOMEM;

        goto clean_sys_ctl;
    }

    entry = create_proc_entry("start", 0644, ip_nf_root);

    if( !entry )
    {
        ret = -ENOMEM;

        pr_err("[IP_NF] create_proc_entry /proc/ip_nf/start failed.\n");

        goto clean_proc_root;
    }

    entry->data       = &is_running;
    entry->read_proc  = &int_read_proc;
    entry->write_proc = &int_write_proc;

    ret = ip_nf_conn_init();

    if ( ret < 0 )
    {
        pr_err("[IP_NF] can't setup ip_nf_conn table.\n");

        goto clean_proc_entry;
    }

    ret = nf_register_hooks(ip_nf_ops, ARRAY_SIZE(ip_nf_ops));

    if( ret < 0 )
    {
        pr_err("[IP_NF] can't register netfilter hooks.\n");

        goto clean_nf_conn;
    }

    pr_debug("[IP_NF] register ip_nf.ko module successful.\n");

    goto clean_nothing;

clean_nf_conn:
    ip_nf_conn_cleanup();

clean_proc_entry:
    remove_proc_entry("start", ip_nf_root);

clean_proc_root:
    remove_proc_entry("ip_nf", NULL);

clean_sys_ctl:
    unregister_sysctl_table(ip_nf_ctl_header);

clean_nothing:
    return ret;
}

static __exit void ip_nf_cleanup(void)
{
    nf_unregister_hooks(ip_nf_ops, ARRAY_SIZE(ip_nf_ops));

    ip_nf_conn_cleanup();

    remove_proc_entry("start", ip_nf_root);
    remove_proc_entry("ip_nf", NULL);
    unregister_sysctl_table(ip_nf_ctl_header);

    pr_debug("[IP_NF] undo register ip_nf module successful.\n");
}

module_init(ip_nf_init);
module_exit(ip_nf_cleanup);

MODULE_LICENSE("GPL");

