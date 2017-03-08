# glib
Buffer &amp; ThreadPool

glib 运行编译环境:Linux

buffer提供了连续地址的缓存功能，当无更多缓存时能够通过内部内存整理及重新分配更多的内存来确保数据的正常存储。

示例代码



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



thpool实现了线程池的功能，能够自定义任务队列长度及工作线程数量。

示例代码

    thpool_t thpool;
    thpool_init(&thpool,8,1000);
    for(int i=0;i<10;i++){
        thpool_add(&thpool,function_ptr,(void*)arg);
    }
    thpool_free(&thpool);
    
    
