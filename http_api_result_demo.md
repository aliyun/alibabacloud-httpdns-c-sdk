
```http request
https://203.107.1.34/139450/resolve?host=www.aliyun.com,qq.com,www.taobao.com,help.aliyun.com&query=4,6
```

```json
{"results":[{"host":"www.aliyun.com","client_ip":"47.96.236.37","ips":["47.118.227.116","47.118.227.107","47.118.227.112","47.118.227.113","47.118.227.108","47.118.227.109","47.118.227.111","47.118.227.115"],"type":1,"ttl":26,"origin_ttl":60},{"host":"www.taobao.com","client_ip":"47.96.236.37","ips":["240e:f7:a093:101:3:0:0:3e8","240e:f7:a093:101:3:0:0:3e7"],"type":28,"ttl":60,"origin_ttl":60},{"host":"www.taobao.com","client_ip":"47.96.236.37","ips":["150.139.156.206","150.139.156.207"],"type":1,"ttl":60,"origin_ttl":60},{"host":"qq.com","client_ip":"47.96.236.37","ips":["113.108.81.189","123.150.76.218","203.205.254.157"],"type":1,"ttl":600,"origin_ttl":600},{"host":"qq.com","client_ip":"47.96.236.37","ips":[],"type":28,"ttl":3600},{"host":"help.aliyun.com","client_ip":"47.96.236.37","ips":["203.119.144.45"],"type":1,"ttl":120,"origin_ttl":120},{"host":"help.aliyun.com","client_ip":"47.96.236.37","ips":[],"type":28,"ttl":3600},{"host":"www.aliyun.com","client_ip":"47.96.236.37","ips":["240e:960:c00:e:3:0:0:3f0","240e:960:c00:e:3:0:0:3ef"],"type":28,"ttl":60,"origin_ttl":60}]}
```


```http request
https://203.107.1.34/139450/d?host=www.aliyun.com&query=4,6
```
```json
{"host":"www.aliyun.com","ipsv6":["240e:960:c00:e:3:0:0:3ef","240e:960:c00:e:3:0:0:3f0"],"client_ip":"47.96.236.37","ips":["47.118.227.116","47.118.227.113","47.118.227.109","47.118.227.111","47.118.227.107","47.118.227.115","47.118.227.112","47.118.227.108"],"ttl":14,"origin_ttl":60}
```

```http request
https://203.107.1.34/139450/d?host=www.aliyun.com
```

```json
{"host":"www.aliyun.com","ips":["47.118.227.109","47.118.227.112","47.118.227.116","47.118.227.107","47.118.227.115","47.118.227.108","47.118.227.111","47.118.227.113"],"ttl":30,"origin_ttl":60,"client_ip":"120.26.165.80"}
```