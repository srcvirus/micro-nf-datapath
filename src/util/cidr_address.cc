#include "cidr_address.h"

CIDRAddress::CIDRAddress(const std::string& str) {
  int slash_idx = str.find("/");
 	std::string ip_str = str.substr(0, slash_idx);
  std::string mask_str = str.substr(slash_idx + 1);
  this->subnet_mask_ = (0xffffffff << (32 - std::stoi(mask_str)));

  // Construct IP address octect by octect.
  // First octect. (MSB)
  int dot_idx = ip_str.find(".");
  this->ip_address_ = (std::stoi(ip_str.substr(0, dot_idx)) << 24);

  // Second octect.
  ip_str = ip_str.substr(dot_idx + 1);
  dot_idx = ip_str.find(".");
  this->ip_address_ |= (std::stoi(ip_str.substr(0, dot_idx)) << 16);

  // Third octect.
  ip_str = ip_str.substr(dot_idx + 1);
  dot_idx = ip_str.find(".");
  this->ip_address_ |= (std::stoi(ip_str.substr(0, dot_idx)) << 8);

  // Fourth octect. (LSB)
  ip_str = ip_str.substr(dot_idx + 1);
  this->ip_address_ |= std::stoi(ip_str);
}
