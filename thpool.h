/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   thpool.h
 * Author: ghosthand
 *
 * Created on 2017年3月7日, 上午10:38
 */

#ifndef THPOOL_H
#define THPOOL_H
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

    // 任务结构体

    typedef struct _task_t {
        void (*func)(void*);
        void* arg;
    } task_t;

    // 线程池结构体

    typedef struct _thpool_t {
        pthread_mutex_t mutex;
        pthread_cond_t signal;
        task_t* task_queue; // 任务队列
        int task_size; // 队列总长度
        int task_head; // 队列头指针
        int task_tail; // 队列尾指针
        int task_count; // 当前任务数
        pthread_t* thread_pool;
        int thread_size;
        int thread_start_count;
        int shutdown;
    } thpool_t;

    /**
     * 初始化线程池
     * @param _thpool
     * @param _size
     * @return success:0
     */
    int thpool_init(thpool_t* _thpool, int _threads, int _tasks);



    /**
     * 添加任务
     * @param _thpool
     * @param func 函数地址
     * @param arg 函数参数
     * @return success:0
     */
    int thpool_add(thpool_t* _thpool, void (*func)(void*), void* arg);


    /**
     * 释放线程池
     * @param _thpool
     * @return success:0
     */
    int thpool_free(thpool_t* _thpool);




#ifdef __cplusplus
}
#endif

#endif /* THPOOL_H */

