from __future__ import annotations
from collections import namedtuple
from enum import Flag, IntEnum, auto
import math

Point = namedtuple('Point', ['x', 'y'], defaults=[0,0])
Vector = Point

def add_points(a: Point, b: Point) -> Point:
    return Point(a.x + b.x, a.y + b.y)

def sub_points(a: Point, b: Point) -> Point:
    return Point(a.x - b.x, a.y - b.y)

class Direction(Flag):
    LEFT = auto()
    UP = auto()
    RIGHT = auto()
    DOWN = auto()
    NONE = LEFT ^ LEFT

def add_flag(flag_a: Direction, flag_b: Direction) -> Direction:
    return flag_a | flag_b

def remove_flag(flag_a: Direction, flag_b: Direction) -> Direction:
    return flag_a & ~flag_b

directionToVelocity: dict[Direction, Vector] = {
    Direction.NONE:  Vector(0, 0),
    Direction.UP:    Vector(0, -1),
    Direction.LEFT:  Vector(-1, 0),
    Direction.RIGHT: Vector(1, 0),
    Direction.DOWN:  Vector(0, 1),

    Direction.LEFT  | Direction.UP :   Vector(-math.sqrt(2)/2, -math.sqrt(2)/2),
    Direction.RIGHT | Direction.UP :   Vector(math.sqrt(2)/2, -math.sqrt(2)/2),
    Direction.RIGHT | Direction.DOWN : Vector(math.sqrt(2)/2, math.sqrt(2)/2),
    Direction.LEFT  | Direction.DOWN : Vector(-math.sqrt(2)/2, math.sqrt(2)/2),
    Direction.LEFT  | Direction.RIGHT: Vector(0, 0),
    Direction.UP    | Direction.DOWN:  Vector(0, 0),

    Direction.UP | Direction.RIGHT | Direction.LEFT:   Vector(0, -1),
    Direction.DOWN | Direction.LEFT | Direction.RIGHT: Vector(0, 1),
    Direction.DOWN | Direction.UP | Direction. RIGHT:  Vector(1, 0),
    Direction.DOWN | Direction.UP | Direction. LEFT:   Vector(-1, 0),

    Direction.UP | Direction.DOWN | Direction.LEFT | Direction.RIGHT: Vector(0, 0),
}

class DataType(IntEnum):
    # INCOMING
    WELCOME_MESSAGE = 0,
    GAME_MAP = 1,
    GAME_STATE = 2,

    # OUTGOING
    SPAWN = 10,
    SHOOT = 11,
    CHANGE_ORIENTATION = 12,
    CHANGE_MOVEMENT_DIRECTION = 13,

    OTHER = 999
    
class Circle:
    def __init__(self, position: Point, radius: float) -> None:
        self.position = position
        self.radius = radius

class Polygon:
    def __init__(self, vertices: list[Point]):
        self.vertices = vertices

class Rectangle:
    def __init__(self, P1: Point, P2: Point, P3: Point, P4: Point) -> None:
        self.vertices = (P1, P2, P3, P4)

class Map:
    def __init__(self, walls: list[Rectangle] = [], obstacles: list[Circle] = [], border: list[Point] = []) -> None:
        # wall = rectangle: ((X1, Y1), (X2, Y2), (X3, Y3), (X4, Y4))
        self.walls: list[Rectangle] = walls
        # obstacle = circle: ((X, Y), R)
        self.obstacles: list[Circle] = obstacles
        self.border: list[Point] = border

    def __str__(self) -> str:
        return self.walls.__str__() + '\n' + self.obstacles.__str__()


class Player:
    def __init__(self, id, alive, health, position: Point, velocity: Vector, orientation_angle: float):
        self.id: int = id
        self.alive: bool = alive
        self.health: int = health
        self.position: Point = position
        self.velocity: Vector = velocity
        self.orientation_angle: float = orientation_angle
        self.health_ratio = health / 100  # 100 = max_health

    def __str__(self) -> str:
        return f"""id={self.id}, alive={self.alive}, health={self.health}, position={self.position}, velocity={self.velocity}, orientation_angle={self.orientation_angle}."""

    def __repr__(self) -> str:
        return (self.id, self.alive, self.health, self.position, self.velocity, self.orientation_angle).__str__()

class Projectile:
    def __init__(self, owner_id, position: Point, velocity: Vector):
        self.owner_id: int = owner_id
        self.position: Point = position
        self.velocity: Vector = velocity

    def __str__(self) -> str:
        return f'Projectile: {(self.owner_id, self.position, self.velocity)}'

    def __repr__(self) -> str:
        return (self.owner_id, self.position, self.velocity).__str__()

class GameState:
    def __init__(self, players: list[Player] = [], projectiles: list[Projectile] = []) -> None:
        self.players: list[Player] = players
        self.projectiles: list[Projectile] = projectiles

    def __str__(self) -> str:
        return f'GameState:\nPlayers    : {self.players.__str__()}\nProjectiles: {self.projectiles.__str__()}'


    

