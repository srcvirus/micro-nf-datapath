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
   struct ether_hdr* ethernet = nullptr;
   uint16_t rx_count = 0, tx_count = 0;
   register int16_t i = 0;
   const int16_t kNumPrefetch = 8;
   printf("NicClassifier thread loop has started\n");

   for(;;) {
      rx_count = rte_eth_rx_burst(0, 0, buf, PACKET_READ_SIZE);
      // for (i = 0; i < rx_count && i < kNumPrefetch; ++i)
      //   rte_prefetch0(rte_pktmbuf_mtod(buf[i], void*));
      for (i = 0; i < rx_count; ++i) {
         // rte_prefetch0(rte_pktmbuf_mtod(buf[i + kNumPrefetch], void*));
         ethernet = rte_pktmbuf_mtod(buf[i], struct ether_hdr*);
         std::swap(ethernet->s_addr.addr_bytes, ethernet->d_addr.addr_bytes);
      }
      tx_count = rte_eth_tx_burst( 1, 0, buf, rx_count );
      for (i = tx_count; i < rx_count; ++i)
         rte_pktmbuf_free(buf[i]);
   }
}

void NICClassifier::AddRule(const FwdRule& fwd_rule){
   std::unique_ptr<FwdRule> rule(
         reinterpret_cast<FwdRule*>(
               rte_zmalloc(NULL, sizeof(FwdRule), RTE_CACHE_LINE_SIZE)));
   *(rule.get()) = fwd_rule;
   fwd_rules_.push_back(std::move(rule));
   auto ptr = std::unique_ptr<struct rte_mbuf*>(
         reinterpret_cast<struct rte_mbuf**>(
               rte_zmalloc(NULL, sizeof(struct rte_mbuf*) * PACKET_READ_SIZE, 
                           RTE_CACHE_LINE_SIZE)));
   rule_buffers_.push_back(std::move(ptr));
   rule_buffer_cnt_.push_back(0);
   rings_.push_back(rte_ring_lookup(fwd_rule.to_ring()));
}
