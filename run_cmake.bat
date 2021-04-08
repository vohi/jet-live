call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
cd build\
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=C:\\work\\build_qtbase\\lib\\cmake ..
ninja -v all > build.log
cd ..
