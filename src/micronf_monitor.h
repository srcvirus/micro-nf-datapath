#ifndef _MICRONF_MONITOR_H_
#define _MICRONF_MONITOR_H_

#include "micronf_agent.h"

class MicronfMonitor {
  public:
   MicronfMonitor(){}
   void Init( MicronfAgent* agent, bool dry );
   void Run();

  private:
   MicronfAgent* agent_;
   bool dry_run_;

};


#endif
