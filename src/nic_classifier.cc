#include "nic_classifier.h"
#include <rte_malloc.h>
#include <rte_prefetch.h>
#include <sched.h>
#include <stdio.h>
#include <unistd.h>

#include "common.h"
#include <rte_log.h>

void inline set_scheduler(int pid) {
  // Change scheduler to RT Round Robin
  int rc, old_sched_policy;
  struct sched_param my_params;
  my_params.sched_priority = 99;
  old_sched_policy = sched_getscheduler(pid);
  rc = sched_setscheduler(pid, SCHED_RR, &my_params);
  if (rc == -1) {
    printf("sched_setscheduler call is failed\n");
    exit(-1);
  } else {
    printf(
        "set_scheduler(). pid: %d. old_sched_policy:  %d. new_sched_policy: %d "
        "\n",
        pid, old_sched_policy, sched_getscheduler(pid));
  }
}

void NICClassifier::Init(MicronfAgent* agent) {
   this->agent_ = agent;
   RTE_LOG(INFO, PMD, "nic class pid: %d \n", getpid());
   set_scheduler( 0 );
}

void NICClassifier::Run() {
  struct rte_mbuf* buf[PACKET_READ_SIZE] = {nullptr};
  struct ether_hdr* ethernet = nullptr;
  struct ipv4_hdr* ipv4 = nullptr;
  struct tcp_hdr* tcp = nullptr;
  uint16_t rx_count = 0, tx_count = 0;
  register int16_t i = 0;
  register uint16_t j = 0;
  const int16_t kNumPrefetch = 8;
  int p = 0;
  int rx_ports = 1;
  printf("NicClassifier thread loop has started\n");
  for (;;) {
     p = 0;
     while ( p < rx_ports ) {
        rx_count = rte_eth_rx_burst( p, 0, buf, PACKET_READ_SIZE);
           assert(rings_[ p ] != nullptr);
           tx_count = rte_ring_enqueue_burst(
                 rings_[ p ], reinterpret_cast<void**>( buf ),
                 rx_count, NULL);
           for (j = tx_count; j < rx_count; ++j)
              rte_pktmbuf_free( buf[j] );      
        p++;
     }
  }
  // TODO read from next port if available
  if (this->scale_bits->bits[INSTANCE_ID_0].test(i)) {
    // TODO: Change port to smart port
  }
}

void NICClassifier::AddRule(const FwdRule& fwd_rule) {
  std::unique_ptr<FwdRule> rule(reinterpret_cast<FwdRule*>(
      rte_zmalloc(NULL, sizeof(FwdRule), RTE_CACHE_LINE_SIZE)));
  *(rule.get()) = fwd_rule;
  fwd_rules_.push_back(std::move(rule));

  auto ptr =
      std::unique_ptr<struct rte_mbuf*>(reinterpret_cast<struct rte_mbuf**>(
          rte_zmalloc(NULL, sizeof(struct rte_mbuf*) * PACKET_READ_SIZE,
                      RTE_CACHE_LINE_SIZE)));
  rule_buffers_.push_back(std::move(ptr));
  rule_buffer_cnt_.push_back(0);
  rings_.push_back(rte_ring_lookup(fwd_rule.to_ring()));
}
