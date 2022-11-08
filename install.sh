#!/bin/bash

#获取项目绝对路径
current_path=$(cd "$(dirname $0)";pwd)

if [ ! -d "${current_path}/build" ]; then
    mkdir ${current_path}/build/
fi

cd ${current_path}/build
rm -rf ./*
cmake ..
make

cd ..
pwd
# if [ ! -d /usr/include/mymuduo ]; then
#     mkdir /usr/include/mymuduo
# fi

# for header in `ls ./include/*.h`
#     cp $header /usr/include/mymuduo
# fi

# cp ./lib/libmymuduo.so /usr/lib

# ldconfig