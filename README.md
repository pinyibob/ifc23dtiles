# bim23dtiles项目的cmake构建

## 运行方法
1. 执行mkdir build, 创建构建目录并cd build;
2. 执行cmake -G 查看编译工具提示，选取编译工具，例如 "Visual Studio 17 2022"
3. 执行命令 cmake -G "Visual Studio 17 2022" -Ddebug=ON ..
    这句命令的意思是选用 vs17工具链， 开启debug编译模式， 编译代码根目录的CMakeLists.txt
4. 执行 cmake --build . 即可编译当前目录，如果是msvc当然也可以打开sln，使用ide进行编译；

注意特殊库的文件依赖：
cgal库需要依赖编译出的 proj、gdal文件夹，他们默认安装在/usr/share/中，部署时，请拷贝到环境变量覆盖的目录((这段话的意义已经不记得了,似乎是gdal编译时依赖安装的proj.db文件))

目前只有mongolib(libbson), osg相关库, freeimage没有使用cmake管理, 请参考编译文档使用linux安装包自行安装(清华源即可).

可执行程序启动命令
--task=/mnt/e/zpy/bimtask.json 

测试数据
f1.ifc
