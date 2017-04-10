#ifndef _COMMON_H_
#define _COMMON_H_

#define MZ_STAT "MZ_STAT"
#define MZ_SCALE "MZ_SCALE"

#include <bitset>   

#define MAX_NUM_MS 100
#define MAX_NUM_PORT 50

class MSStats {
	public:
		int num_nf;
		unsigned int packet_drop[MAX_NUM_MS][MAX_NUM_PORT]; 
};

class ScaleBitVector {
	public:
		int num_nf;
		std::bitset<MAX_NUM_PORT> bits[MAX_NUM_MS];
};

#endif //_COMMON_H_
