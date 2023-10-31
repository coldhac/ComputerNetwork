# ComputerNetwork
北京大学计算机网络Lab作业

计网的查重真的很严格，可以参考，切勿抄袭！！

## Lab1: MyFTP
主要可以分为以下几部分：
+ `limit.hpp`: 定义了一些常量限制，如文件名长度限制，文件大小限制等；
+ `header.hpp`: 定义了`header`报文结构体，定义了文档中报文常量，并声明了比较函数；
+ `sockall.hpp`: 定义了新的`recv`和`send`函数，避免出现缓冲区过小而无法传输大文件的问题；
+ `ftp_client.cpp`: 主要处理指令的分类，和客户端具体功能实现，额外补充了指令的帮助文档，通过`help`查看；
+ `ftp_server.cpp`: 主要处理报文的分类，和服务端具体功能实现，额外补充了对参数的检查，使用非法参数会退出程序。

虽然测试数据保证一定合法，但代码中还是实现了对各种意外情况的判断，错误发生时会使用标准错误流输出提示。
