#ifndef _NIC_CLASSIFIER_H_
#define _NIC_CLASSIFIER_H_

#include <string>
#include <vector>
#include <utility> 

#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_tcp.h>
#include <rte_ip.h>
#include <rte_ring.h>

#include "./util/fwd_rule.h"
#include "micronf_agent.h"

#define PACKET_READ_SIZE 32

class NICClassifier {
	public:
		NICClassifier(){}
		void Init(MicronfAgent* agent);
		void Run();
		void AddRule(const FwdRule&);

	private:
		class EgressRxBuffer{
			public:
				EgressRxBuffer() : count(0) {}

				struct rte_mbuf* buffer[PACKET_READ_SIZE];
				struct rte_ring* ring;
				uint16_t count;
		};

	private:
		MicronfAgent* agent_;

		std::vector<FwdRule> fwd_rules_;
		std::map<const std::string, EgressRxBuffer*> egress_rx_buf_;

		void EnqueueRxPacket(std::string, struct rte_mbuf*);
		void FlushRxQueue(std::string);

};

#endif
