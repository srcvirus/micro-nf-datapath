#include "cidr_address.h"

#include <rte_byteorder.h>

CIDRAddress::CIDRAddress(const std::string& str) {
  int slash_idx = str.find("/");
 	std::string ip_str = str.substr(0, slash_idx);
  std::string mask_str = str.substr(slash_idx + 1);
  this->subnet_mask_ = rte_cpu_to_be_32(
      (0xffffffff << (32 - std::stoi(mask_str))));

  // Construct IP address octect by octect.
  // First octect. (MSB)
  int dot_idx = ip_str.find(".");
  uint32_t temp_ip = 0;
  temp_ip = (std::stoi(ip_str.substr(0, dot_idx)) << 24);

  // Second octect.
  ip_str = ip_str.substr(dot_idx + 1);
  dot_idx = ip_str.find(".");
  temp_ip |= (std::stoi(ip_str.substr(0, dot_idx)) << 16);

  // Third octect.
  ip_str = ip_str.substr(dot_idx + 1);
  dot_idx = ip_str.find(".");
  temp_ip |= (std::stoi(ip_str.substr(0, dot_idx)) << 8);

  // Fourth octect. (LSB)
  ip_str = ip_str.substr(dot_idx + 1);
  temp_ip |= std::stoi(ip_str);

  this->ip_address_ = rte_cpu_to_be_32(temp_ip);
}
