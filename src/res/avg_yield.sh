grep '3: yi' | awk '{ sum += $4 } END { print sum/NR}'
