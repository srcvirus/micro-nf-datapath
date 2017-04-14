#ifndef _COMMON_H_
#define _COMMON_H_

#define MAX_NUM_MS 100
#define MZ_STAT "MZ_STAT"
#define MZ_SCALE "MZ_SCALE"

#include <bitset>   

class MSStats {
	public:
		int num_nf;
		unsigned int packet_drop[MAX_NUM_MS]; 
};

class ScaleBitVector{
	public:
		int num_nf;
		std::bitset<MAX_NUM_MS> bits;
};

#endif //_COMMON_H_
