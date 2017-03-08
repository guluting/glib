/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "buffer.h"

#define DEFAULT_SIZE 1024

static void move_mem(buffer_t* _buf);

/**
 * 缓冲初始化函数
 * @param _buf 缓冲对象
 * @param _size 初始化大小
 * @return success:0
 */
int buffer_init(buffer_t *_buf, int _size) {
    assert(_buf != NULL && _size > 0);
    int count = _size / DEFAULT_SIZE;
    if (_size % DEFAULT_SIZE > 0) {
        ++count;
    }
    _size = count * DEFAULT_SIZE;
    if (pthread_mutex_init(&_buf->mutex, NULL) != 0) {
        return -1;
    }
    _buf->data_base = (unsigned char*) malloc(_size);
    if (_buf->data_base == NULL) {
        pthread_mutex_destroy(&_buf->mutex);
        return -1;
    }
    _buf->rd_ptr = 0;
    _buf->wt_ptr = 0;
    _buf->shutdown = 0;
    _buf->size = _size;
    return 0;
}

/**
 * 缓冲区内存整理(内部函数)
 * @param _buf 缓冲对象
 */
void move_mem(buffer_t* _buf) {
    int mv_size = _buf->wt_ptr - _buf->rd_ptr;
    if (mv_size == 0) {
        // 如果可读数据长度为0 证明所有数据已经全部读取完毕
        // 将读写指针归零 并清理缓存
        _buf->rd_ptr = 0;
        _buf->wt_ptr = 0;
        memset((void*) _buf->data_base, 0, _buf->size);
    } else {
        // 移动未读数据到基址处,并归零读指针,将写指针向前移动rd_ptr的距离
        memmove((void*) _buf->data_base,
                (void*) (_buf->data_base + _buf->rd_ptr),
                mv_size);
        _buf->wt_ptr = _buf->wt_ptr - _buf->rd_ptr;
        _buf->rd_ptr = 0;
        memset((void*) (_buf->data_base + _buf->wt_ptr), 0, _buf->size - _buf->wt_ptr);
    }
}

/**
 * 缓冲区写入数据
 * @param _buf 缓冲对象
 * @param _data 待写入数据
 * @param _size 待写入数据长度
 * @return success:0
 */
int buffer_write(buffer_t* _buf, unsigned char* _data, int _size) {
    assert(_buf != NULL && _data != NULL && _size > 0);
    int ret = -1;
    if (pthread_mutex_lock(&_buf->mutex) != 0) {
        return ret;
    }
    if (_buf->shutdown) {
        goto _err;
    }

    int wt_size = _buf->size - _buf->wt_ptr;
    if (wt_size < _size) {
        // 当前剩余内存无法容纳 _size 长度字节
        if ((_buf->rd_ptr + wt_size) >= _size) {
            /* 通过内部移动内存快来增加可写入空间 */
            move_mem(_buf);
        } else {
            // 扩大内存容量
            int count = (_size / DEFAULT_SIZE);
            if (_size % DEFAULT_SIZE > 0) {
                ++count;
            }
            int new_size = count * DEFAULT_SIZE + _buf->size;
            void* new_addr = (unsigned char*) realloc((void*) _buf->data_base, new_size);
            if (new_addr == NULL) {
                goto _err;
            }
            _buf->data_base = (unsigned char*) new_addr;
            _buf->size = new_size;
        }
    }
    memcpy((void*) (_buf->data_base + _buf->wt_ptr), (void*) _data, _size);
    _buf->wt_ptr += _size;
    assert(_buf->wt_ptr <= _buf->size);
    if (_buf->wt_ptr == _buf->size) {
        // 如果写入数据后无可写入空间 则先做内部移动
        move_mem(_buf);
        if (_buf->wt_ptr == _buf->size) {
            // 如果内部移动后任然无可写入空间 则分配更大的空间
            int new_size = DEFAULT_SIZE + _buf->size;
            void* new_addr = (unsigned char*) realloc((void*) _buf->data_base, new_size);
            if (new_addr == NULL) {
                goto _err;
                ;
            }
            _buf->data_base = (unsigned char*) new_addr;
            _buf->size = new_size;
        }
    }
    ret = 0;
_err:
    pthread_mutex_unlock(&_buf->mutex);
    return ret;
}

/**
 * 缓冲区读取数据
 * @param _buf 缓冲对象
 * @param _dest 读取数据存放地址
 * @param _size 读取数据长度
 * @return success:0
 */
int buffer_read(buffer_t* _buf, unsigned char* _dest, int _size) {
    assert(_buf != NULL && _dest != NULL && _size > 0);
    int ret = -1;
    if (pthread_mutex_lock(&_buf->mutex) != 0) {
        return ret;
    }
    if (_buf->shutdown) {
        goto _err;
    }
    if (_size > (_buf->wt_ptr - _buf->rd_ptr)) {
        // 如果读取长度大于可读长度,则直接放回
        goto _err;
    }
    memcpy((void*) _dest, (void*) (_buf->data_base + _buf->rd_ptr), _size);
    _buf->rd_ptr += _size;
    assert(_buf->rd_ptr <= _buf->wt_ptr);
    if (_buf->rd_ptr == _buf->wt_ptr) {
        // 数据读取完 如果无可读数据 则整理内存
        move_mem(_buf);
    }
    ret = 0;
_err:
    pthread_mutex_unlock(&_buf->mutex);
    return ret;
}

/**
 * 获取可写入字节数
 * @param _buf
 * @param _size 
 * @return success:not -1
 */
int buffer_wt_size(buffer_t* _buf) {
    assert(_buf != NULL);
    int ret = -1;
    if (pthread_mutex_lock(&_buf->mutex) != 0) {
        return ret;
    }
    if (_buf->shutdown) {
        goto _err;
    }
    ret = _buf->size - _buf->wt_ptr;
_err:
    pthread_mutex_unlock(&_buf->mutex);
    return ret;
}

/**
 * 获取当前写指针的位置
 * @param _buf 缓冲对象
 * @param _size 
 * @return success:not NULL
 */
unsigned char* buffer_wt_ptr(buffer_t* _buf, int _size) {
    assert(_buf != NULL && _size >= 0 && _size <= (_buf->size - _buf->wt_ptr));
    unsigned char* ret = NULL;
    if (pthread_mutex_lock(&_buf->mutex) != 0) {
        return ret;
    }
    if (_buf->shutdown) {
        goto _err;
    }
    ret = _buf->data_base + _buf->wt_ptr;
    _buf->wt_ptr += _size;
    assert(_buf->wt_ptr <= _buf->size);
    if (_buf->wt_ptr == _buf->size) {
        // 如果无可写空间 则进行内部内存移动
        move_mem(_buf);
    }
_err:
    pthread_mutex_unlock(&_buf->mutex);
    return ret;
}



/**
 * 释放缓冲区对象
 * @param _buf
 * @return success:0
 */
int buffer_free(buffer_t* _buf) {
    assert(_buf != NULL);
    int ret = -1;
    if(pthread_mutex_lock(&_buf->mutex) != 0){
        return ret;
    }
    if(_buf->shutdown){
        return ret;
    }
    _buf->shutdown = 1;
    pthread_mutex_unlock(&_buf->mutex);
    pthread_mutex_destroy(&_buf->mutex);
    free((void*)_buf->data_base);
    ret = 0;
    return ret;
}