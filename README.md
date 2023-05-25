### 编译运行步骤如下(Ubuntu 22.04.2 LTS)：

**1、切换到server目录下，执行make命令：**

~~~
make
~~~

编译生成可执行文件secondFileSystem。

**2、切换到client目录下，执行make命令**

~~~
make
~~~

编译生成可执行文件main

**3、运行secondFileSystem，启动服务器**

~~~
./secondFileSystem
~~~

**4、运行main，启动客户端**

~~~
./main 127.0.0.1 8888
~~~

**5、输入任意的用户名登录文件系统，输入--help获得命令帮助**


提供三个换入换出测试文件1.jpg, test.txt, demo.pdf


部分参考代码：https://github.com/fffqh/MultiUser-secondFileSystem