local mg     = require "moongen"
local memory = require "memory"
local device = require "device"
local ts     = require "timestamping"
local stats  = require "stats"
local hist   = require "histogram"
local timer  = require "timer"
local log    = require "log"

local PKT_SIZE = 512
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
	parser:option("-s --size", "Size of the packets in bytes."):default(100):convert(tonumber)
	parser:option("-d --duration", "Duration of the expriment in seconds."):default(10):convert(tonumber)
	parser:option("-f --file", "Filename of the latency histogram."):default("histogram.csv")
end

function master(args)
	local dev0 = device.config({port = args.dev0, rxQueues = 2, txQueues = 1})
	local dev1 = device.config({port = args.dev1, rxQueues = 1, txQueues = 2})
	device.waitForLinks()
        
	mg.startTaskOnCore(loadSlave_CPUID, "loadSlave", dev0:getTxQueue(0), args.size, args.duration)
	mg.startTaskOnCore(timestamp_CPUID, "timestamper", dev1:getTxQueue(0), dev0:getRxQueue(0), args.size, args.duration, args.file):wait()
	mg.waitForTasks()
	mg.stop()
end


function timestamper(txQueue, rxQueue, size, duration, fl)
        local runtime = timer:new(duration)           -- Total Runtime
	local rateLimit = timer:new(0.001)      -- Iter-transmission delay
        local hist = hist:new()
        local timestamper
	file = io.open("fl", "w")
	io.output(file)     
	timestamper = ts:newTimestamper(txQueue, rxQueue)
        while mg.running() and runtime:running() do
                local lat = timestamper:measureLatency( size, function(buf)
                        buf:getEthernetPacket():fill{
                                ethSrc = ETH_SRC,
			        ethDst = ETH_DST,
			        ethType = 0x88F7
		        }       
         end )
                hist:update(lat)
		rateLimit:wait()
		rateLimit:reset()
        end
        hist:print()
        hist:save(file)
	io.close(file)
        if hist.numSamples == 0 then
                print("Received no packets.")
        end
end

function loadSlave(queue, size, duration)
        local runtime = timer:new(duration)           -- Total Runtime
	local mem = memory.createMemPool(function(buf)
		buf:getEthernetPacket():fill{
			ethSrc = ETH_SRC,
			ethDst = ETH_DST,
			ethType = 0x1234
		}
	end)
	local bufs = mem:bufArray()
	while mg.running() and runtime:running() do
		bufs:alloc(size)
		queue:send(bufs)
	end
end

