## C语言的fread和fwrite
---  

- 作用
    
    - fread：从已打开的 FILE* 流中读取二进制字节到内存缓冲区。
    - fwrite：把内存缓冲区的字节写入已打开的 FILE* 流（通常是文件或二进制流）。
- 原型
    
    - size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
    - size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
- 参数含义（两者相同的前 3 项）
    
    - ptr / const void *ptr
        - fread：目标缓冲区地址，读取的数据写入到这里。
        - fwrite：源缓冲区地址，要从这里读字节并写到流中。
    - size
        - 每个“元素”的字节数（例如 sizeof(int) 或 sizeof(MyStruct)）。
    - nmemb
        - 要读/写的元素数量。
    - stream
        - FILE*，必须是用 fopen 等打开的文件流；以适当模式打开（可读/可写，如 "r","rb","w","wb","a"）。
- 返回值
    
    - 返回成功读/写的元素个数（不是字节数）。
    - 实际读/写的字节数 = 返回值 * size。
    - 若返回 < nmemb，可能到达 EOF（fread）或发生错误（ferror），都应检查 feof() / ferror()。
- 示例：读取与写入（简单）
    
    - 写一个结构体： FILE _f = fopen("out.bin","wb"); MyStruct s = ...; if (fwrite(&s, sizeof(s), 1, f) != 1) { /_ 处理错误 */ } fclose(f);
        
    - 读文件全部到缓冲区（按字节）： FILE *f = fopen("in.bin","rb"); if (!f) ...; fseek(f, 0, SEEK_END); long size = ftell(f); rewind(f); char *buf = malloc(size); size_t read = fread(buf, 1, size, f); // size=1, nmemb=size -> 返回读到的字节数 if (read != size) { if (feof(f)) {...} if (ferror(f)) {...} } fclose(f);
        
- 处理部分写入（对于某些非阻塞流或底层中断场景）
    
    - 虽然对常规磁盘文件 fwrite 一般会写完，但在套接字 / 管道 / 非阻塞流上可能写入较少元素，建议循环写直到全部写完： size_t total = 0; size_t to_write = nbytes; const char _p = buf; while (to_write > 0) { size_t written = fwrite(p, 1, to_write, f); if (written == 0) { if (ferror(f)) { /_ 错误处理 _/ break; } /_ 若是暂时无写入，可能需要重试/等待 */ } to_write -= written; p += written; total += written; }
- 注意点
    
    - fread/fwrite 是按原始字节操作（binary-safe）。
    - 对文本数据，注意不同平台换行转换（以二进制模式 "rb"/"wb" 可避免转换）。
    - 调用 fflush(stream) 或 fclose() 可将缓冲区刷到磁盘。若需要强制持久化到磁盘使用 fsync(fileno(stream))。
    - 对于结构体的直接写入，跨平台（字节序、对齐）可能不兼容。
    - 当 size=1 时，返回值等于字节数，通常更直观。