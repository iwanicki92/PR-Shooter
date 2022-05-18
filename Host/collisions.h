#pragma once
#include "basic_structs.hpp"

bool checkCollision(const Circle& c, const Rectangle& rec);
bool checkCollision(const Rectangle& rec, const Circle& c);
double distance(const Point& a, const Point& b);