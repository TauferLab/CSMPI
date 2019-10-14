for i in `seq 2 6`; do for j in `seq $i 7`; do diff <(cut -f$i seq.txt) <(cut -f$j seq.txt); done; done | sed '/^[<>]/!d' | sort -u | less
