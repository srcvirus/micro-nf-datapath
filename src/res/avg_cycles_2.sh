grep '2: av' | awk '{ sum += $3} END { print sum/NR}' 
