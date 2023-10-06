export PATH=/mnt/d/software/cmake-3.27.6-linux-x86_64/bin:/mnt/d/Code/FFmpeg/build/lib:$PATH;
# export LD_LIBRARY_PATH=/mnt/d/Code/FFmpeg/build/lib:/mnt/d/Code/opencv/install/lib:$LD_LIBRARY_PATH;
echo "PATH=$PATH"
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
rm -rf build out
mkdir build out
cd ./build
cmake ..
make -j
cd ../out
./rtsp_demo
