#include "Svg2Bmp.h"
#include <cstdlib>
#include <cmath>
#include <sstream>
#include <vector>
#include <algorithm>

#define debug if (DEBUG) cout

using namespace std;

#define DEFAULT_HW 1024

Svg2Bmp::Svg2Bmp ()
{
}

Svg2Bmp::Svg2Bmp (const string & svgName, const string & expName)
{
	svgHeight = DEFAULT_HW;
	svgWidth = DEFAULT_HW;
	ClearAttr ();
	svgFile.open (svgName.c_str());
	if (svgFile.fail())
	{
		cerr << svgName << " could not be opened.\n";
		exit (1);
	}
	bmpName = svgName;
	size_t s = bmpName.rfind ("svg");
	bmpName.replace (s, 3, "bmp");
	string str;
	for (svgFile >> str; str != "<svg"; svgFile >> str);
	for (svgFile >> str; str != ">"; svgFile >> str)
		Attribute (str);
	svgHeight = height > 0 ? height : 1024;
	svgWidth = width > 0 ? width : 1024;
	if (expName == "")
	{ 
		bmp = bitmap_image (svgWidth+1, svgHeight+1);
		if (bmp.failed())
		{
			cerr << "Bitmap could not be created\n";
			exit (2);
		}
		bmp.clear(0xFF);
	}
	else
	{
		bmp = bitmap_image (expName);
		svgHeight = bmp.height();
		svgWidth = bmp.width();
	}
	fillBmp = bitmap_image (svgWidth+1, svgHeight+1);
	while (svgFile >> str && (str != "</svg>" && str != "</svg"))
	{
		ClearAttr ();
		string attr;
		while (svgFile >> attr && (attr != "/>" && attr != ">" && attr != "-->"))
			Attribute (attr);
		// for round rects
		if (rx == -1)
			rx = ry != -1 ? ry : 0;
		if (ry == -1)
			ry = rx != -1 ? rx : 0;
		if (VERBOSE)
			ReportAttributes (str);
		if (str == "<line")
			Line ();
		else if (str == "<rect")
			Rect ();
		else if (str == "<circle")
			Circle ();
		else if (str == "<ellipse")
			Ellipse ();
		else if (str == "<polygon")
			Polygon ();
		else if (str == "<polyline")
			Polyline ();
		else if (str == "<path")
			Path ();
	}
}

Svg2Bmp::~Svg2Bmp ()
{
	bmp.save_image (bmpName.c_str());
	svgFile.close();
}

Svg2Bmp::Color::Color ()
{
	name = "none";
	red = -1;
	green = -1;
	blue = -1;
}

Svg2Bmp::Color::Color (string n)
{
	*this = Color ();
	if (n == "black") *this = Color (n, 0, 0, 0);
	else if (n == "silver") *this = Color (n, 192, 192, 192);
	else if (n == "gray") *this = Color (n, 128, 128, 128);
	else if (n == "white") *this = Color (n, 255, 255, 255);
	else if (n == "maroon") *this = Color (n, 128, 0, 0);
	else if (n == "red") *this = Color (n, 255, 0, 0);
	else if (n == "violet") *this = Color (n, 128, 0, 128);
	else if (n == "fuchsia") *this = Color (n, 255, 0, 255);
	else if (n == "green") *this = Color (n, 0, 128, 0);
	else if (n == "lime") *this = Color (n, 0, 255, 0);
	else if (n == "olive") *this = Color (n, 128, 128, 0);
	else if (n == "yellow") *this = Color (n, 255, 255, 0);
	else if (n == "navy") *this = Color (n, 0, 0, 128);
	else if (n == "blue") *this = Color (n, 0, 0, 255);
	else if (n == "teal") *this = Color (n, 0, 128, 128);
	else if (n == "aqua") *this = Color (n, 0, 255, 255);
	else if (n == "tan") *this = Color (n, 209, 180, 142);
	else if (n == "orange") *this = Color (n, 237, 153, 7);
	else if (n == "purple") *this = Color (n, 124, 40, 209);
	else if (n[0] == '#')
	{
		name = n;
		red = stoi ("0x" + n.substr (1,2), nullptr, 0);
		green = stoi ("0x" + n.substr (3,2), nullptr, 0);
		blue = stoi ("0x" + n.substr (5,2), nullptr, 0);
	}
}

Svg2Bmp::Color::Color (string n, int r, int g, int b)
{
	name = n;
	red = r;
	green = g;
	blue = b;
}

ostream & operator << (ostream & outs, const Svg2Bmp::Color & C)
{
	outs << C.name << '(' << C.red << ',' << C.green << ',' << C.blue << ')';
	return outs;
}

Svg2Bmp::Point::Point ()
{
	x = 0;
	y = 0;
}

Svg2Bmp::Point::Point (double X, double Y)
{
	x = X;
	y = Y;
}

double dist (const Svg2Bmp::Point & P1, const Svg2Bmp::Point & P2)
{
	double dx = P1.x - P2.x;
	double dy = P1.y - P2.y;
	return sqrt (dx*dx + dy*dy);
}

Svg2Bmp::Point Svg2Bmp::Point::operator += (const Point & P)
{
	x += P.x;
	y += P.y;
	return *this;
}

istream & operator >> (istream & ins, Svg2Bmp::Point & P)
{
	char separator;
	ins >> P.x;
	ins.get (separator);
	ins >> P.y;
	return ins;
}

ostream & operator << (ostream & outs, const Svg2Bmp::Point & P)
{
	outs << P.x << "," << P.y;
	return outs;
}

Svg2Bmp::LineSeg::LineSeg ()
{
	m = b = 0;
}

Svg2Bmp::LineSeg::LineSeg (const Point & P1, const Point & P2)
{
	p1 = P1;
	p2 = P2;
	y1 = round (p1.y);
	y2 = round (p2.y);
	if (p1.x == p2.x)
		m = INFINITY;
	else
	{
		m = (p2.y - p1.y) / (p2.x - p1.x);
		b = p1.y - m * p1.x;
	}
}

bool Svg2Bmp::LineSeg::operator < (const LineSeg & LS) const
{
	if (p1.y == LS.p1.y)
		return p1.x < LS.p1.x;
	return p1.y < LS.p1.y;
}

double Svg2Bmp::LineSeg::xIntercept (double y)
{
	if (m == INFINITY || m == 0 || y1 == y2)
		return p1.x;
	return (y - b) / m;
}

ostream & operator << (ostream & outs, const  Svg2Bmp::LineSeg & LS)
{
	outs << '(' << LS.p1 << ")-(" << LS.p2 << ')';
	return outs;
}

Svg2Bmp::TransformElement::TransformElement ()
{
	eType = ' ';
}

Svg2Bmp::TransformElement::TransformElement (char t)
{
	eType = t;
}

Svg2Bmp::PathElement::PathElement ()
{
	eType = ' ';
}

Svg2Bmp::PathElement::PathElement (char t)
{
	eType = t;
}

void Svg2Bmp::ClearAttr ()
{
	height = 0;
	width = 0;
	fill = Color ("none");
	stroke = Color ("none");
	stroke_width = 1;
	x1 = 0;
	y1 = 0;
	x2 = 0;
	y2 = 0;
	cx = 0;
	cy = 0;
	r = 0;
	x = 0;
	y = 0;
	rx = -1;
	ry = -1;
	points = "";
	d = "";
	path.clear();
	style = "";
	transform = "";
	transformations.clear();
}

string format (const string & S)
{ // for style string
	string str = S;
	size_t pos = 0;
	while ((pos = str.find ("\t", pos)) != string::npos)
		str.replace (pos, 1, " ");
	pos = 0;
	while ((pos = str.find ("  ", pos)) != string::npos)
		str.replace (pos, 2, " ");
	pos = 0;
	while ((pos = str.find ("; ", pos)) != string::npos)
		str.replace (pos, 1, "\"");
	pos = 0;
	while ((pos = str.find (";", pos)) != string::npos)
		str.replace (pos, 1, "\" ");
	pos = 0;
	while ((pos = str.find (": ", pos)) != string::npos)
		str.replace (pos, 2, "=\"");
	pos = 0;
	while ((pos = str.find (":", pos)) != string::npos)
		str.replace (pos, 1, "=\"");
	if (str.back() == ' ')
		str.pop_back();
	if (str.back() != '\"')
		str.push_back('\"');
	return str;
}

void Svg2Bmp::Attribute (const string & S)
{
	string str = S;
	if (str.substr(0,7) == "height=")
		height = nValue (str, svgHeight);
	else if (str.substr(0,6) == "width=")
		width = nValue (str, svgWidth);
	else if (str.substr(0,5) == "fill=")
		fill = Color (sValue (str));
	else if (str.substr(0,7) == "stroke=")
		stroke = Color (sValue (str));
	else if (str.substr(0,13) == "stroke-width=")
		stroke_width = nValue (str);
	else if (str.substr(0,3) == "x1=")
		x1 = nValue (str, svgWidth);
	else if (str.substr(0,3) == "y1=")
		y1 = nValue (str, svgHeight);
	else if (str.substr(0,3) == "x2=")
		x2 = nValue (str, svgWidth);
	else if (str.substr(0,3) == "y2=")
		y2 = nValue (str, svgHeight);
	else if (str.substr(0,3) == "cx=")
		cx = nValue (str, svgWidth);
	else if (str.substr(0,3) == "cy=")
		cy = nValue (str, svgHeight);
	else if (str.substr(0,2) == "r=")
		r = nValue (str);
	else if (str.substr(0,2) == "x=")
		x = nValue (str, svgWidth);
	else if (str.substr(0,2) == "y=")
		y = nValue (str, svgHeight);
	else if (str.substr(0,3) == "rx=")
		rx = nValue (str, svgWidth);
	else if (str.substr(0,3) == "ry=")
		ry = nValue (str, svgHeight);
	else if (str.substr(0,7) == "points=")
	{
		points = sValue (str);
		for (int i = 0; i < points.length(); i++)
			if (points[i] == ',')
				points[i] = ' ';	
	}
	else if (str.substr(0,2) == "d=")
		d = sValue (str);
	else if (str.substr(0,6) == "style=")
	{
		style = sValue (str);
		string modStr = format (style);
		stringstream ss (modStr);
		string one;
		while (ss >> one)
			Attribute (one);
	}
	else if (str.substr(0,10) == "transform=")
	{
		transform = sValue (str);
		String2Transformations (transform);
	}
}

void Svg2Bmp::ReportAttributes (string eType)
{
	cout << "SVG Element: " << eType << endl;
	cout << "\tfill = " << fill << endl;
	cout << "\tstroke = " << stroke << endl;
	cout << "\tstroke-width = " << stroke_width << endl;
	cout << "\tx1 = " << x1 << endl;
	cout << "\ty1 = " << y1 << endl;
	cout << "\tx2 = " << x2 << endl;
	cout << "\ty2 = " << y2 << endl;
	cout << "\tcx = " << cx << endl;
	cout << "\tcy = " << cy << endl;
	cout << "\tr = " << r << endl;
	cout << "\tx = " << x << endl;
	cout << "\ty = " << y << endl;
	cout << "\theight = " << height << endl;
	cout << "\twidth = " << width << endl;
	cout << "\trx = " << rx << endl;
	cout << "\try = " << ry << endl;
	cout << "\tpoints = " << points << endl;
	cout << "\td = " << d << endl;
	cout << "\tstyle = " << style << endl;
	cout << "\ttransform = " << transform << endl;
}

double Svg2Bmp::nValue (const string & str)
{
	size_t c1 = str.find ("\"");
	if (c1 == string::npos)
		return 0;
	size_t c2 = str.rfind ("\"");
	if (c2 == c1)
		return 0;
	double n = stod (str.substr (c1+1, c2-c1-1));
	return n;
}

double Svg2Bmp::nValue (const string & str, int base)
{
	double n = nValue (str);
	size_t p = str.find ("%");
	if (p == string::npos)
		return n;
	double pct = n * base / 100.0;
	return pct;
}

string Svg2Bmp::sValue (const string & str)
{
	string all = str;
	size_t c1 = all.find ("\"");
	if (c1 == string::npos)
		return "";
	size_t c2 = all.rfind ("\"");
	if (c2 == c1)
	{
		string s;
		do
		{
			svgFile >> s;
			all += " " + s;
		} while (s.find ("\"") == string::npos);
		c2 = all.rfind ("\"");
	}
	string s = all.substr (c1+1, c2-c1-1);
	return s;
}

void Svg2Bmp::String2Transformations (const string & t)
{
	transformations.clear();
	int pos1 = 0, pos2;
	while ((pos2 = t.find (')', pos1)) != string::npos)
	{
		string s = t.substr (pos1, pos2-pos1); 
		debug << s << endl;
		for (int i = 0; i < s.length(); i++)
			if (s[i] == '(' || s[i] == ',')
				s[i] = ' ';
		debug << s << endl;
		stringstream ss (s);
		string w;
		ss >> w;
		TransformElement te(toupper (w[0]));
		double v;
		while (ss >> v)
			te.values.push_back (v);
		debug << te.eType;
		switch (te.eType)
		{
			case 'R':
				while (te.values.size() < 3)
					te.values.push_back(0);
				break;
			case 'S':
				if (te.values.size() < 1)
					te.values.push_back(1);
				if (te.values.size() < 2)
					te.values.push_back(te.values[0]);
				break;
			case 'T':
				while (te.values.size() < 2)
					te.values.push_back(0);
				break;
		}
		for (int i = 0; i < te.values.size(); i++)
			debug << ' ' << te.values[i];
		debug << endl;
		transformations.push_back (te);
		pos1 = pos2+1;
	}
}

void Svg2Bmp::String2Path (const string & d)
{
	path.clear();
	stringstream ss (d);
	char et;
	Point start;
	while (ss >> et)
	{
		PathElement pe;
		Point p, q, r;
		pe.eType = toupper (et);
		switch (et)
		{
			case 'M':
				ss >> p;
				pe.points.push_back(p);
				start = p;
				break;
			case 'm':
				ss >> p;
				pe.points.push_back(p+=start);
				start = p;
				break;
			case 'L':
				ss >> p;
				pe.points.push_back(p);
				start = p;
				break;
			case 'l':
				ss >> p;
				pe.points.push_back(p+=start);
				start = p;
				break;
			case 'Q':
				ss >> p >> q;
				pe.points.push_back(p);
				pe.points.push_back(q);
				start = q;
				break;
			case 'q':
				ss >> p >> q;
				pe.points.push_back(p+=start);
				pe.points.push_back(q+=start);
				start = q;
				break;
			case 'C':
				ss >> p >> q >> r;
				pe.points.push_back(p);
				pe.points.push_back(q);
				pe.points.push_back(r);
				start = r;
				break;
			case 'c':
				ss >> p >> q >> r;
				pe.points.push_back(p+=start);
				pe.points.push_back(q+=start);
				pe.points.push_back(r+=start);
				start = r;
				break;
		}
		path.push_back(pe);
	}
}

bool Svg2Bmp::combine (LineSeg & ls1, const LineSeg & ls2)
{ 
        double x1 = ls1.p1.x, x2 = ls1.p2.x;
        if (ls1.p1.x < ls1.p2.x && ls2.p1.x < ls2.p2.x)
        {
                if (ls2.p1.x > ls1.p2.x || ls2.p2.x < ls1.p1.x)
                        return false;
                x1 = min (ls1.p1.x, ls2.p1.x);
                x2 = max (ls1.p2.x, ls2.p2.x);
        }
        else if (ls1.p1.x < ls1.p2.x && ls2.p1.x > ls2.p2.x)
        {
                if (ls2.p2.x > ls1.p2.x || ls2.p1.x < ls1.p1.x)
                        return false;
                x1 = min (ls1.p1.x, ls2.p2.x);
                x2 = max (ls1.p2.x, ls2.p1.x);
        }
        else if (ls1.p1.x > ls1.p2.x && ls2.p1.x > ls2.p2.x)
        {
                if (ls2.p2.x > ls1.p1.x || ls2.p1.x < ls1.p2.x)
                        return false;
                x1 = max (ls1.p2.x, ls2.p2.x);
                x2 = min (ls1.p1.x, ls2.p1.x);
        }
        else if (ls1.p1.x > ls1.p2.x && ls2.p1.x < ls2.p2.x)
        {
                if (ls2.p1.x > ls1.p1.x || ls2.p2.x < ls2.p1.x)
                        return false;
                x1 = max (ls1.p1.x, ls2.p2.x);
                x2 = min (ls1.p2.x, ls2.p1.x);
        }
        ls1.p1.x = x1;
        ls1.p2.x = x2;
        return true;
}

vector <Svg2Bmp::Point> Svg2Bmp::getFillPoints ()
{
	vector <LineSeg> ls;
	int miny = svgHeight, maxy = 0;
	for (int i = 0; i < lineSegs.size(); i++)
	{
		miny = min (miny, lineSegs[i].y1);
		miny = min (miny, lineSegs[i].y2);
		maxy = max (maxy, lineSegs[i].y1);
		maxy = max (maxy, lineSegs[i].y2);
		if (lineSegs[i].p1.y == lineSegs[i].p2.y)
		{
			LineSeg one = lineSegs[i];
			double y = one.p1.y;
			int j = i+1;
			while (j < lineSegs.size() && lineSegs[j].p1.y == y && lineSegs[j].p2.y == y)
			{
				combine (one, lineSegs[j]);
				j++;
			}
			ls.push_back (one);
			i = j-1;
		}
		else
			ls.push_back (lineSegs[i]);
	}
	ls.insert(ls.begin(), ls.back());
	vector <Point> P;
	vector <double> X;
	miny = max (0, miny);
	maxy = min (svgHeight, maxy);
	for (int y = miny+1; y < maxy; y++)
	{
		X.clear();
		for (int i = 1; i < ls.size(); i++)
		{
			if ((y >= ls[i].p1.y && y <= ls[i].p2.y) || (y >= ls[i].p2.y && y <= ls[i].p1.y))
			{
				double x = ls[i].xIntercept(y);
				/*
				if (x == ls[i].p1.x && x == ls[i-1].p2.x) 
				{ // Is this point needed?
					if (ls[i].p2.y < y && ls[i-1].p1.y < y)
						X.push_back (x);
					else if (ls[i].p2.y > y && ls[i-1].p1.y > y)
						X.push_back (x);
				}
				else
				*/
				X.push_back (x);
			}
		}
		debug << y << " : ";
		for (int i = 0; i < X.size(); i++)
			debug << X[i] << ' ';
		debug << endl;
		sort (X.begin(), X.end()); 
		vector<double>::iterator itr = X.begin();
		for (int i = 1; i < X.size(); i+=2)
		{
			double x1 = *itr++;
			double x2 = *itr++;
			double x = (x2 + x1)/2;
			if (x >= 0 && x2 - x1 > 2)
				P.push_back (Point ((x2 + x1)/2, y));
		}
	}
	return P;
}
void Svg2Bmp::Path ()
{
	String2Path (d);
	transformPath (); // Added for Project 2
	fillPath (bmp, fill.red, fill.green, fill.blue);
	drawPath (bmp, path, stroke.red, stroke.green, stroke.blue, 1);
}

// From: Svg2Bmp.hpp
// Project: CS 375 Project 1
// Author: Dr. Watts
// Description: This section contains the implementations of the functions
// added to the class Svg2Bmp for Project 1.

void Svg2Bmp::drawLine (bitmap_image & bmp, Point p1, Point p2, int red, int green, int blue, int width)
{
	debug << "drawLine from " << p1 << " to " << p2 << endl;
	lineSegs.push_back (LineSeg (p1, p2)); // Added for Project 2
	bmp.set_pixel (p1.x, p1.y, red, green, blue);
	bmp.set_pixel (p2.x, p2.y, red, green, blue);
	int count = ceil (max (abs (p2.x - p1.x), abs (p2.y - p1.y)));
	double dx = (p2.x - p1.x) / count;
	double dy = (p2.y - p1.y) / count;
	double x = p1.x;
	double y = p1.y;
	for (int i = 0; i <= count; i++) 
	{
		bmp.set_pixel (x, y, red, green, blue);
		x += dx;
		y += dy;
	}
}

Svg2Bmp::Point Svg2Bmp::getRPoint (double t, Point p1, Point p2)
{
	double x = p1.x + t * (p2.x - p1.x);
	double y = p1.y + t * (p2.y - p1.y);
	return Point (x, y);
}

void Svg2Bmp::drawCurve (bitmap_image & bmp, Point p1, Point p2, Point p3, int red, int green, int blue, int width)
{
	vector <Point> curve;
	int np1 = round (dist (p1, p2));
	int np2 = round (dist (p2, p3));
	int np = (np1 + np2) / 2;
	for (int i = 0; i <= np; i++)
	{
		double t = double (i) / np;
		Point r1 = getRPoint (t, p1, p2);
		Point r2 = getRPoint (t, p2, p3);
		curve.push_back (getRPoint (t, r1, r2));
	}
	for (int i = 1; i < curve.size(); i++)
	{
		debug << curve[i-1] << ",";
		drawLine (bmp, curve[i-1], curve[i], red, green, blue, width);
	}
	debug << curve[curve.size()-1] << endl;
}

void Svg2Bmp::drawCurve (bitmap_image & bmp, Point p1, Point p2, Point p3, Point p4, int red, int green, int blue, int width)
{
	vector <Point> curve;
	int np1 = round (dist (p1, p2));
	int np2 = round (dist (p2, p3));
	int np3 = round (dist (p3, p4));
	int np = (np1 + np2 + np3) / 3;
	for (int i = 0; i <= np; i++)
	{
		double t = double (i) / np;
		Point r1 = getRPoint (t, p1, p2);
		Point r2 = getRPoint (t, p2, p3);
		Point r3 = getRPoint (t, p3, p4);
		Point r4 = getRPoint (t, r1, r2);
		Point r5 = getRPoint (t, r2, r3);
		curve.push_back (getRPoint (t, r4, r5));
	}
	for (int i = 1; i < curve.size(); i++)
	{
		debug << curve[i-1] << ",";
		drawLine (bmp, curve[i-1], curve[i], red, green, blue, width);
	}
	debug << curve[curve.size()-1] << endl;
}

void Svg2Bmp::drawPath (bitmap_image & bmp, vector <PathElement> & path, int red, int green, int blue, int width)
{
	if (red == -1 || green == -1 || blue == -1)
		return;
	debug << "Drawing Path with color (" << red << ',' << green << ',' << blue << ')' << endl;
	lineSegs.clear ();
	for (int p = 0; p < path.size(); p++)
	{
		debug << path[p].eType << " : ";
		for (int i = 0; i < path[p].points.size(); i++)
			debug << path[p].points[i] << ' ';
		debug << endl;
	}
	Point zPoint, p1, p2, p3, p4;
	//if (path[0].eType == 'M')
	//	zPoint = path[0].points[0];
	for (int i = 0; i < path.size(); i++)
	{
		PathElement pe = path[i];
		switch (pe.eType)
		{
			case 'M':
				zPoint = pe.points[0];
				p1 = pe.points[0];
				break; 
			case 'L':
				p2 = pe.points[0];
				drawLine (bmp, p1, p2, red, green, blue, width);
				p1 = p2;
				break; 
			case 'Q':
				p2 = pe.points[0];
				p3 = pe.points[1];
				drawCurve (bmp, p1, p2, p3, red, green, blue, width);
				p1 = p3;
				break; 
			case 'C':
				p2 = pe.points[0];
				p3 = pe.points[1];
				p4 = pe.points[2];
				drawCurve (bmp, p1, p2, p3, p4, red, green, blue, width);
				p1 = p4;
				break; 
			case 'Z':
				p2 = zPoint;
				drawLine (bmp, p1, p2, red, green, blue, width);
				p1 = p2;
				break; 
		}
	}
}

void Svg2Bmp::Line ()
{
	stringstream ss;
	ss << "M " << Point (x1, y1);
	ss << " L " << Point (x2, y2);
	d = ss.str();
	String2Path (d);
	transformPath (); // Added for Project 2
	drawPath (bmp, path, stroke.red, stroke.green, stroke.blue, 1);
}

void Svg2Bmp::Rect ()
{
	Point ul = Point (x, y);
	Point ur = Point (x+width, y);
	Point ll = Point (x, y+height);
	Point lr = Point (x+width, y+height);
	stringstream ss;
	if (rx == 0 || ry == 0)
	{ // regular rectangle
		ss << "M " << ul;
		ss << " L " << ur;
		ss << " L " << lr;
		ss << " L " << ll;
		ss << " L " << ul;
	}
	else
	{ // rounded rectangle
		Point ul1 (ul.x, ul.y+ry); 
		Point ul2 (ul.x+rx, ul.y);
		Point ur1 (ur.x-rx, ur.y);
		Point ur2 (ur.x, ur.y+ry);
		Point lr1 (lr.x, lr.y-ry);
		Point lr2 (lr.x-rx, lr.y);
		Point ll1 (ll.x+rx, ll.y);
		Point ll2 (ll.x, ll.y-ry); 
		if (ul2.x > ur1.x)
			ul2.x = ur1.x = (ul2.x + ur1.x) / 2;
		if (ur2.y > lr1.y)
			ur2.y = lr1.y = (ur2.y + lr1.y) / 2; 
		if (ll1.x > lr2.x)
			ll1.x = lr2.x = (ll1.x + lr2.x) / 2;
		if (ul1.y > ll2.y)
			ul1.y = ll2.y = (ul1.y + ll2.y) / 2;
		double offsetx = rx * 0.55;
		double offsety = ry * 0.55;
		ss << "M " << ul1;
		ss << " C " << Point (ul1.x, ul1.y-offsety) << ' ' << Point (ul2.x-offsetx, ul2.y) << ' ' << ul2; 
		ss << " L " << ur1;
		ss << " C " << Point (ur1.x+offsetx, ur1.y) << ' ' << Point (ur2.x, ur2.y-offsety) << ' ' << ur2; 
		ss << " L " << lr1;
		ss << " C " << Point (lr1.x, lr1.y+offsety) << ' ' << Point (lr2.x+offsetx, lr2.y) << ' ' << lr2; 
		ss << " L " << ll1;
		ss << " C " << Point (ll1.x-offsetx, ll1.y) << ' ' << Point (ll2.x, ll2.y+offsety) << ' ' << ll2; 
		ss << " L " << ul1;
	}
	d = ss.str();
	String2Path (d);
	transformPath (); // Added for Project 2
	fillPath (bmp, fill.red, fill.green, fill.blue);
	drawPath (bmp, path, stroke.red, stroke.green, stroke.blue, 1);
}

void Svg2Bmp::Circle ()
{
	double offset = r * 0.55;
	stringstream ss;
	ss << "M " << Point (cx-r, cy);
	ss << " C " << Point (cx-r, cy-offset) << ' ' << Point (cx-offset, cy-r) << ' ' << Point (cx, cy-r); 
	ss << " C " << Point (cx+offset, cy-r) << ' ' << Point (cx+r, cy-offset) << ' ' << Point (cx+r, cy); 
	ss << " C " << Point (cx+r, cy+offset) << ' ' << Point (cx+offset, cy+r) << ' ' << Point (cx, cy+r); 
	ss << " C " << Point (cx-offset, cy+r) << ' ' << Point (cx-r, cy+offset) << ' ' << Point (cx-r, cy); 
	d = ss.str();
	String2Path (d);
	transformPath (); // Added for Project 2
	fillPath (bmp, fill.red, fill.green, fill.blue);
	drawPath (bmp, path, stroke.red, stroke.green, stroke.blue, 1);
}

void Svg2Bmp::Ellipse ()
{
	double offsetx = rx * 0.55;
	double offsety = ry * 0.55;
	stringstream ss;
	ss << "M " << Point (cx-rx, cy);
	ss << " C " << Point (cx-rx, cy-offsety) << ' ' << Point (cx-offsetx, cy-ry) << ' ' << Point (cx, cy-ry); 
	ss << " C " << Point (cx+offsetx, cy-ry) << ' ' << Point (cx+rx, cy-offsety) << ' ' << Point (cx+rx, cy); 
	ss << " C " << Point (cx+rx, cy+offsety) << ' ' << Point (cx+offsetx, cy+ry) << ' ' << Point (cx, cy+ry); 
	ss << " C " << Point (cx-offsetx, cy+ry) << ' ' << Point (cx-rx, cy+offsety) << ' ' << Point (cx-rx, cy); 
	d = ss.str();
	String2Path (d);
	transformPath (); // Added for Project 2
	fillPath (bmp, fill.red, fill.green, fill.blue);
	drawPath (bmp, path, stroke.red, stroke.green, stroke.blue, 1);
}

void Svg2Bmp::Polygon ()
{
	stringstream ss (points), ds;
	Point p;
	ss >> p;
	ds << "M " << p;
	while (ss >> p)
		ds << " L " << p;
	ds << " Z";
	d = ds.str ();
	debug << "d = " << d << endl;
	String2Path (d);
	transformPath (); // Added for Project 2
	fillPath (bmp, fill.red, fill.green, fill.blue);
	drawPath (bmp, path, stroke.red, stroke.green, stroke.blue, 1);
	
}

void Svg2Bmp::Polyline ()
{
	stringstream ss (points), ds;
	Point p;
	ss >> p;
	ds << "M " << p;
	while (ss >> p)
		ds << " L " << p;
	d = ds.str ();
	debug << "d = " << d << endl;
	String2Path (d);
	transformPath (); // Added for Project 2
	fillPath (bmp, fill.red, fill.green, fill.blue);
	drawPath (bmp, path, stroke.red, stroke.green, stroke.blue, 1);
}

#include "Svg2Bmp.hpp"
