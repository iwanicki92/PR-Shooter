#pragma once
#include <array>
#include <ostream>

// type of data sent and received from client
enum DataType {
    // OUTGOING
    WELCOME_MESSAGE = 0,
    GAME_MAP = 1,
    GAME_STATE = 2,
    // INCOMING
    SPAWN = 10,
    SHOOT = 11,
    CHANGE_ORIENTATION = 12,
    CHANGE_MOVEMENT_DIRECTION = 13,

    OTHER = 999
};

struct Point {
    Point(double x, double y);
    Point(const Point& copy) = default;
    Point() = default;

    double x = 0;
    double y = 0;

    double length() const;
    Point& operator*=(double rhs);
    Point& operator/=(double rhs);
    Point& operator+=(const Point& rhs);
    Point operator*(double rhs) const;
    Point operator+(const Point& rhs) const;
    Point operator-(const Point& rhs) const;
};

using Vector = Point;

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