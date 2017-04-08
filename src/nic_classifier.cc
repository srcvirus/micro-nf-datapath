#include "nic_classifier.h"
#include <unistd.h>
#include <rte_malloc.h>
#include <rte_prefetch.h>
#include <rte_memzone.h>
#include <stdio.h>

#include "common.h"
#include <rte_cycles.h>

void NICClassifier::Init(MicronfAgent* agent){
	//todo initialize using config file
	this->agent_ = agent;
}

void NICClassifier::Run(){
	struct rte_mbuf *buf[PACKET_READ_SIZE] = {nullptr};
  uint16_t rx_count = 0, tx_count = 0;
  register uint16_t i = 0;
  register uint16_t j = 0;
  uint64_t cur_tsc = 0, diff_tsc = 0, prev_tsc = rte_rdtsc(), timer_tsc = 0,
           total_tx = 0, cur_tx = 0;
	const uint64_t kTimerPeriod = rte_get_timer_hz() * 3;
	bool scale_flag = false;
	int idx_scale = 0;

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
        for(j = tx_count; j < rule_buffer_cnt_[i]; ++j)
          rte_pktmbuf_free(rule_buffers_[i].get()[j]);
      }
      rule_buffer_cnt_[i] = 0;
    }
		// TODO read from next port if available

    cur_tsc = rte_rdtsc();
    timer_tsc += (cur_tsc - prev_tsc);
    if (unlikely(timer_tsc > kTimerPeriod)) {
			int num_nf = this->agent_->micronf_stats->num_nf;
			printf("detecting packet drop. . . . num_nf: %d\n", num_nf);
			// Check statistic of all microservices
			for(int i=0; i < num_nf; i++){
				if(this->agent_->micronf_stats->packet_drop[i] != 0){
					// TODO
					// IMPORTANT  detect the rate & set 'scale_flag' 'idx_scale'to true
					printf("num_nf: %d\n", num_nf);
					printf("Drop at %i : %u\n", i, this->agent_->micronf_stats->packet_drop[i]);	
				}
			}
			
			if(scale_flag){
				// note i-1 is assumed to be the previous microservice needed to scale
				this->agent_->scale_bits->bits[idx_scale - 1]	= 1;
			}

      timer_tsc = 0;
    }
    prev_tsc = cur_tsc;

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
