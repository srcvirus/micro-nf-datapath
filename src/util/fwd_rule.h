#ifndef _FWD_RULE_H_
#define _FWD_RULE_H_

#include "cidr_address.h"

class FwdRule {
	public:
		FwdRule() {}
  	FwdRule(const CIDRAddress& src_addr, const CIDRAddress& dst_addr,
          uint16_t src_port, uint16_t dst_port, std::string ring_name)
      : src_addr_(src_addr),
        dst_addr_(dst_addr),
        src_port_(src_port),
        dst_port_(dst_port),
        to_ring_(ring_name) {}

  	bool Match(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port,
             uint16_t dst_port) {
    	return src_addr_.Match(src_ip) && dst_addr_.Match(dst_ip) &&
           src_port == src_port_ && dst_port == dst_port_;
  	}

		std::string to_ring() const { return to_ring_; }				

	private:
		CIDRAddress src_addr_;
		CIDRAddress dst_addr_;
		uint16_t src_port_;
		uint16_t dst_port_;
		std::string to_ring_;
};

#endif
