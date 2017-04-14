#include "micronf_monitor.h"
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <rte_cycles.h>

#define DROP_RATE_LIMIT 10  // 4 x 32 (batch)

void MicronfMonitor::Init(MicronfAgent* agent){
	this->agent_ = agent; 
}

void MicronfMonitor::Run(){
  uint64_t cur_tsc = 0, diff_tsc = 0, prev_tsc = rte_rdtsc(), timer_tsc = 0,
           total_tx = 0, cur_tx = 0;
	// gather the statistic per second
  const uint64_t kTimerPeriod = rte_get_timer_hz() * 1;
	// wait 5s after the scale-out operation is performed
  const uint64_t countupTimerPeriod = rte_get_timer_hz() * 5;
	unsigned int drop_history[MAX_NUM_MS][MAX_NUM_PORT][2] = {};	
	unsigned int round = 0;
	uint64_t countup_timer[MAX_NUM_MS][MAX_NUM_PORT] = {};

	//initialize timer
	for(int i=0; i < MAX_NUM_MS; i++)
		for(int j=0; j < MAX_NUM_PORT; j++){
			countup_timer[i][j] = countupTimerPeriod;
		}

	while(1){
		cur_tsc = rte_rdtsc();
		timer_tsc += (cur_tsc - prev_tsc);
		for(int i=0; i < MAX_NUM_MS; i++)
			for(int j=0; j < MAX_NUM_PORT; j++)
				if(countup_timer[i][j] < countupTimerPeriod)
						countup_timer[i][j] += (cur_tsc - prev_tsc);

		int num_nf = this->agent_->micronf_stats->num_nf;

		if (unlikely(timer_tsc > kTimerPeriod)) {
			printf("detecting packet drop. . . . num_nf: %d\n", num_nf);
			
			for(int i=0; i < num_nf; i++){
				for(int j=0; j < MAX_NUM_PORT; j++){
					if(this->agent_->micronf_stats->packet_drop[i][j] != 0){
						printf("num_nf: %d\n", num_nf);
						printf("Drop at [%x][%x] : %u\n", i, j, this->agent_->micronf_stats->packet_drop[i][j]);
						drop_history[i][j][round & 1] = this->agent_->micronf_stats->packet_drop[i][j];
						
					}
				}
			}
			
			if(round & 1){
				for(int i=0; i < num_nf; i++){
					for(int j=0; j < MAX_NUM_PORT; j++){
						if(drop_history[i][j][1] - drop_history[i][j][0] > DROP_RATE_LIMIT){
							if(countup_timer[i][j] >= countupTimerPeriod){
								this->agent_->scale_bits->bits[i].set(j, true);
								countup_timer[i][j] = 0;	
								printf("scale out timer is fired up!\n");
								int next_pp_id = std::get<0>(this->agent_->neighborGraph[i][j]);
								int next_pp_port = std::get<1>(this->agent_->neighborGraph[i][j]);
								PacketProcessorConfig pp_config_scale = this->agent_->ppConfigList[next_pp_id];
								int int_new_instance_id = this->agent_->getNewInstanceId();
								std::string new_instance_id = std::to_string(int_new_instance_id);
								std::string new_conf_path = "../confs/mac_swapper_" + new_instance_id+".conf";

								pp_config_scale.set_instance_id(int_new_instance_id);
								int fd = open(new_conf_path.c_str(), O_RDWR|O_CREAT, 0644);
								if(fd < 0)
									rte_exit(EXIT_FAILURE, "Cannot open configuration file %s\n", 
															new_conf_path.c_str());
								printf("\ni:%d j:%d next_pp_id: %d\n",i, j, next_pp_id);		
								printf("\npp_config_scale\n");		
								printf("pp_config_scale class: %s\n", pp_config_scale.packet_processor_class().c_str());		
								printf("pp_config_scale num_ingress: %d\n", pp_config_scale.num_ingress_ports());		
	
								google::protobuf::io::FileOutputStream config_file_handle(fd);
						 		config_file_handle.SetCloseOnDelete(true);
								google::protobuf::TextFormat::Print(pp_config_scale, &config_file_handle);
								
								//std::string ring_after_lb = this->agent_->getScaleRingName();						
								//std::string ring_before_lb = this->agent_->getScaleRingName();						
								//todo change instance_id ring_id
								//this->agent_->CreateRing(new_r1);
								//this->agent_->CreateRing(new_r2);
								this->agent_->DeployOneMicroService(pp_config_scale, new_conf_path);	
							}
						}
					}
				}
			}

			round++;
			timer_tsc = 0;
		}

		prev_tsc = cur_tsc;
	}
}
