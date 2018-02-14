package.path = package.path ..";?.lua;test/?.lua;app/?.lua;"
pktgen.screen("on");
pktgen.pause("Screen on\n", 2000);

pktgen.set_ipaddr("0", "src", "10.10.0.7/24");
pktgen.set_ipaddr("0", "dst", "10.10.0.10");
pktgen.set_mac("0", "3c:fd:fe:ad:76:c8");

local function sendPls(sendRate, pktSize, seconds)
    file = io.open("results", "w")
    io.output(file)
    pktgen.set("0", "rate", sendRate);
    pktgen.set("0", "size", pktSize);


    pktgen.start("0");
    --printf("After start. delay for %d seconds\n", durationMs);
    pktgen.delay(5000);
    --prints("pktStats", pktgen.pktStats("all"));
    --prints("portRates", pktgen.portStats("all", "rate"));
    for i = 1,seconds,1
    do
        io.write(pktgen.portStats("all","rate")[1]["pkts_rx"],",",pktgen.portStats("all","rate")[1]["mbits_rx"],"\n");
	pktgen.delay(1000);
        
    end

    pktgen.stop("all");

    pktgen.clear("all");
--    pktgen.cls();
    pktgen.reset("all");
    io.close(file);
end


rate = 100; size = 200; seconds = 60;
printf("Runnning at rate %d, size %d, for %d secs\n", rate, size, seconds);
sendPls(rate, size, seconds);
io.output(io.stdout);
printf("done\n");
