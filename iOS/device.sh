[ -d _build ] || mkdir _build
pushd _build
cmake -GXcode ../../src -DCMAKE_TOOLCHAIN_FILE=../ios-cmake/toolchain/iOS.cmake -DIOS_PLATFORM=OS -DIOS=1
popd

