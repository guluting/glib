/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdlib.h>

#include "thpool.h"
#include <assert.h>
#include <sched.h>


// 任务数量
#define MAX_TASK_COUNT 65535
#define MIN_TASK_COUNT 64
// 线程数量
#define MAX_THREAD_COUNT 64
#define MIN_THREAD_COUNT 1


static void* thpool_word_thread(void* _thpool);
static void thpool_clean(thpool_t* _thpool);

/**
 * 初始化线程池
 * @param _thpool
 * @param _size
 * @return success:0
 */
int thpool_init(thpool_t* _thpool, int _threads, int _tasks) {
    assert(_thpool != NULL && _threads > 0 && _tasks > 0);

    int ret = -1;
    // 线程数量和任务数量检测
    _threads = _threads < MIN_THREAD_COUNT ? MIN_THREAD_COUNT : _threads;
    _threads = _threads > MAX_THREAD_COUNT ? MAX_THREAD_COUNT : _threads;
    _tasks = _tasks < MIN_TASK_COUNT ? MIN_TASK_COUNT : _tasks;
    _tasks = _tasks > MAX_TASK_COUNT ? MAX_TASK_COUNT : _tasks;

    // 初始化任务队列
    _thpool->task_queue = (task_t*) malloc(_tasks * sizeof (task_t));
    // 初始化线程
    _thpool->thread_pool = (pthread_t*) malloc(_threads * sizeof (pthread_t));
    // 初始化信号和锁
    if (pthread_mutex_init(&_thpool->mutex, NULL) != 0 ||
            pthread_cond_init(&_thpool->signal, NULL) != 0 ||
            _thpool->task_queue == NULL || _thpool->thread_pool == NULL) {
        goto _err;
    }
    // 创建工作者线程
    _thpool->thread_start_count = 0;
    for (int i = 0; i < _threads; i++) {
        if (pthread_create(&(_thpool->thread_pool[i]), NULL, thpool_word_thread, (void*) _thpool) != 0) {
            goto _err;
        }
        ++_thpool->thread_start_count;
    }
    _thpool->task_head = 0;
    _thpool->task_tail = 0;
    _thpool->task_size = _tasks;
    _thpool->task_count = 0;
    _thpool->thread_size = _threads;
    _thpool->shutdown = 0;
    ret = 0;
_err:
    printf("thread start count:%d\n", _thpool->thread_start_count);
    return ret;
}

/**
 * 工作者线程(内部函数)
 * @param thpool
 */
void* thpool_word_thread(void* _thpool) {
    thpool_t* thpool = (thpool_t*) _thpool;
    task_t task;
    for (;;) {
        pthread_mutex_lock(&thpool->mutex);
        while ((thpool->task_count == 0) && (!thpool->shutdown)) {
            // 当任务量为0 并且 shutdown为0时 进入等待
            pthread_cond_wait(&thpool->signal, &thpool->mutex);
        }
        if (thpool->shutdown && thpool->task_count == 0) {
            // 如果线程池销毁 并且任务量为0 则退出工作循环
            break;
        }
        task.func = thpool->task_queue[thpool->task_head].func;
        task.arg = thpool->task_queue[thpool->task_head].arg;
        thpool->task_head = (thpool->task_head + 1) % thpool->task_size;
        --thpool->task_count;
        pthread_mutex_unlock(&thpool->mutex);
        sched_yield();
        // 执行任务
        (*(task.func))(task.arg);
    }
    --thpool->thread_start_count;
    printf("thpool threads %d\n",thpool->thread_start_count);
    pthread_mutex_unlock(&thpool->mutex);
    pthread_exit(NULL);
    return NULL;
}

/**
 * 添加任务
 * @param _thpool
 * @param func 函数地址
 * @param arg 函数参数
 * @return success:0
 */
int thpool_add(thpool_t* _thpool, void (*func)(void*), void* arg) {
    assert(_thpool != NULL && func != NULL);
    int ret = -1;
    int tail;
    if(pthread_mutex_lock(&_thpool->mutex) != 0){
        return ret;
    }
    tail = (_thpool->task_tail + 1) % _thpool->task_size;
    if (_thpool->task_count == _thpool->task_size) {
        // 任务队列满
        // assert(_thpool->task_count < _thpool->task_size);
        goto _err;
    }
    if (_thpool->shutdown) {
        // 线程池已关闭
        goto _err;
    }
    _thpool->task_queue[_thpool->task_tail].func = func;
    _thpool->task_queue[_thpool->task_tail].arg = arg;
    _thpool->task_tail = tail;
    ++_thpool->task_count;
    pthread_cond_signal(&_thpool->signal);
    ret = 0;
_err:
    pthread_mutex_unlock(&_thpool->mutex);
    sched_yield();
    return ret;
}

/**
 * 释放线程池
 * @param _thpool
 * @return success:0
 */
int thpool_free(thpool_t* _thpool) {
    assert(_thpool != NULL);
    int ret = -1;
    
    if(pthread_mutex_lock(&_thpool->mutex) !=0 ){
        return ret;
    }
    if (_thpool->shutdown) {
        // 如果线程池已经停止 则直接解锁返回
        pthread_mutex_unlock(&_thpool->mutex);
        return ret;
    }
    _thpool->shutdown = 1;
    // 发送广播并解锁
    if(pthread_cond_broadcast(&_thpool->signal) != 0 ||
            pthread_mutex_unlock(&_thpool->mutex) != 0) {
        return ret;
    }
    // 等待线程退出
    for(int i=0;i<_thpool->thread_start_count;i++){
        if(pthread_join(_thpool->thread_pool[i],NULL) != 0){
            return ret;
        }
    }
    thpool_clean(_thpool);
    ret = 0;
    return ret;
}

/**
 * 回收资源(内部函数)
 * @param _thpool
 */
void thpool_clean(thpool_t* _thpool) {
    assert(_thpool != NULL);
    if (_thpool->thread_pool) {
        free(_thpool->task_queue);
        free(_thpool->thread_pool);
    }
    pthread_mutex_destroy(&_thpool->mutex);
    pthread_cond_destroy(&_thpool->signal);
    printf("thpool_clean success\n");
    return;
}