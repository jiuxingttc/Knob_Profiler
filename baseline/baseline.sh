# /bin/bash
/usr/local/opt/llvm/clang++ -fno-stack-protector -pg  -O0 example.cpp -o example
time ./example 10
gprof ./example > example_baseline.txt

rm ./gmon.out
time ./example 1
gprof ./example > example_compare_baseline.txt