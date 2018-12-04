#rm -fr build
cmake -H. -Bbuild
cmake --build build -- -j3
