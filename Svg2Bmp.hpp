// File: Svg2Bmp.hpp
// Project: CS 375 Project 2
// Author: Cole Nicholson-Rubidoux
// Description: This file contains the implementations of the functions
//	prototyped in Svg2Bmp.hh
// Svg2Bmp is designed to rasterize an SVG image into a Bitmap image. I only tested this code to work on Linux systems,
// so if you run this on anything else use at your own risk.

void Svg2Bmp::fillPath (bitmap_image & bmp,  int red, int green, int blue)
{
    // This function contains a simple algorithm to fill a shape with a given color
    debug << "Entering fill path\n";
    if(red == -1 && green == -1 && blue == -1)
    { // If there is no fill target, exit
      debug << "Exiting fill path\n";
      return;
    }
    bitmap_image clearBmp = bmp;
    for(int i = 0; i < clearBmp.width(); i++) // Fill empty image with color that is not the fill color
    for(int j = 0; j < clearBmp.height(); j++)
      if(red == 255 && green == 255 && blue == 255)
        clearBmp.set_pixel(i, j, 0, 0, 0);
      else
        clearBmp.set_pixel(i, j, 255, 255, 255);
    PathElement close('L'); // Close unclosed shapes, this allows us to fill shapes that not close themselves
    close.points.push_back(path[0].points[0]);
    debug << close.eType << endl;
    path.push_back(close);
    drawPath(clearBmp, path, red, green, blue, 1); //  Draw path in fill color with closed shape
    path.pop_back(); // Remove closing line
    vector<Point> fillVector, filledPoints;
    fillVector = getFillPoints(); // Get vector of points of interior points
    for(int i = 0; i < fillVector.size(); i++)
    { // Call Fill() at each point in the vector
      Fill(clearBmp, filledPoints, fillVector[i].x, fillVector[i].y, red, green, blue);
    }
    rgb_t copyPixel;
    for(int i = 0; i < filledPoints.size(); i++)
    { // Copy filled pixels to original image
      clearBmp.get_pixel(filledPoints[i].x, filledPoints[i].y, copyPixel);
      bmp.set_pixel(filledPoints[i].x, filledPoints[i].y, copyPixel);
    }
    debug << "Exiting fill path\n";
}

void Svg2Bmp::Fill (bitmap_image & bmp, vector<Point> & filledPoints, int currentX, int currentY, int fillRed, int fillGreen, int fillBlue)
{
    // This function serves as a wrapper function for filling a shape with a color as well as gets the correct colors to use
    debug << "Entering Fill\n";
    if(currentX < 0 || currentY < 0 || currentX > bmp.width() || currentY > bmp.height()) // Check for image bounds
        return;
    rgb_t targetColor;
    bmp.get_pixel(currentX, currentY, targetColor); // Set start point as target color
    FillBase(bmp, filledPoints, currentX, currentY, fillRed, fillGreen, fillBlue, targetColor); // Recursive call to fill function
    debug << "Exiting Fill\n";
}

void Svg2Bmp::FillBase(bitmap_image & bmp, vector<Point> & filledPoints, int currentX, int currentY, int fillRed, int fillGreen, int fillBlue, rgb_t targetColor)
{
    // This function preforms a simple flood fill to set the interior color of a shape to the desired color
    debug << "Entering FillBase\n";
    if(currentX < 0 || currentY < 0 || currentX > bmp.width() || currentY > bmp.height()) // Check for image bounds
        return;
    rgb_t curPixel;
    bmp.get_pixel(currentX, currentY, curPixel);
    if(curPixel.red == fillRed && curPixel.green == fillGreen && curPixel.blue == fillBlue) // If current color == fill color
        return;
    else if(curPixel.red != targetColor.red || curPixel.green != targetColor.green || curPixel.blue != curPixel.blue) // If current color != target color
        return;
    else
    { // Fill target pixel with fill color
      debug << "Filling Pixel: (" << currentX << ", " << currentY << ")\n";
      filledPoints.push_back(Point(currentX, currentY));
      bmp.set_pixel(currentX, currentY, fillRed, fillGreen, fillBlue);
    }
    // Recursive calls to adjacent pixels
    FillBase(bmp, filledPoints, currentX, currentY + 1, fillRed, fillGreen, fillBlue, targetColor); // South
    FillBase(bmp, filledPoints, currentX, currentY - 1, fillRed, fillGreen, fillBlue, targetColor); // North
    FillBase(bmp, filledPoints, currentX + 1, currentY, fillRed, fillGreen, fillBlue, targetColor); // East
    FillBase(bmp, filledPoints, currentX - 1, currentY, fillRed, fillGreen, fillBlue, targetColor); // West
    debug << "Exiting FillBase\n";
}

void Svg2Bmp::transformPath ()
{
    debug << "Entering transformPath\n";
    if(transformations.empty())
        return;
    else
    {
      for(int i = transformations.size() - 1; i >= 0; i--)
        { // Iterate through transformation vector
          switch(transformations[i].eType)
            {
            case 'T':
              for(int j = 0; j < path.size(); j++)
                { // For each elem in path
                  for(int k = 0; k < path[j].points.size(); k++)
                    { // For each point in path section
                      path[j].points[k].x = path[j].points[k].x + transformations[i].values[0]; // Translate X
                      path[j].points[k].y = path[j].points[k].y + transformations[i].values[1]; // Translate Y
                    }
                }
              break;
            case 'R':
              for(int j = 0; j < path.size(); j++)
                { // For each elem in path
                  for(int k = 0; k < path[j].points.size(); k++)
                    { // For each point in path section
                      double r, theta;
                      r = sqrt(pow(path[j].points[k].x - transformations[i].values[1], 2) + pow(path[j].points[k].y - transformations[i].values[2], 2));
                      theta = atan2(path[j].points[k].y - transformations[i].values[2], path[j].points[k].x - transformations[i].values[1]);
                      path[j].points[k].x = (double)(r * cos(theta + (transformations[i].values[0] * ((22.0 / 7.0) / 180.0)))) + transformations[i].values[1]; // Rotate X
                      path[j].points[k].y = (double)(r * sin(theta + (transformations[i].values[0] * ((22.0 / 7.0) / 180.0)))) + transformations[i].values[2]; // Rotate Y
                    }
                }
              break;
            case 'S':
              for(int j = 0; j < path.size(); j++)
                { // For each elem in path
                  for(int k = 0; k < path[j].points.size(); k++)
                    { // For each point in path section
                      path[j].points[k].x = path[j].points[k].x * transformations[i].values[0]; // Scale X
                      path[j].points[k].y = path[j].points[k].y * transformations[i].values[1]; // Scale Y
                    }
                }
              break;
            default:
              break;
            }
        }
    }
    debug << "Exiting transformPath\n";
}
