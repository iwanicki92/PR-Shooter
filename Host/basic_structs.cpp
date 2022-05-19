#include "basic_structs.hpp"

Point::Point(double x, double y) : x(x), y(y) {}

Point& Point::operator*=(double rhs) {
    this->x *= rhs;
    this->y *= rhs;
    return *this;
}

Point& Point::operator+=(const Point& rhs) {
    this->x += rhs.x;
    this->y += rhs.y;
    return *this;
}

Point Point::operator*(double rhs) const {
    return Point(x * rhs, y * rhs);
}

Point Point::operator+(const Point& rhs) const {
    return Point(x + rhs.x, y + rhs.y);
}

Point Point::operator-(const Point& rhs) const {
    return Point(x - rhs.x, y - rhs.y);
}

Circle::Circle(Point p, double r) : centre(p), r(r) {}

Rectangle::Rectangle(Point p1, Point p2, Point p3, Point p4) : points({{p1, p2, p3, p4}}) {}

std::ostream& operator<<(std::ostream& out, const Point& rhs) {
    out << rhs.x << "," << rhs.y;
    return out;
}