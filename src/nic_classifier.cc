#include "nic_classifier.h"

void NICClassifier::Init(MicronfAgent* agent){
	//todo initialize using config file
	this->agent_ = agent;
}

void NICClassifier::Run(){
	int in_port_id = 0;

	for(;;){
		struct rte_mbuf *buf[PACKET_READ_SIZE];
		uint16_t rx_count; 

		rx_count = rte_eth_rx_burst(in_port_id, 0, buf, PACKET_READ_SIZE);
		
		if(likely(rx_count > 0)){
			for(int i=0; i<rx_count; i++){
				struct ether_hdr* ethernet = rte_pktmbuf_mtod(buf[i], struct ether_hdr*);
				struct ipv4_hdr* ipv4 = reinterpret_cast<struct ipv4_hdr*>(ethernet + 1);
        struct tcp_hdr* tcp = reinterpret_cast<struct tcp_hdr*>(ipv4 + 1);
		
				for(auto& rule : fwd_rules_){
					if(rule.Match(ipv4->src_addr, ipv4->dst_addr, tcp->src_port, 
												tcp->dst_port)){
						EnqueueRxPacket(rule.to_ring(), buf[i]);
					}
				}		
			}
			//FIXME read from next port if available
		}
		
		// flushing the queued packets to ring
		for(auto& rule : fwd_rules_){
			FlushRxQueue(rule.to_ring());
		}
	}
}

void NICClassifier::AddRule(const FwdRule& fwd_rule){
	fwd_rules_.push_back(fwd_rule);
	agent_->CreateRing(fwd_rule.to_ring());
	egress_rx_buf_.insert(std::pair<const std::string, EgressRxBuffer*>(fwd_rule.to_ring(), 
													new NICClassifier::EgressRxBuffer()));

}

void NICClassifier::EnqueueRxPacket(std::string ring_name, struct rte_mbuf* pkt){
	egress_rx_buf_[ring_name]->buffer[egress_rx_buf_[ring_name]->count++] = pkt;	
}

void NICClassifier::FlushRxQueue(std::string ring_name){
	if(egress_rx_buf_[ring_name]->count == 0)
		return;
	struct rte_ring* rx_ring = rte_ring_lookup(ring_name.c_str());
	if(rte_ring_enqueue_bulk(rx_ring, (void **)egress_rx_buf_[ring_name]->buffer, 
			egress_rx_buf_[ring_name]->count) != 0){
		//dropping packet
		for(int j=0; j < egress_rx_buf_[ring_name]->count; j++)
			rte_pktmbuf_free(egress_rx_buf_[ring_name]->buffer[j]);
	}
	
	egress_rx_buf_[ring_name]->count = 0;
}


