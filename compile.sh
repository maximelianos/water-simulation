mkdir build
cd build
rm -r ./*; cmake .. && make && cp ../{granite.bmp,ball.bmp,sky.bmp} . && ./main
