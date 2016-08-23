all:
	mkdir -p bin
	cd bin && cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_STEREO=ON -DENABLE_VISGL=ON -DENABLE_MGL=ON -DENABLE_QR=ON
	cmake --build bin/

clean:
	rm -rf build bin lib include share
