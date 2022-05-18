#pragma once
struct Point {
	double x;
	double y;

	Point();
	Point(double x, double y);

};

struct Circle {
	Point p;
	double r;

	Circle();
	Circle(Point p, double r);
};

struct Rectangle {
	Point p1;
	Point p2;
	Point p3;
	Point p4;

	Rectangle();
	Rectangle(Point p1, Point p2, Point p3, Point p4);
};

double triangleField(Point a, Point b, Point c);
bool circleLineSegmentCollision(Circle c, Point a, Point b);
bool circleRectangleCollision(Circle c, Rectangle rec);
bool ciclePointCollision(Circle c, Point a);
double distance(Point a, Point b);