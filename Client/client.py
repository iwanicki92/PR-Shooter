class Game:

    def __init__(self, size, type_of_message, n, m):
        self.size = size
        self.type_of_message = type_of_message
        self.n = n
        self.m = m


class Player:

    def __init__(self, id, health, x_pos, y_pos, speed, angle_of_movement, angle_of_watching):
        self.id = id
        self.health = health
        self.x_pos = x_pos
        self.y_pos = y_pos
        self.speed = speed
        self.angle_of_movement = angle_of_movement
        self.angle_of_watching = angle_of_watching


class Projectile:
    pass


