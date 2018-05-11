local mg     = require "moongen"
local memory = require "memory"
local device = require "device"
local ts     = require "timestamping"
local stats  = require "stats"
local hist   = require "histogram"
local timer  = require "timer"
local log    = require "log"

local ETH_DST = "3c:fd:fe:ad:84:54"
local ETH_SRC = "3c:fd:fe:a9:3a:88"
local SRC_IP = "10.0.0.10"
local DST_IP = "10.0.0.7"
local SRC_PORT = 1234
local DST_PORT = 5678
local loadSlave_CPUID = 5
local timestamp_CPUID = 3

function configure(parser)
	parser:description("generate traffic and measure the latency.")
	parser:argument("dev0", "Device to recv from"):convert(tonumber)
	parser:argument("dev1", "Device to transmit from"):convert(tonumber)
	parser:option("-r --rate", "Transmit rate in Mbit/s"):default(10000):convert(tonumber)
	parser:option("-s --size", "Size of the packets in bytes."):default(60):convert(tonumber)
	parser:option("-d --duration", "Duration of the expriment in seconds."):default(10):convert(tonumber)
	parser:option("-f --file", "Filename of the latency histogram."):default("histogram.csv")
	parser:option("-l --load", "Rate for additional load run together with TS task in Mbps."):default(0):convert(tonumber)
end

function master(args)
	local dev0 = device.config({port = args.dev0, rxQueues = 2})
	local dev1 = device.config({port = args.dev1, txQueues = 2})
	device.waitForLinks()
        
        if args.load > 0 then 
                dev1:getTxQueue(0):setRate( args.load )
        	mg.startTaskOnCore(loadSlave_CPUID, "loadSlave", dev1:getTxQueue(0), args.duration, args.size)
	        mg.startTask("counterSlave", dev0:getRxQueue(0), args.duration)
        else 
             print('Remember ts packets are sent and received sequentially in blocking manner')
        end

	mg.startTaskOnCore(timestamp_CPUID, "timestamper", dev1:getTxQueue(1), dev0:getRxQueue(1), args.size, args.duration, args.file):wait()
	mg.waitForTasks()
	mg.stop()
end


function timestamper(txQueue, rxQueue, size, duration, fl)
        local runtime = timer:new(duration)           -- Total Runtime
	local rateLimit = timer:new(0.001)      -- Iter-transmission delay
        local maxWait = 15                      -- Miliseconds
        local hist = hist:new()
        local timestamper
	file = io.open("fl", "w")
	io.output(file)     
	timestamper = ts:newTimestamper(txQueue, rxQueue)
        mg.sleepMillis(1000) -- ensure that the load task is running
	
        while mg.running() and runtime:running() do
                local lat = timestamper:measureLatency( size, function(buf)
                        buf:getEthernetPacket():fill{
                                ethSrc = ETH_SRC,
			        ethDst = ETH_DST,
			        ethType = 0x88F7
		        } 
                        end,  maxWait )
                hist:update(lat)
		rateLimit:wait()
		rateLimit:reset()
        end
        hist:print()
        hist:save(file)
	io.close(file)
        if  hist.numSamples == 0 then
                print("Received no packets.")
        end
end

local IP_SRC	= "192.168.0.1"
local NUM_FLOWS	= 256 -- src ip will be IP_SRC + random(0, NUM_FLOWS - 1)
local IP_DST	= "10.0.0.1"
local PORT_SRC	= 1234

function loadSlave(queue, duration, size)
        local runtime = timer:new(duration + 1)           -- Total Runtime
        local port = 1234
	mg.sleepMillis(100) -- wait a few milliseconds to ensure that the rx thread is running
	-- TODO: implement barriers
	local mem = memory.createMemPool(function(buf)
		buf:getUdpPacket():fill{
			pktLength = size, -- this sets all length headers fields in all used protocols
			ethSrc = queue, -- get the src mac from the device
			ethDst = ETH_DST,
			-- ipSrc will be set later as it varies
			ip4Dst = IP_DST,
			udpSrc = PORT_SRC,
			udpDst = port,
			-- payload will be initialized to 0x00 as new memory pools are initially empty
		}
	end)
	-- TODO: fix per-queue stats counters to use the statistics registers here
	local txCtr = stats:newManualTxCounter("Port " .. port, "plain")
	local baseIP = parseIPAddress(IP_SRC)
	-- a buf array is essentially a very thing wrapper around a rte_mbuf*[], i.e. an array of pointers to packet buffers
	local bufs = mem:bufArray()
	while mg.running() and runtime:running() do
		-- allocate buffers from the mem pool and store them in this array
		bufs:alloc(size)
		for _, buf in ipairs(bufs) do
			-- modify some fields here
			local pkt = buf:getUdpPacket()
			-- select a randomized source IP address
			-- you can also use a wrapping counter instead of random
			pkt.ip4.src:set(baseIP + math.random(NUM_FLOWS) - 1)
			-- you can modify other fields here (e.g. different source ports or destination addresses)
		end
		-- send packets
		bufs:offloadUdpChecksums()
		txCtr:updateWithSize(queue:send(bufs), size)
	end
	txCtr:finalize()
end

function counterSlave(queue, duration)
	-- the simplest way to count packets is by receiving them all
	-- an alternative would be using flow director to filter packets by port and use the queue statistics
	-- however, the current implementation is limited to filtering timestamp packets
	-- (changing this wouldn't be too complicated, have a look at filter.lua if you want to implement this)
	-- however, queue statistics are also not yet implemented and the DPDK abstraction is somewhat annoying
        local runtime = timer:new(duration)           -- Total Runtime
	local bufs = memory.bufArray()
	local ctrs = {}
	while mg.running() and runtime:running() do
		local rx = queue:recv(bufs)
		for i = 1, rx do
			local buf = bufs[i]
			local pkt = buf:getUdpPacket()
			local port = pkt.udp:getDstPort()
			local ctr = ctrs[port]
			if not ctr then
				ctr = stats:newPktRxCounter("Port " .. port, "plain")
				ctrs[port] = ctr
			end
			ctr:countPacket(buf)
		end
		-- update() on rxPktCounters must be called to print statistics periodically
		-- this is not done in countPacket() for performance reasons (needs to check timestamps)
		for k, v in pairs(ctrs) do
			v:update()
		end
		bufs:freeAll()
           
	end
        print( duration)
	for k, v in pairs(ctrs) do
		v:finalize()
	end
	-- TODO: check the queue's overflow counter to detect lost packets
end
