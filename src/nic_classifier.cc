#include "nic_classifier.h"
#include <unistd.h>
#include <rte_malloc.h>
#include <rte_prefetch.h>
#include <stdio.h>

#include "common.h"

void NICClassifier::Init(MicronfAgent* agent){
	this->agent_ = agent;
}

void NICClassifier::Run(){
	struct rte_mbuf *buf[PACKET_READ_SIZE] = {nullptr};
  uint16_t rx_count = 0, tx_count = 0;
  register uint16_t i = 0;
  register uint16_t j = 0;
	printf("NicClassifier thread loop has started\n");

	for(;;) {
		rx_count = rte_eth_rx_burst(0, 0, buf, PACKET_READ_SIZE);
    if (unlikely(rx_count == 0)) continue;
    for(i = 0; i < rx_count; i++){
      rte_prefetch0(buf[i]);
      struct ether_hdr* ethernet = rte_pktmbuf_mtod(buf[i], struct ether_hdr*);
	    struct ipv4_hdr* ipv4 = reinterpret_cast<struct ipv4_hdr*>(ethernet + 1);
      struct tcp_hdr* tcp = reinterpret_cast<struct tcp_hdr*>(ipv4 + 1);
      for (j = 0; j < fwd_rules_.size(); ++j) {
        if (fwd_rules_[j]->Match(ipv4->src_addr, ipv4->dst_addr, tcp->src_port, 
            tcp->dst_port)) {
          rule_buffers_[j].get()[rule_buffer_cnt_[j]++] = buf[i];
          break;
        }
      }
    }
    for (i = 0; i < rule_buffers_.size(); ++i) {
      tx_count = rte_ring_enqueue_burst(rings_[i],
          reinterpret_cast<void**>(rule_buffers_[i].get()),
          rule_buffer_cnt_[i]);

      if (unlikely(tx_count < rule_buffer_cnt_[i])) {
				this->micronf_stats->packet_drop[INSTANCE_ID_0][i] += rule_buffer_cnt_[i] - tx_count;
        for(j = tx_count; j < rule_buffer_cnt_[i]; ++j)
          rte_pktmbuf_free(rule_buffers_[i].get()[j]);
      }

			if(this->scale_bits->bits[INSTANCE_ID_0].test(i)){
					// TODO 
					// Change port to smart port
			}

      rule_buffer_cnt_[i] = 0;
    }

		// TODO read from next port if available
	}
}

void NICClassifier::AddRule(const FwdRule& fwd_rule){
  std::unique_ptr<FwdRule> rule(
      reinterpret_cast<FwdRule*>(
        rte_zmalloc(NULL, sizeof(FwdRule), RTE_CACHE_LINE_SIZE)));
  *(rule.get()) = fwd_rule;
  fwd_rules_.push_back(std::move(rule));
	agent_->CreateRing(fwd_rule.to_ring());
  auto ptr = std::unique_ptr<struct rte_mbuf*>(reinterpret_cast<struct rte_mbuf**>(
        rte_zmalloc(
          NULL, sizeof(struct rte_mbuf*) * PACKET_READ_SIZE, RTE_CACHE_LINE_SIZE)));
  rule_buffers_.push_back(std::move(ptr));
  rule_buffer_cnt_.push_back(0);
  rings_.push_back(rte_ring_lookup(fwd_rule.to_ring()));
}
