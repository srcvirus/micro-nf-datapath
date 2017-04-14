#ifndef _NIC_CLASSIFIER_H_
#define _NIC_CLASSIFIER_H_

#include "./util/fwd_rule.h"
#include "micronf_agent.h"
#include "common.h"

#include <memory>
#include <string>
#include <utility> 
#include <vector>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ip.h>
#include <rte_mbuf.h>
#include <rte_ring.h>
#include <rte_tcp.h>
#include <rte_memzone.h>

#define PACKET_READ_SIZE 64
#define INSTANCE_ID_0 0

class NICClassifier {
	public:
		NICClassifier(){
		  this->stat_mz = rte_memzone_lookup(MZ_STAT);
		  this->micronf_stats = (MSStats*) this->stat_mz->addr;
			this->scale_bits_mz = rte_memzone_lookup(MZ_SCALE);
	    this->scale_bits = (ScaleBitVector*) this->scale_bits_mz->addr;
		}
		void Init(MicronfAgent* agent);
		void Run();
		void AddRule(const FwdRule&);

	private:
		MicronfAgent* agent_;
    std::vector<std::unique_ptr<FwdRule>> fwd_rules_;
    std::vector<rte_ring*> rings_;
    std::vector<std::unique_ptr<struct rte_mbuf*>> rule_buffers_;
    std::vector<size_t> rule_buffer_cnt_;
	
	protected:
  	const struct rte_memzone *scale_bits_mz;
 		ScaleBitVector *scale_bits;
		const struct rte_memzone *stat_mz;
		MSStats* micronf_stats;
};

#endif
