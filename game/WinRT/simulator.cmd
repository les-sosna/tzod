mkdir _build
pushd _build
	mkdir data
	pushd data
		mklink /J data ..\..\..\data
	popd	
	cmake .. -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0 -DWINRT=1
popd
