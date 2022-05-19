import pygame
import math
from enum import Flag, auto


class Direction(Flag):

    LEFT = auto()
    UP = auto()
    RIGHT = auto()
    DOWN = auto()


dictionary = {
    Direction.UP: (0, -1),
    Direction.LEFT: (-1, 0),
    Direction.RIGHT: (1, 0),
    Direction.DOWN: (0, 1),

    Direction.LEFT | Direction.UP: (-math.sqrt(2)/2, -math.sqrt(2)/2),
    Direction.UP | Direction.RIGHT: (math.sqrt(2)/2, -math.sqrt(2)/2),
    Direction.RIGHT | Direction.DOWN: (math.sqrt(2)/2, math.sqrt(2)/2),
    Direction.DOWN | Direction.LEFT: (-math.sqrt(2)/2, -math.sqrt(2)/2),
    Direction.LEFT | Direction.RIGHT: (0, 0),
    Direction.UP | Direction.DOWN: (0, 0),

    Direction.UP | Direction.RIGHT | Direction.LEFT: (0, -1),
    Direction.DOWN | Direction.LEFT | Direction.RIGHT: (0, 1),
    Direction.DOWN | Direction.UP | Direction. RIGHT: (1, 0),
    Direction.DOWN | Direction.UP | Direction. LEFT: (-1, 0),

    Direction.UP | Direction.DOWN | Direction.LEFT | Direction.RIGHT: (0, 0),

    Direction.LEFT ^ Direction.LEFT: (0, 0)

}


def add_flag(flag_a, flag_b):
    return flag_a | flag_b


def remove_flag(flag_a, flag_b):
    return flag_a & ~flag_b


class Game:

    def __init__(self):
        self.direction = Direction.LEFT ^ Direction.LEFT

        self.run = True


    def changeMovement(self, dir: Direction, add: bool):
        if add == True:
            self.direction = add_flag(self.direction, dir)
        else:
            self.direction = remove_flag(self.direction, dir)

    def event_handler(self):

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.run = False
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_LEFT:
                    self.changeMovement(Direction.LEFT, True)
                if event.key == pygame.K_RIGHT:
                    self.changeMovement(Direction.RIGHT, True)
                if event.key == pygame.K_UP:
                    self.changeMovement(Direction.UP, True)
                if event.key == pygame.K_DOWN:
                    self.changeMovement(Direction.DOWN, True)

            if event.type == pygame.KEYUP:
                if event.key == pygame.K_LEFT:
                    self.changeMovement(Direction.LEFT, False)
                if event.key == pygame.K_RIGHT:
                    self.changeMovement(Direction.RIGHT, False)
                if event.key == pygame.K_UP:
                    self.changeMovement(Direction.UP, False)
                if event.key == pygame.K_DOWN:
                    self.changeMovement(Direction.DOWN, False)


    def start_game(self):
        pygame.init()
        display_width = 500
        display_height = 500

        result = pygame.display.set_mode((display_width, display_height))

        pygame.display.set_caption('PR-Shooter')
        x = 80
        y = 80
        width = 20
        height = 20
        value = 3

        pygame.key.set_repeat()

        delay = 16
        player_speed = 100
        while self.run:
            self.event_handler()
            pygame.time.delay(delay)

            move_x, move_y = dictionary[self.direction]
            x += move_x * player_speed / delay
            y += move_y * player_speed / delay

            pygame.draw.rect(result, (0, 0, 255), (x, y, width, height))
            pygame.display.update()

            result.fill((0, 0, 0))
        pygame.quit()

game = Game()

game.start_game()




