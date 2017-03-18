#ifndef _CIDR_ADDRESS_H_
#define _CIDR_ADDRESS_H_

#include <stdint.h>
#include <string>

#define IPAddress(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | ((d)))

class CIDRAddress {
 public:
  CIDRAddress() : ip_address_(0), subnet_mask_(0) {}
  CIDRAddress(const std::string& addr_str);
  bool Match(const uint32_t& ip_address) {
    return (ip_address & subnet_mask_) == (ip_address_ & subnet_mask_);
  }
  uint32_t ip_address() const { return ip_address_; }
  uint32_t subnet_mask() const { return subnet_mask_; }

 private:
  uint32_t ip_address_;
  uint32_t subnet_mask_;
};

#endif
