local mg     = require "moongen"
local memory = require "memory"
local device = require "device"
local ts     = require "timestamping"
local stats  = require "stats"
local hist   = require "histogram"
local timer  = require "timer"

local PKT_SIZE = 60
local ETH_DST = "3c:fd:fe:ad:76:c8"
local ETH_SRC = "3c:fd:fe:ad:74:e4"
local SRC_IP = "10.0.0.7"
local DST_IP = "10.0.0.10"
local SRC_PORT = 319
local DST_PORT = 319

function configure(parser)
	parser:description("generate traffic and measure the latency.")
	parser:argument("dev0", "Device to recv from"):convert(tonumber)
	parser:argument("dev1", "Device to transmit from"):convert(tonumber)
	parser:option("-r --rate", "Transmit rate in Mbit/s"):default(10000):convert(tonumber)
	parser:option("-s --size", "Size of the packets in bytes."):default(100):convert(tonumber)
	parser:option("-f --file", "Filename of the latency histogram."):default("histogram.csv")
end

function master(args)
	local dev0 = device.config({port = args.dev0, rxQueues = 2, txQueues = 2})
	local dev1 = device.config({port = args.dev1, rxQueues = 2, txQueues = 2})
	device.waitForLinks()
	device.waitForLinks()

	--dev:getTxQueue(0):setRate(args.rate - (args.size + 4) * 8 / 1000)
	dev1:getTxQueue(0):setRate(args.rate)
	--stats.startStatsTask{dev}
	--mg.startTask("loadSlave", dev1:getTxQueue(1), dev0:getRxQueue(1), args.size)
	--mg.startSharedTask("timerSlave", dev1:getTxQueue(0), dev0:getRxQueue(0), args.file )
	--mg.startTask("timerSlave", dev1:getTxQueue(0), dev0:getRxQueue(0), args.size, args.file)
	mg.startTask("timestamper", dev1:getTxQueue(0), dev0:getRxQueue(0), args.size):wait()
	mg.waitForTasks()
	mg.stop()
end

local function fillUdpPacket(buf, len)
        buf:getUdpPacket():fill{
                ethSrc = ETH_SRC,
                ethDst = ETH_DST,
                ip4Src = SRC_IP,
                ip4Dst = DST_IP,
                --udpSrc = SRC_PORT,
                --udpDst = DST_PORT,
                pktLength = len
        }
end

local function fillTcpPacket(buf, len)
	buf:getTcpPacket():fill{
		ethSrc = ETH_SRC,
		ethDst = ETH_DST,
                ip4Src = SRC_IP,
                ip4Dst = DST_IP,
		--tcpSrc = SRC_PORT,
		--tcpDst = DST_PORT,
		pktLength = len
	}
end


function loadSlave(txQueue, rxQueue, size)
        local mem = memory.createMemPool(function(buf)
		buf:getEthernetPacket().eth.dst:setString(ETH_DST)
		--fillTcpPacket(buf, size)
                --[[buf:getEthernetPacket():fill{
                        --ethSrc = txDev,
                        ethSrc = ETH_SRC,
                        ethDst = ETH_DST,
                        ethType = 0x1234
                }--]]
        end)
        local bufs = mem:bufArray()
	local tim = timer:new(200000)
	local txCtr = stats:newDevTxCounter(txQueue, "plain")
	local rxCtr = stats:newDevRxCounter(rxQueue, "plain")
	local loadhist = hist:new()

	mg.sleepMillis(5000)
        while mg.running() and tim:running() do
                bufs:alloc(size)
                txQueue:send(bufs)
		txCtr:update()
		rxCtr:update()
		loadhist:update(txCtr)
		loadhist:update(rxCtr)
        end
	txCtr:finalize()
	rxCtr:finalize()
	--loadhist:print()
	--loadhist:save('myhist.csv')
end

function timerSlave(txQueue, rxQueue, size, histfile)
	local timestamper = ts:newUdpTimestamper(txQueue, rxQueue)
	local hist = hist:new()
	local rateLimit = timer:new(1)
	local timeout = timer:new(4)
			
	mg.sleepMillis(6000) -- make sure other tasks are running
	timeout:reset()
	while mg.running() and timeout:running() do
		--timestamper:measureLatency(function(buf) buf:getEthernetPacket().eth.dst:setString(ETH_DST) end)
		hist:update(timestamper:measureLatency(function(buf) 
			--buf:getEthernetPacket().eth.dst:setString(ETH_DST)
			--fillUdpPacket(buf, size)
			--fillTcpPacket(buf, size)
			--local pkt = buf:getUdpPacket()
			--pkt.ip4.src:set(src_ip)
			--pkt.ip4.dst:set(src_ip)
			--pkt:setSrc()
			
			--local pkt = buf:getEthernetPacket()
			--pkt.eth.dst:setString(ETH_DST) 
		end))
		rateLimit:busyWait()
		rateLimit:reset()
		hist:update(timestamper:measureLatency(function(buf) buf:getEthernetPacket().eth.dst:setString(ETH_DST) end))
	end
	hist:print()
	--hist:save('latency.txt')
end

function timestamper(txQueue, rxQueue, size)
        --local filter = rxQueue.qid ~= 0
        --log:info("Testing timestamping %s %s rx filtering for %d seconds.",
        --        udp and "UDP packets to port " .. udp or "L2 PTP packets",
          --      filter and "with" or "without",
          --      RUN_TIME
        --)
        --if randomSrc then
        --        log:info("Using multiple flows, this can be slower on some NICs.")
        --end
        --if vlan then
        --        log:info("Adding VLAN tag, this is not supported on some NICs.")
        --end
        local runtime = timer:new(3)
	local rateLimit = timer:new(0.001)
        local hist = hist:new()
        local timestamper
	file = io.open("results", "w")
	io.output(file)

                
	timestamper = ts:newTimestamper(txQueue, rxQueue)
        while mg.running() and runtime:running() do
                local lat = timestamper:measureLatency(function(buf)
			--fillUdpPacket(buf, size)
			--buf:getUdpPacket().udp:setSrcPort(math.random(1, 1000))
			--buf:getEthernetPacket().eth.dst:setString(ETH_DST)
			--pkt.eth.src.setString(ETH_SRC)
                end)
                hist:update(lat)
		--print(lat)
		--io.write(lat,"\n")
		rateLimit:busyWait()
		rateLimit:reset()
        end
        hist:print()
	io.close(file)
        --if hist.numSamples == 0 then
        --        log:error("Received no packets.")
        --end
        print()
end

