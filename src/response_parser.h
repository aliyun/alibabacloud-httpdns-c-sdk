//
// Created by cagaoshuai on 2024/1/19.
//

#ifndef HTTPDNS_C_SDK_HTTPDNS_RESPONSE_PARSER_H
#define HTTPDNS_C_SDK_HTTPDNS_RESPONSE_PARSER_H

#include "httpdns_result.h"
#include "cJSON.h"
#include "sds.h"
#include "httpdns_list.h"


/*
 报文示例：
{
	"service_ip":["203.107.1.65",
	"203.107.1.34",
	"203.107.1.66",
	"203.107.1.33",
	"203.107.1.1"],
	"service_ipv6":["2401:b180:2000:20::1c",
	"2401:b180:2000:20::10"]
}
 */
httpdns_raw_schedule_result_t *parse_schedule_result(char *body);

/*
 * 报文示例：
{
"ipsv6":["240e:960:c00:e:3:0:0:3ef","240e:960:c00:e:3:0:0:3f0"],
"host":"www.aliyun.com",
"client_ip":"47.96.236.37",
"ips":["47.118.227.108","47.118.227.111","47.118.227.112"],
"ttl":60,
"origin_ttl":60
}
 */

httpdns_raw_single_resolve_result_t *parse_single_resolve_result(char *body);


/*
{
	"dns":[{
		"host":"www.aliyun.com",
		"client_ip":"47.96.236.37",
		"ips":["47.118.227.116"],
		"type":1,
		"ttl":26,
		"origin_ttl":60
	},
	{
		"host":"www.taobao.com",
		"client_ip":"47.96.236.37",
		"ips":["240e:f7:a093:101:3:0:0:3e8"],
		"type":28,
		"ttl":60,
		"origin_ttl":60
	}]
}
*/
httpdns_raw_multi_resolve_result_t *parse_multi_resolve_result(char *body);

#endif //HTTPDNS_C_SDK_HTTPDNS_RESPONSE_PARSER_H
