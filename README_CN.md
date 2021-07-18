# soliditypp
Solidity++, a smart contract programming language that extends Ethereum Solidity by adding asynchronous semantics while maintains major compatibility.

## How to build
Run ```./build.sh``` to build the project.

## Run tests
Run ```./test.sh``` to run test cases.

## Quick start

## Project Architecture
soliditypp是基于solidity扩展而来，项目中会复用大部分solidity的代码，同时，对一些代码进行扩展和修改。

由于solidity项目本身并没有进行良好的面向对象设计，因而我们无法基于面向对象的方式，通过类的继承进行扩展。

如果直接fork solidity项目并做修改，后续merge原项目新的commit将非常困难，每次merge都要解决冲突。

因此，我们采用如下方式构建本项目：

* 将solidity原项目作为本项目的submodule，其中内容不做任何改动
* 仿照原项目，重新建立CMakeLists.txt。其中，未经改动的源文件直接引用submodule中的文件；需要改动的源文件复制到上层目录，并修改CMakeLists.txt。

## About cmake
本项目采用cmake构建。通常每个目录下都有一个CMakeLists.txt文件，如下所示：

```cmake
# 定义submodule中原始项目源文件路径
set(ORIGINAL_SOURCE_DIR ${PROJECT_SOURCE_DIR}/solidity/libevmasm)

# 指定本模块每个源文件的位置
set(sources
	${ORIGINAL_SOURCE_DIR}/Assembly.cpp # 表示直接使用原始项目的代码，位于submodule目录下
	Assembly.h # 表示该文件已复制到当前目录下并进行了修改，位于当前目录下

	${ORIGINAL_SOURCE_DIR}/AssemblyItem.cpp
	${ORIGINAL_SOURCE_DIR}/AssemblyItem.h
    # ...
)

# 指定头文件目录的顺序，优先在当前项目目录下寻找头文件，如未找到，再到submodule目录下寻找。这样可以保证本项目复制并修改的那些头文件优先生效。
include_directories(AFTER ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/solidity)

add_library(evmasm ${sources})
target_link_libraries(evmasm PUBLIC solutil)
```

## Tests
测试用例分为Automatic Tests和Interactive Tests两类。

### Automatic Tests
```test/solidity```目录下的测试用例用于测试 *Solidity* 的兼容性；

```test/soliditypp```目录下的测试用例用于测试对 *Solidity++* 新增特性的支持。

### Interactive Tests
交互测试工具的代码在```test/interactive```目录下。

语法相关的测试用例在```test/syntax```目录下。

其中，```test/syntax/solidity```目录下的测试用例用于测试 *Solidity* 的兼容性；

```test/syntax/soliditypp```目录下的测试用例用于测试对 *Solidity++* 新增特性的支持。

注：这个目录结构是在```test/Common.cpp```中hardcode的，如下：
```cpp
for (auto const& basePath: searchPath)
{
    fs::path syntaxTestPath = basePath / "syntax";
    std::cout << "Finding path to the syntax tests: " << syntaxTestPath << std::endl;
    if (fs::exists(syntaxTestPath) && fs::is_directory(syntaxTestPath)) {
        std::cout << "Found path to the syntax tests: " << syntaxTestPath << std::endl;
        return basePath;
    }
}
```

### 在Python中计算测试数据
#### blake2b
```python
import hashlib
hashlib.blake2b(b"abc", digest_size=32).hexdigest()
hashlib.blake2b(bytes([0] * 10), digest_size=32).hexdigest()
```