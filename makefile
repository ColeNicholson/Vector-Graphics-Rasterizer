SVG2BMP.out : Driver.o Svg2Bmp.o
	g++ -o SVG2BMP.out Driver.o Svg2Bmp.o

SVG2BMPD.out : Driver.o Svg2BmpD.o
	g++ -o SVG2BMPD Driver.o Svg2BmpD.o

Driver.o : DriverFile.cpp Svg2Bmp.h
	g++ -std=c++11 -g -c DriverFile.cpp

Svg2Bmp.o : Svg2Bmp.cpp Svg2Bmp.h Svg2Bmp.hh Svg2Bmp.hpp Bitmap_Image.hpp
	g++ -std=c++11 -g -c Svg2Bmp.cpp -DDEBUG=false

Svg2BmpD.o : Svg2Bmp.cpp Svg2Bmp.h Svg2Bmp.hh Svg2Bmp.hpp Bitmap_Image.hpp
	g++ -std=c++11 -g -c Svg2Bmp.cpp -DDEBUG=true -o Svg2BmpD.o

clean :
	rm -rf SVG2BMP*.out *.o *.gch *~
