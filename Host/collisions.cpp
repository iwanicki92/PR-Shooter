#include "collisions.h"
#include <cmath>

Point::Point(double x, double y) {
	this->x = x;
	this->y = y;
}
Point::Point() {
	this->x = 0;
	this->y = 0;
}

Circle::Circle(Point p, double r) {
	this->p = p;
	this->r = r;
}

Circle::Circle() {
	this->p = Point(0,0);
	this->r = 0;
}

Rectangle::Rectangle(Point p1, Point p2, Point p3, Point p4) {
	this->p1 = p1;
	this->p2 = p2;
	this->p3 = p3;
	this->p4 = p4;
}

Rectangle::Rectangle() {
	this->p1 = Point(0, 1);
	this->p2 = Point(1, 1);
	this->p3 = Point(1, 0);
	this->p4 = Point(0, 0);
}

double distance(Point a, Point b) {
	return sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}

double triangleField(Point a, Point b, Point c) {
	double edge1 = distance(a, b);
	double edge2 = distance(b, c);
	double edge3 = distance(c, a);
	double p = (edge1 + edge2 + edge3) / 2;
	return sqrt(p * (p - edge1) * (p - edge2) * (p - edge3));
}

bool ciclePointCollision(Circle c, Point a)
{
	// sprawdzenie, czy punkt znajduje siê w kole lub na okrêgu
	double disX = c.p.x - a.x;
	double disY = c.p.y - a.y;
	double distance = sqrt((disX * disX) + (disY * disY));
	if (c.r >= distance)
		return true;
	return false;
}

bool circleRectangleCollision(Circle c, Rectangle rec) {
	// 1. Sprawdzenie, czy œrodek ko³a(punkt) jest w prostok¹cie - ko³o ca³e w œrodku czworok¹ta
	double field1 = triangleField(rec.p1, rec.p2, c.p);
	field1 += triangleField(rec.p2, rec.p3, c.p);
	field1 += triangleField(rec.p3, rec.p4, c.p);
	field1 += triangleField(rec.p4, rec.p1, c.p);
	double field2 = triangleField(rec.p1, rec.p2, rec.p3) + triangleField(rec.p3, rec.p4, rec.p1);
	if (field1 >= field2 - 0.01 && field1 <= field2 + 0.01)
		return true;
	// 2. Sprawdzenie, czy jedna z 4 krawêdzi przecina siê z ko³em - przecinanie siê
	if (circleLineSegmentCollision(c, rec.p1, rec.p2) || circleLineSegmentCollision(c, rec.p2, rec.p3))
		return true;
	else if (circleLineSegmentCollision(c, rec.p3, rec.p4) || circleLineSegmentCollision(c, rec.p4, rec.p1))
		return true;
	return false;
}

bool circleLineSegmentCollision(Circle c, Point a, Point b) {
	if (ciclePointCollision(c, a) || ciclePointCollision(c, b))
		return true;
	// znalezienie najbli¿szego punktu od ko³a le¿¹cego na lini AB,
	double line_length = distance(a, b);
	double dot = (((c.p.x - a.x) * (b.x - a.x)) + ((c.p.y - a.y) * (b.y - a.y))) / (line_length * line_length);
	Point closest(a.x + (dot * (b.x - a.x)), a.y + (dot * (b.y - a.y)));
	// sprawdzenie, czy najbli¿szy punkt nale¿y do odcinka
	double length_ac = distance(a, closest);
	double length_bc = distance(b, closest);
	if (!(length_ac + length_bc >= line_length - 0.01 && length_ac + length_bc <= line_length + 0.01)) {
		// punkt nie znajduje siê na odcinku
		return false;
	}
	// sprawdzenie odleg³oœci miêdzy najbli¿szym punktem, a œrodkiem ko³a
	double length_closest_c = distance(closest, c.p);
	if (length_closest_c <= c.r)
		return true;
	return false;
}