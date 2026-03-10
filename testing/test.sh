#!/bin/bash 

cp ../include/BytesBuffer.h . 
cp ../src/BytesBuffer.cpp . 

g++ -Wall main.cpp BytesBuffer.cpp && ./a.out