#pragma once
#include <array>
#include <ostream>

// type of data sent to client
enum DataType {
    CLIENT_ACCEPTANCE = 0,
    GAME_MAP = 1,
    GAME_STATE = 2
};

struct Point {
    Point(double x, double y);
    Point() = default;

    double x = 0;
    double y = 0;
};

std::ostream& operator<<(std::ostream& out, const Point& rhs);

struct Rectangle {
    Rectangle(Point p1, Point p2, Point p3, Point p4);
    Rectangle() = default;

    std::array<Point, 4> points;
};

struct Circle {
    Circle(Point p, double r);
    Circle() = default;

    Point centre;
    double r = 0;
};