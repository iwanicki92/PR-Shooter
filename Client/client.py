from keyboard import Game


class Game_settings():

    def __init__(self, size, type_of_message, n, m):
        self.size = size
        self.type_of_message = type_of_message
        self.n = n
        self.m = m
        self.projectiles: list[Projectile] = []
        self.players: list[Player] = []
        self.list_of_walls: list[list[tuple[float, float]]] = []
        self.list_of_obstacles: list[tuple[float, float, float]] = []

class Player:

    def __init__(self, ident, health, x_pos, y_pos, velocity_x, velocity_y, angle_of_watching):
        self.ident = ident
        self.health = health
        self.x_pos = x_pos
        self.y_pos = y_pos
        self.velocity_x = velocity_x
        self.velocity_y = velocity_y
        self.angle_of_watching = angle_of_watching


class Projectile:
    def __init__(self, ident, x_pos, y_pos, velocity_x, velocity_y):
        self.id = ident
        self.x_pos = x_pos
        self.y_pos = y_pos
        self.velocity_x = velocity_x
        self.velocity_y = velocity_y



class Message:
    #def __init__(self, send_map, spawn, change_orientation, change_movement, shoot, change):
        pass