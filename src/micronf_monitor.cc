#include "micronf_monitor.h"
#include <rte_cycles.h>

#define DROP_RATE_LIMIT 50

void MicronfMonitor::Init(MicronfAgent* agent){
	this->agent_ = agent; 
}

void MicronfMonitor::Run(){
  uint64_t cur_tsc = 0, diff_tsc = 0, prev_tsc = rte_rdtsc(), timer_tsc = 0,
           total_tx = 0, cur_tx = 0;
  const uint64_t kTimerPeriod = rte_get_timer_hz() * 2;
  int idx_scale = 0;
	unsigned int drop_history[MAX_NUM_MS][MAX_NUM_PORT][2] = {};	
	unsigned int round = 0;	

	while(1){
		cur_tsc = rte_rdtsc();
		timer_tsc += (cur_tsc - prev_tsc);
		int num_nf = this->agent_->micronf_stats->num_nf;

		if (unlikely(timer_tsc > kTimerPeriod)) {
			printf("detecting packet drop. . . . num_nf: %d\n", num_nf);
			
			for(int i=0; i < num_nf; i++){
				for(int j=0; j < MAX_NUM_PORT; j++){
					if(this->agent_->micronf_stats->packet_drop[i][j] != 0){
						// TODO
						// IMPORTANT  detect the rate & set 'scale_flag' 'idx_scale'to true
						printf("num_nf: %d\n", num_nf);
						printf("Drop at [%x][%x] : %u\n", i, j, this->agent_->micronf_stats->packet_drop[i][j]);
						drop_history[i][j][round % 2] = this->agent_->micronf_stats->packet_drop[i][j];
						
					}
				}
			}
			
			timer_tsc = 0;
			round++;
			if(round % 2 == 0){
				for(int i=0; i < num_nf; i++){
					for(int j=0; j < MAX_NUM_PORT; j++){
						if(drop_history[i][j][1] - drop_history[i][j][0] > DROP_RATE_LIMIT){
							this->agent_->scale_bits->bits[i + 1][j] = 1;
							// FIXME sleep to prevent trying to scale twice
						}
					}
				}
			}
		}
		prev_tsc = cur_tsc;
	}
}
