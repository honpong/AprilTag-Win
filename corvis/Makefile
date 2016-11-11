all:
	mkdir -p bin
	cd bin && cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_VISGL=ON -DENABLE_MGL=ON
	cd bin && $(MAKE) -j7

clean:
	rm -rf build bin lib include share
