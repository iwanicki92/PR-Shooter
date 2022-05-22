#pragma once
#include "basic_structs.hpp"

bool checkCollision(const Circle& c1, const Circle& c2);
bool checkCollision(const Circle& c, const Rectangle& rec);
bool checkCollision(const Rectangle& rec, const Circle& c);
bool checkCollision(const Circle& c, const Point& P1, const Point& P2);
// height falling from point a onto bc segment
double triangleHeight(const Point& a, const Point& b, const Point& c);
double distance(const Point& a, const Point& b);
double square(double x);