#ifndef _NIC_CLASSIFIER_H_
#define _NIC_CLASSIFIER_H_

#include "./util/fwd_rule.h"
#include "micronf_agent.h"
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


#define PACKET_READ_SIZE 32

class NICClassifier {
	public:
		NICClassifier(){}
		void Init(MicronfAgent* agent);
		void Run();
		void AddRule(const FwdRule&);

	private:
		MicronfAgent* agent_;
    std::vector<std::unique_ptr<FwdRule>> fwd_rules_;
    std::vector<rte_ring*> rings_;
    std::vector<std::unique_ptr<struct rte_mbuf*>> rule_buffers_;
    std::vector<size_t> rule_buffer_cnt_;
};

#endif
