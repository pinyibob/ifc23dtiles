# bim23dtiles项目的cmake构建

## 运行方法
1. 执行mkdir build, 创建构建目录并cd build;
2. 执行cmake -G 查看编译工具提示，选取编译工具，例如 "Visual Studio 17 2022"
3. 执行命令 cmake -G "Visual Studio 17 2022" -Ddebug=ON ..
    这句命令的意思是选用 vs17工具链， 开启debug编译模式， 编译代码根目录的CMakeLists.txt
4. 执行 cmake --build . 即可编译当前目录，如果是msvc当然也可以打开sln，使用ide进行编译；

注意特殊库的文件依赖：
cgal库需要依赖编译出的 proj、gdal文件夹，他们默认安装在/usr/share/中，部署时，请拷贝到环境变量覆盖的目录
