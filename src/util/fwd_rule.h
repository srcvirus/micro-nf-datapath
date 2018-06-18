#ifndef _FWD_RULE_H_
#define _FWD_RULE_H_

#include "cidr_address.h"

#include <string.h>

#include <rte_byteorder.h>
#include <rte_malloc.h>
#include <rte_memcpy.h>

class FwdRule {
	public:
		inline FwdRule() {}
  	inline FwdRule(const CIDRAddress& src_addr, const CIDRAddress& dst_addr,
          uint16_t src_port, uint16_t dst_port, const char* ring_name)
      : src_addr_(src_addr),
        dst_addr_(dst_addr),
        src_port_(rte_cpu_to_be_16(src_port)),
        dst_port_(rte_cpu_to_be_16(dst_port)) {
          size_t len = strlen(ring_name);
          to_ring_ = reinterpret_cast<char*>(
              rte_zmalloc(NULL, len, RTE_CACHE_LINE_SIZE));
          rte_memcpy(to_ring_, ring_name, len);
        }
    inline FwdRule(const FwdRule& fwd_rule) {
      size_t len = strlen(fwd_rule.to_ring_);
      this->src_addr_ = fwd_rule.src_addr_;
      this->dst_addr_ = fwd_rule.dst_addr_;
      this->src_port_ = fwd_rule.src_port_;
      this->dst_port_ = fwd_rule.dst_port_;
      this->to_ring_ = reinterpret_cast<char*>(
          rte_zmalloc(NULL, len, RTE_CACHE_LINE_SIZE));
      rte_memcpy(to_ring_, fwd_rule.to_ring_, len);
    }
  	inline bool Match(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port,
             uint16_t dst_port) {
           return true;
           //return src_addr_.Match(src_ip) && dst_addr_.Match(dst_ip) &&
           //src_port == src_port_ && dst_port == dst_port_;
  	}
    inline FwdRule& operator=(const FwdRule& fwd_rule) {
      size_t len = strlen(fwd_rule.to_ring_);
      this->src_addr_ = fwd_rule.src_addr_;
      this->dst_addr_ = fwd_rule.dst_addr_;
      this->src_port_ = fwd_rule.src_port_;
      this->dst_port_ = fwd_rule.dst_port_;
      this->to_ring_ = reinterpret_cast<char*>(
          rte_zmalloc(NULL, len, RTE_CACHE_LINE_SIZE));
      rte_memcpy(to_ring_, fwd_rule.to_ring_, len);
    }
		const char* to_ring() const { return static_cast<const char*>(to_ring_); }				
    ~FwdRule() {
      rte_free(to_ring_);
    }
	private:
		CIDRAddress src_addr_;
		CIDRAddress dst_addr_;
		uint16_t src_port_;
		uint16_t dst_port_;
    char *to_ring_;
};

#endif
