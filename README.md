# soliditypp
Solidity++, a smart contract programming language that extends Ethereum Solidity by adding asynchronous semantics while maintains major compatibility.

## Get Started
```shell
git clone https://github.com/vitelabs/soliditypp.git
cd soliditypp
git submodule update --init --recursive
```

## How to build
Run ```./build.sh``` to build the project.

## Run tests
Run ```./test.sh``` to run test cases.

## Project Architecture
*soliditypp* is an extension of  [*solidity* project](https://github.com/ethereum/solidity). 
Most of the code from *solidity* will be reused in the project, and some codes will be extended and modified in *soliditypp*.

Since the original solidity project does not have a perfect object-oriented design, we cannot add functionalities easily by implementing interfaces or inheriting classes.

Therefore, we had to hack the original project and replace some class files to make the compiler recognize the new syntax.

If we make changes directly on the code base of the original project, it will be very difficult to resolve conflicts in subsequent merges with commits from the original project.

Therefore, we structure the project like this:

* Use the original solidity project as a submodule of this project, leave the project unchanged
* Recreate the cmake system similar to the original project.
Let the source files that are not need to be modified directly refer to the corresponding file path in the submodule, 
and the source files that need to be modified are copied to the upper directory, and specify the file path in CMakeLists.txt to the new location.

Below is an example of CMakeLists.txt：

```cmake
# The source directory of the original project from submodule
set(ORIGINAL_SOURCE_DIR ${PROJECT_SOURCE_DIR}/solidity/libevmasm)

# The source files
set(sources
    # Use the original source file from submodule    
	${ORIGINAL_SOURCE_DIR}/Assembly.cpp
    # Use the hacked source file in the current directory
    Assembly.h

	${ORIGINAL_SOURCE_DIR}/AssemblyItem.cpp
	${ORIGINAL_SOURCE_DIR}/AssemblyItem.h
    # ...
)

# From which directories to search for header files.
# NOTE: Order matters. 
# It's required to search for header files in the current directory before searching in the directory from submodule.
# Therefore a hacked header file will shadow the original one with the same name.
include_directories(AFTER ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/solidity)

add_library(evmasm ${sources})
target_link_libraries(evmasm PUBLIC solutil)
```

## Test
Test cases are divided into two categories: Automatic Tests and Interactive Tests.

### Automatic Tests
The test cases in the ```test/solidity``` directory are used to test the backward compatibility of *Solidity*;

The test cases in the ```test/soliditypp``` directory are used to test new features introduced by *Solidity++*.

### Interactive Tests
The code of the interactive testing tool is in the directory ```test/interactive```.

Syntax-related test cases are in the ```test/syntax``` directory.

Among them, the test cases in the ```test/syntax/solidity``` directory are used to test the backward compatibility of *Solidity*;

The test cases in the ```test/syntax/soliditypp``` directory are used to test new features introduced by *Solidity++*.

Note: This directory structure is hardcoded in ```test/Common.cpp```, as follows:
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

### Prepare Test Data using Python
#### blake2b
```python
import hashlib
hashlib.blake2b(b"abc", digest_size=32).hexdigest()
hashlib.blake2b(bytes([0] * 10), digest_size=32).hexdigest()
```
