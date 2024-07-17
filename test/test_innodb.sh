# /bin/bash
export knob_profiler_root=/home/LLVM_MYSQL/knob_profiler
# export SchemaComponent=/home/LLVM_MYSQL/knob_profiler/baseline/example.cc
export EnvSchema=/home/LLVM_MYSQL/mysql-8.0.36/sql/mysqld.cc
rm -rf $knob_profiler_root/knob_profiler/var_discover/build
mkdir -p $knob_profiler_root/knob_profiler/var_discover/build
cd $knob_profiler_root/knob_profiler/var_discover/build
LLVM_DIR=/usr/local/opt/llvm cmake -DCMAKE_C_COMPILER=/usr/local/opt/llvm/bin/clang -DCMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++ -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
make -j 16
cd -

rm -rf r_mysqld
mkdir r_mysqld
cd r_mysqld
cwd=`pwd`

rm -rf /tmp/knob_profiler
mkdir -p /tmp/knob_profiler
chmod 777 /tmp/knob_profiler/

# 1. generate risky variables
# clang++ -std=c++17 -fno-stack-protector -g -flegacy-pass-manager -Xclang -load -Xclang /home/LLVM_MYSQL/knob_profiler/knob_profiler/var_discover/build/libKnobDependencyPass.so -pg -O0 -I/home/LLVM_MYSQL/mysql-8.0.36/include -I/home/LLVM_MYSQL/mysql-8.0.36/build/include -I/home/LLVM_MYSQL/mysql-8.0.36 -I/home/LLVM_MYSQL/mysql-8.0.36/build -I/home/LLVM_MYSQL/mysql-8.0.36/extra/rapidjson/include -c /home/LLVM_MYSQL/mysql-8.0.36/sql/sql_executor.cc -o sql_executor.o
# clang++ -std=c++17 -fno-stack-protector -g -flegacy-pass-manager -Xclang -load -Xclang /home/LLVM_MYSQL/knob_profiler/knob_profiler/var_discover/build/libKnobDependencyPass.so -pg -O0 -DMYSQL_SERVER -DMYSQL_SERVER  -DENABLED_DEBUG_SYNC -I/home/LLVM_MYSQL/mysql-8.0.36/include -I/home/LLVM_MYSQL/mysql-8.0.36/build/include -I/home/LLVM_MYSQL/mysql-8.0.36 -I/home/LLVM_MYSQL/mysql-8.0.36/build/ -I/home/LLVM_MYSQL/mysql-8.0.36/extra/rapidjson/include -c /home/LLVM_MYSQL/mysql-8.0.36/sql/sql_parse.cc -o sql_parse.o  
# clang++ -std=c++17 -fno-stack-protector -g -flegacy-pass-manager -Xclang -load -Xclang /home/LLVM_MYSQL/knob_profiler/knob_profiler/var_discover/build/libKnobDependencyPass.so -pg -O0 -DMUTEX_EVENT -I/home/LLVM_MYSQL/mysql-8.0.36/include -I/home/LLVM_MYSQL/mysql-8.0.36/build/include -I/home/LLVM_MYSQL/mysql-8.0.36 -I/home/LLVM_MYSQL/mysql-8.0.36/build/ -I/home/LLVM_MYSQL/mysql-8.0.36/extra/rapidjson/include  -I/home/LLVM_MYSQL/mysql-8.0.36/storage/innobase/include  -I/home/LLVM_MYSQL/mysql-8.0.36/storage/innobase -I/home/LLVM_MYSQL/mysql-8.0.36/sql -c /home/LLVM_MYSQL/mysql-8.0.36/storage/innobase/buf/buf0buf.cc -o buffer.o   
clang++ -std=c++17 -fno-stack-protector -g -flegacy-pass-manager -Xclang -load -Xclang /home/LLVM_MYSQL/knob_profiler/knob_profiler/var_discover/build/libKnobDependencyPass.so -pg -O0 -DMYSQL_SERVER  -DENABLED_DEBUG_SYNC -I/home/LLVM_MYSQL/mysql-8.0.36 -I/home/LLVM_MYSQL/mysql-8.0.36/include -I/home/LLVM_MYSQL/mysql-8.0.36/build/include  -I/home/LLVM_MYSQL/mysql-8.0.36/extra/rapidjson/include -I/home/LLVM_MYSQL/mysql-8.0.36/build/  -I/home/LLVM_MYSQL/mysql-8.0.36/extra/protobuf/protobuf-3.19.4/src  -I/home/LLVM_MYSQL/mysql-8.0.36/extra/icu/icu-release-73-1/source/common -c /home/LLVM_MYSQL/mysql-8.0.36/sql/mysqld.cc -o mysqld.o

cp /tmp/knob_profiler/schema.txt .
cp /tmp/knob_profiler/src2basicblock.txt .
cp /tmp/knob_profiler/executionPath.txt .
