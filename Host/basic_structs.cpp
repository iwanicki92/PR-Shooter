#include "basic_structs.hpp"

Point::Point(double x, double y) : x(x), y(y) {}

Circle::Circle(Point p, double r) : centre(p), r(r) {}

Rectangle::Rectangle(Point p1, Point p2, Point p3, Point p4) : points({{p1, p2, p3, p4}}) {}

std::ostream& operator<<(std::ostream& out, const Point& rhs) {
    out << rhs.x << "," << rhs.y;
    return out;
}