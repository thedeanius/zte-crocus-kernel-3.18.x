#include <linux/init.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/types.h>
#include <net/sock.h>
#include <net/netlink.h>

#include "gf3238_spi.h"

#define NETLINK_TEST_MILAN 25
#define MAX_MSGSIZE_MILAN 32
int stringlength_milan(char *s);
void sendnlmsg_milan(char *message);
int pid_milan;
int err_milan;
struct sock *nl_sk_milan;
int flag_milan;

struct gf_uk_channel {
	int channel_id;
	int reserved;
	char buf[3 * 1024];
	int len;
};

void sendnlmsg_milan(char *message)
{
	struct sk_buff *skb_1;
	struct nlmsghdr *nlh;
	int len = NLMSG_SPACE(MAX_MSGSIZE_MILAN);
	int slen = 0;
	int ret = 0;

	if (!message || !nl_sk_milan || !pid_milan) {
		return;
	}
	skb_1 = alloc_skb(len, GFP_KERNEL);
	/*
	*if (!skb_1) {
	*	pr_err("my_net_link:alloc_skb_1 error\n");
	*}
	*/
	slen = strlen(message);
	nlh = nlmsg_put(skb_1, 0, 0, 0, MAX_MSGSIZE_MILAN, 0);

	NETLINK_CB(skb_1).portid = 0;
	NETLINK_CB(skb_1).dst_group = 0;

	message[slen] = '\0';
	memcpy(NLMSG_DATA(nlh), message, slen + 1);
	/* printk("my_net_link:send message %d.\n",*(char *)NLMSG_DATA(nlh)); */

	ret = netlink_unicast(nl_sk_milan, skb_1, pid_milan, MSG_DONTWAIT);
	if (!ret) {
		/* kfree_skb(skb_1); */
		FP_LOG(INFO, "send msg from kernel to usespace failed ret 0x%x\n", ret);
	}
}

void nl_data_ready_milan(struct sk_buff *__skb)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	char str[100] = { 0 };

	skb = skb_get(__skb);
	if (skb->len >= NLMSG_SPACE(0)) {
		nlh = nlmsg_hdr(skb);

		memcpy(str, NLMSG_DATA(nlh), sizeof(str));
		pid_milan = nlh->nlmsg_pid;

		if (pid_milan)
			FP_LOG(INFO, "Message pid_milan %d received:%s\n", pid_milan, str);
		/* while(i--) */
		/* { */
		/* init_completion(&cmpl); */
		/* wait_for_completion_timeout(&cmpl,3 * HZ); */
		/* sendnlmsg("I am from kernel!"); */
		/* } */
		/* flag = 1; */
		kfree_skb(skb);
	}
}

int netlink_init_milan(void)
{
	struct netlink_kernel_cfg netlink_cfg;

	memset(&netlink_cfg, 0, sizeof(struct netlink_kernel_cfg));

	netlink_cfg.groups = 0;
	netlink_cfg.flags = 0;
	netlink_cfg.input = nl_data_ready_milan;
	netlink_cfg.cb_mutex = NULL;
	/* netlink_cfg.bind = NULL; */

	nl_sk_milan = netlink_kernel_create(&init_net, NETLINK_TEST_MILAN,
				&netlink_cfg);

	if (!nl_sk_milan) {
		FP_LOG(ERR, "my_net_link: create netlink socket error.\n");
		return -EPERM;
	}

	return 0;
}

void netlink_exit_milan(void)
{
	if (nl_sk_milan != NULL) {
		netlink_kernel_release(nl_sk_milan);
		nl_sk_milan = NULL;
	}

	FP_LOG(INFO, "my_net_link: self module exited\n");
}
