#include "collisions.h"
#include <cmath>

double triangleArea(const Point& a, const Point& b, const Point& c);
bool circleLineSegmentCollision(const Circle& c, const Point& a, const Point& b);
bool ciclePointCollision(const Circle& c, const Point& a);

double square(double x) {
	return x * x;
}

double distance(const Point& a, const Point& b) {
	return std::sqrt(square(b.x - a.x) + square(b.y - a.y));
}

bool checkCollision(const Circle& c1, const Circle& c2) {
	return (square(c1.centre.x - c2.centre.x) + square(c1.centre.y - c2.centre.y)) < square(c1.r + c2.r) - 1.0e-10;
}

bool checkCollision(const Circle& c, const Rectangle& rec) {
	// 1. Sprawdzenie, czy środek koła(punkt) jest w prostokącie - koło całe w środku czworokąta
	double field1 = triangleArea(rec.points[0], rec.points[1], c.centre);
	field1 += triangleArea(rec.points[1], rec.points[2], c.centre);
	field1 += triangleArea(rec.points[2], rec.points[3], c.centre);
	field1 += triangleArea(rec.points[3], rec.points[0], c.centre);
	double field2 = triangleArea(rec.points[0], rec.points[1], rec.points[2])
					+ triangleArea(rec.points[2], rec.points[3], rec.points[0]);
	if (field1 >= field2 - 0.01 && field1 <= field2 + 0.01)
		return true;
	// 2. Sprawdzenie, czy jedna z 4 krawędzi przecina się z kołem - przecinanie się
	if (circleLineSegmentCollision(c, rec.points[0], rec.points[1])
		|| circleLineSegmentCollision(c, rec.points[1], rec.points[2]))
		return true;
	else if (circleLineSegmentCollision(c, rec.points[2], rec.points[3])
		|| circleLineSegmentCollision(c, rec.points[3], rec.points[0]))
		return true;
	return false;
}

bool checkCollision(const Rectangle& rec, const Circle& c) {
	return checkCollision(c, rec);
}

bool checkCollision(const Circle& c, const Point& P1, const Point& P2) {
	return circleLineSegmentCollision(c, P1, P2);
}

double triangleHeight(const Point& a, const Point& b, const Point& c) {
	// h_bc = 2 * triangleArea / |bc|
	return 2 * triangleArea(a,b,c) / distance(b,c);
}

// triangleArea = sqrt(p(p-|ab|)(p-|bc|)(p-|ca|))
// p = (|ab|+|bc|+|ca|) / 2
double triangleArea(const Point& a, const Point& b, const Point& c) {
	double edge1 = distance(a, b);
	double edge2 = distance(b, c);
	double edge3 = distance(c, a);
	double p = (edge1 + edge2 + edge3) / 2;
	return sqrt(p * (p - edge1) * (p - edge2) * (p - edge3));
}

bool ciclePointCollision(const Circle& c, const Point& a)
{
	// sprawdzenie, czy punkt znajduje się w kole lub na okręgu
	double disX = c.centre.x - a.x;
	double disY = c.centre.y - a.y;
	double distance = sqrt((disX * disX) + (disY * disY));
	if (c.r >= distance)
		return true;
	return false;
}

bool circleLineSegmentCollision(const Circle& c, const Point& a, const Point& b) {
	if (ciclePointCollision(c, a) || ciclePointCollision(c, b))
		return true;
	// znalezienie najbliższego punktu od koła leżącego na lini AB,
	double line_length = distance(a, b);
	double dot = (((c.centre.x - a.x) * (b.x - a.x)) + ((c.centre.y - a.y) * (b.y - a.y))) / (line_length * line_length);
	Point closest(a.x + (dot * (b.x - a.x)), a.y + (dot * (b.y - a.y)));
	// sprawdzenie, czy najbliższy punkt należy do odcinka
	double length_ac = distance(a, closest);
	double length_bc = distance(b, closest);
	if (!(length_ac + length_bc >= line_length - 0.01 && length_ac + length_bc <= line_length + 0.01)) {
		// punkt nie znajduje się na odcinku
		return false;
	}
	// sprawdzenie odległości między najbliższym punktem, a środkiem koła
	double length_closest_c = distance(closest, c.centre);
	if (length_closest_c <= c.r)
		return true;
	return false;
}