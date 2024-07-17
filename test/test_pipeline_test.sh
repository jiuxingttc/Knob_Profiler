# /bin/bash
export knob_profiler_root=/home/LLVM_MYSQL/knob_profiler
# export SchemaComponent=/home/LLVM_MYSQL/knob_profiler/baseline/example.cc
export EnvSchema=/home/LLVM_MYSQL/knob_profiler/baseline/example.cc
rm -rf $knob_profiler_root/knob_profiler/var_discover/build
mkdir -p $knob_profiler_root/knob_profiler/var_discover/build
cd $knob_profiler_root/knob_profiler/var_discover/build
LLVM_DIR=/usr/local/opt/llvm cmake -DCMAKE_C_COMPILER=/usr/local/opt/llvm/bin/clang -DCMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++ -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
make -j 16
cd -

rm -rf r
mkdir r
cd r
cwd=`pwd`

rm -rf /tmp/knob_profiler
mkdir -p /tmp/knob_profiler
chmod 777 /tmp/knob_profiler/

# 1. generate risky variables
clang++ -fno-stack-protector -g -flegacy-pass-manager -Xclang -load -Xclang /home/LLVM_MYSQL/knob_profiler/knob_profiler/var_discover/build/libKnobDependencyPass.so -pg -O0 /home/LLVM_MYSQL/knob_profiler/baseline/example.cc -o example

cp /tmp/knob_profiler/schema.txt .
cp /tmp/knob_profiler/src2basicblock.txt .
cp /tmp/knob_profiler/executionPath.txt .
