// File: Svg2Bmp.hh
// Project: CS 375 Project 2
// Author: Cole Nicholson-Rubidoux
// Description: This file contains additional private variables
//	and function prototypes for the Svg2Bmp class 

#define VERBOSE true

void fillShape (bitmap_image & bmp, bitmap_image & fillbmp, Point p, int red, int green, int blue, int width);
void fillPath (bitmap_image & bmp,  int red, int green, int blue);
void Fill (bitmap_image & bmp, vector<Point>& filledPoints, int currentX, int currentY, int fillRed, int fillGreen, int fillBlue);
void FillBase(bitmap_image& bmp, vector<Point>& filledPoints, int currentX, int currentY, int fillRed, int fillGreen, int fillBlue, rgb_t targetColor);
void transformPath ();
