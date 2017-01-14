/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>
#include <inttypes.h>
#include <sys/queue.h>
#include <errno.h>
#include <netinet/ip.h>

#include <rte_common.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_byteorder.h>
#include <rte_launch.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_atomic.h>
#include <rte_ring.h>
#include <rte_log.h>
#include <rte_debug.h>
#include <rte_mempool.h>
#include <rte_memcpy.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_ethdev.h>
#include <rte_byteorder.h>
#include <rte_malloc.h>
#include <rte_fbk_hash.h>
#include <rte_string_fns.h>

#include "common.h"
#include "args.h"
#include "init.h"

static int
sleep_lcore(__attribute__((unused)) void *dummy)
{
    //can also be used to select one core to display stat
    return 0;
}

static void 
do_listen(void)
{
	RTE_LOG(INFO, APP, "App is waiting for API call.\n");
	for(;;){
		//TODO listen to an API to create more ring
		sleep(1);
	}
}

int
main(int argc, char *argv[])
{
	/* initialise the system */
	if (init(argc, argv) < 0 )
		return -1;
	RTE_LOG(INFO, APP, "Finished Process Init.\n");

	/* put all other cores to sleep bar master */
	rte_eal_mp_remote_launch(sleep_lcore, NULL, SKIP_MASTER);

	/* listen to an ochestrator */
	do_listen();

	return 0;
}
