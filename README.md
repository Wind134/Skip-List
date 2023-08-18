# C++实现的跳表(Skip List)
这是一个基于跳表（Skip List）实现的一个轻量级键值型存储引擎；

## 使用方法
在源码目录中编译：`g++ -o ../build/output main.cpp`
- 确保你的上级目录中build文件夹存在，否则会报错；

我的main主程序中设定从文件中读取数据，然后将数据存储至引擎，因此需要在可执行文件后加上要读取的文件路径，这部分可以自定义；

运行举例：`../build.output ../store/data`
- 根据实际目录自行修改；


## 原理
已将具体的原理解析上传至个人博客；

详细的实现原理请移步[个人博客](https://wind134.github.io/posts/Redis_skiplist/)；
