mkdir _build
pushd _build
cmake ../../src -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0
popd
