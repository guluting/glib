/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: ghosthand
 *
 * Created on 2017年3月6日, 下午1:59
 */

#include <cstdlib>
#include <string.h>
#include "buffer.h"
#include "thpool.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>


using namespace std;


void test(void* i){
    //printf("%d\n",(int)i);
}


void testBuffer(){
    unsigned char data[10];
    memset((void*)data,'A',10);
    buffer_t buff;
    buffer_init(&buff,5);
    buffer_write(&buff,data,10);
    unsigned char buf[2];
    
    for(int i=0;i<10;i++){
        buffer_read(&buff,buf,1);
        buf[1]='\0';
        printf("%c\n", (char)buf[0]);
        printf("free mem:%d\n",buffer_wt_size(&buff));
    }
    buffer_free(&buff);
}


void testThreadPool(){
    thpool_t thpool;
    thpool_init(&thpool,8,1000);
    for(int i=0;i<10;i++){
        thpool_add(&thpool,test,(void*)i);
    }
    thpool_free(&thpool);
}

int main(int argc, char** argv) {
    testBuffer();
    testThreadPool();
    getchar();
    return 0;
}

