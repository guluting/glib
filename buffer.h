/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   buffer.h
 * Author: ghosthand
 *
 * Created on 2017年3月6日, 下午2:01
 */

#ifndef BUFFER_H
#define BUFFER_H
#include <pthread.h>


#ifdef __cplusplus
extern "C" {
#endif

    typedef struct _buffer_t {
        pthread_mutex_t mutex; // 同步锁
        int size; // 总长度
        int wt_ptr; // 写指针
        int rd_ptr; // 读指针
        unsigned char* data_base; // 数据指针
        int shutdown; // 
    } buffer_t;

    /**
     * 缓冲初始化函数
     * @param _buf 缓冲对象
     * @param _size 初始化大小
     * @return success:0
     */
    int buffer_init(buffer_t* _buf, int _size);



    /**
     * 缓冲区写入数据
     * @param _buf 缓冲对象
     * @param _data 待写入数据
     * @param _size 待写入数据长度
     * @return success:0
     */
    int buffer_write(buffer_t* _buf, unsigned char* _data, int _size);



    /**
     * 缓冲区读取数据
     * @param _buf 缓冲对象
     * @param _dest 读取数据存放地址
     * @param _size 读取数据长度
     * @return success:0
     */
    int buffer_read(buffer_t* _buf, unsigned char* _dest, int _size);



    /**
     * 获取可写入字节数
     * @param _buf
     * @param _size 
     * @return success:not -1
     */
    int buffer_wt_size(buffer_t* _buf);



    /**
     * 获取当前写指针的位置
     * @param _buf 缓冲对象
     * @param _size 
     * @return success:not NULL
     */
    unsigned char* buffer_wt_ptr(buffer_t* _buf, int _size);




    /**
     * 释放缓冲区对象
     * @param _buf
     * @return success:0
     */
    int buffer_free(buffer_t* _buf);

#ifdef __cplusplus
}
#endif

#endif /* BUFFER_H */

