for i in 0 1 2 3 4 5 6 7 21 22 23 24 25 26 27 28
do
  gpio mode $i out
  gpio write $i 1
done
sleep 4
for i in 0 1 2 3 4 5 6 7 21 22 23 24 25 26 27 28
do
  gpio write $i 0
done
sleep 2
for i in 0 1 2 3 4 5 6 7 21 22 23 24 25 26 27 28
do
  gpio write $i 1
done
sleep 2
for delay in .5 .25 .125 .0625 .03125 .015625 .0078125 .00390625
do
  for i in 0 1 2 3 4 5 6 7 21 22 23 24 25 26 27 28
  do
    gpio write $i 0
    sleep $delay
    gpio write $i 1
    sleep $delay
  done
done