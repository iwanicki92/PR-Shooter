import math
from game_objects import Point

def add_points(a: Point, b: Point) -> Point:
    return Point(a.x + b.x, a.y + b.y)

def sub_points(a: Point, b: Point) -> Point:
    return Point(a.x - b.x, a.y - b.y)

def rotate_polygon( points, angle, pivot):
    """
    :param points: list of points representing polygon vertices
    :param angle: angle of rotation (radians)
    :param pivot: pivot point (around which we rotate the polygon)
    """
    new_points = []
    sine = math.sin(angle)
    cosine = math.cos(angle)
    for point in points:
        temp_x = point.x
        temp_y = point.y

        temp_x -= pivot.x
        temp_y -= pivot.y

        new_x = temp_x * cosine - temp_y * sine
        new_y = temp_x * sine + temp_y * cosine

        new_x += pivot.x
        new_y += pivot.y

        new_points.append(Point(new_x, new_y))

    return new_points

def translate_polygon( points, vector):
    """
    :param points: list of points representing polygon vertices
    :param vector: translation vector

    This method ADDS vector's value to all points
    """
    new_poly = []
    for point in points:
        temp_x = point.x
        temp_y = point.y
        temp_x += vector.x
        temp_y += vector.y
        new_poly.append(Point(temp_x, temp_y))
    return new_poly

def average(x) -> float:
    return sum(x) / len(x)