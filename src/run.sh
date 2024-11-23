make clean
rm libq.my_debug_out.res
make debug
./sim -mode 2 -L2sizeKb 1024 ../traces/libq.mtr.gz | head -n1000000 > libq.my_debug_out.res