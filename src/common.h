#ifndef _COMMON_H_
#define _COMMON_H_

#define MAX_NUM_MS 100

class MSStats {
	public:
		int num_nf;
		unsigned int packet_drop[MAX_NUM_MS]; 
};

#endif //_COMMON_H_
