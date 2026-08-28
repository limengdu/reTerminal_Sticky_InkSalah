#pragma once

#include <cstddef>
#include <cstdint>

// Builds one DNS A response that points the requested host to the portal IP.
// 生成一份DNS A记录响应，把请求的域名统一指向配网页地址。
size_t captive_dns_build_response(const uint8_t *query,
                                  size_t query_length,
                                  const uint8_t portal_ip[4],
                                  uint8_t *response,
                                  size_t response_capacity);
