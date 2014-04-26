/*
 * @file send_tcp.c
 *
 * @author Akagi201
 * @date 2014/04/26
 *
 * TCP发包测试
 */

#include <libnet.h>

int main(void) {
	libnet_t *handle = NULL; /* Libnet句柄 */
	int packet_size = 0; /* 构造的数据包大小 */
	char *device = "eth0"; /* 设备名字,也支持点十进制的IP地址,会自己找到匹配的设备 */
	char *src_ip_str = "192.168.90.102"; /* 源IP地址字符串 */
	char *dst_ip_str = "192.168.90.201"; /* 目的IP地址字符串 */
	u_char src_mac[6] = { 0x08, 0x00, 0x27, 0x08, 0x69, 0x58 }; /* 源MAC */
	u_char dst_mac[6] = { 0x14, 0xda, 0xe9, 0xdd, 0xea, 0x40 }; /* 目的MAC */
	/* 网络序的目的IP和源IP */
	u_long dst_ip = 0;
	u_long src_ip = 0;
	char error[LIBNET_ERRBUF_SIZE] = { 0 }; /* 出错信息 */

	/* 各层build函数返回值 */
	libnet_ptag_t eth_tag;
	libnet_ptag_t ip_tag;
	libnet_ptag_t tcp_tag;
	libnet_ptag_t tcp_op_tag;

	u_short proto = IPPROTO_TCP; /* 传输层协议 */
	u_char payload[255] = { 0 }; /* 承载数据的数组，初值为空 */
	u_long payload_s = 0; /* 承载数据的长度，初值为0 */

	/* 把目的IP地址字符串转化成网络序 */
	dst_ip = libnet_name2addr4(handle, dst_ip_str, LIBNET_RESOLVE);

	/* 把源IP地址字符串转化成网络序 */
	src_ip = libnet_name2addr4(handle, src_ip_str, LIBNET_RESOLVE);

	/* 初始化libnet */
	if ((handle = libnet_init(LIBNET_LINK, device, error)) == NULL) {
		printf("libnet_init failure\n");
		return (-1);
	}

	strncpy(payload, "test", sizeof(payload) - 1); /* 构造负载的内容 */
	payload_s = strlen(payload); /* 计算负载内容的长度 */

#if 0
	/* 构建TCP的选项,通常在第一个TCP通信报文中设置MSS */
	tcp_op_tag = libnet_build_tcp_options(
			payload,
			payload_s,
			handle,
			0
	);
	if (-1 == tcp_op_tag) {
		printf("build_tcp_options failure\n");
		return (-2);
	};
#endif

	tcp_tag = libnet_build_tcp(30330, /* 源端口 */
	30331, /* 目的端口 */
	8888, /* 序列号 */
	8889, /* 确认号 */
	TH_PUSH | TH_ACK, /* Control flags */
	14600, /* 窗口尺寸 */
	0, /* 校验和,0为自动计算 */
	0, /* 紧急指针 */
	LIBNET_TCP_H + payload_s, /* 长度 */
	payload, /* 负载内容 */
	payload_s, /* 负载内容长度 */
	handle, /* libnet句柄 */
	0 /* 新建包 */
	);

	if (-1 == tcp_tag) {
		printf("libnet_build_tcp failure\n");
		return (-3);
	}

	/* 构造IP协议块，返回值是新生成的IP协议块的一个标记 */
	ip_tag = libnet_build_ipv4(LIBNET_IPV4_H + LIBNET_TCP_H + payload_s, /* IP协议块的总长,*/
	0, /* tos */
	(u_short) libnet_get_prand(LIBNET_PRu16), /* id,随机产生0~65535 */
	0, /* frag 片偏移 */
	(u_int8_t) libnet_get_prand(LIBNET_PR8), /* ttl,随机产生0~255 */
	proto, /* 上层协议 */
	0, /* 校验和，此时为0，表示由Libnet自动计算 */
	src_ip, /* 源IP地址,网络序 */
	dst_ip, /* 目标IP地址,网络序 */
	NULL, /* 负载内容或为NULL */
	0, /* 负载内容的大小*/
	handle, /* Libnet句柄 */
	0 /* 协议块标记可修改或创建,0表示构造一个新的*/
	);

	if (-1 == ip_tag) {
		printf("libnet_build_ipv4 failure\n");
		return (-4);
	}

	/* 构造一个以太网协议块,只能用于LIBNET_LINK */
	eth_tag = libnet_build_ethernet(dst_mac, /* 以太网目的地址 */
	src_mac, /* 以太网源地址 */
	ETHERTYPE_IP, /* 以太网上层协议类型，此时为IP类型 */
	NULL, /* 负载，这里为空 */
	0, /* 负载大小 */
	handle, /* Libnet句柄 */
	0 /* 协议块标记，0表示构造一个新的 */
	);

	if (-1 == eth_tag) {
		printf("libnet_build_ethernet failure\n");
		return (-5);
	};

	packet_size = libnet_write(handle); /* 发送已经构造的数据包*/

	libnet_destroy(handle); /* 释放句柄 */

	return (0);
}

