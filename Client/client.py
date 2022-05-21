from __future__ import annotations
import struct
from typing import Literal
import pygame
import socket
from io import BytesIO
from game_objects import *

class Game:
    def __init__(self):
        self.direction = Direction.NONE
        self.angle = 0
        self.player_radius = 10
        self.projectile_radius = 2
        self.run = True
        self.map = Map()
        self.game_state = GameState()
        self.my_own_id = -1
        self.s_connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            self.s_connection.connect(('localhost', 5000))
            self.connected = True
        except ConnectionRefusedError as e:
            print(e)
            self.connected = False

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.s_connection.close()
    
    def calculateOrientationAngle(self, player_position: Point, mouse_position: Point):
        return math.atan2(-(player_position.y - mouse_position.y), mouse_position.x - player_position.x)


    def change_movement(self, dir: Direction, add: bool):
        if add == True:
            self.direction = add_flag(self.direction, dir)
        else:
            self.direction = remove_flag(self.direction, dir)

        self.send_message(DataType.CHANGE_MOVEMENT_DIRECTION)

    def event_handler(self):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.run = False
            if event.type == pygame.MOUSEBUTTONDOWN:
                if event.button == 1:
                    # TODO pygame.BUTTON_LEFT doesn't work, make our own enums?
                    # TODO pygame detects only one click, change it somehow?
                    self.send_message(DataType.SHOOT)
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_LEFT or event.key == pygame.K_a:
                    self.change_movement(Direction.LEFT, True)
                if event.key == pygame.K_RIGHT or event.key == pygame.K_d:
                    self.change_movement(Direction.RIGHT, True)
                if event.key == pygame.K_UP or event.key == pygame.K_w:
                    self.change_movement(Direction.UP, True)
                if event.key == pygame.K_DOWN or event.key == pygame.K_s:
                    self.change_movement(Direction.DOWN, True)
                if event.key == pygame.K_SPACE:
                    self.send_message(DataType.SPAWN)

            if event.type == pygame.KEYUP:
                if event.key == pygame.K_LEFT or event.key == pygame.K_a:
                    self.change_movement(Direction.LEFT, False)
                if event.key == pygame.K_RIGHT or event.key == pygame.K_d:
                    self.change_movement(Direction.RIGHT, False)
                if event.key == pygame.K_UP or event.key == pygame.K_w:
                    self.change_movement(Direction.UP, False)
                if event.key == pygame.K_DOWN or event.key == pygame.K_s:
                    self.change_movement(Direction.DOWN, False)
                if event.key == pygame.K_ESCAPE:
                    self.run = False

    def start_game(self):
        pygame.init()
        display_width = 800
        display_height = 600

        self.display = pygame.display.set_mode((display_width, display_height))
        self.display.fill((255,255,255))

        pygame.display.set_caption('PR-Shooter')
        pygame.key.set_repeat()

        while self.run:
            self.event_handler()
            self.receive_message()
            self.drawGame()

        pygame.quit()

    def drawGame(self):
        for player in self.game_state.players:
            if player.alive == False:
                continue
            color = (255, 0, 0) if player.id != self.my_own_id else (0, 0, 255)
            pygame.draw.circle(self.display, color, player.position, self.player_radius)

        for projectile in self.game_state.projectiles:
            pygame.draw.circle(self.display, (255, 165, 0), projectile.position, self.projectile_radius)
        pygame.display.update()
        self.display.fill((255,255,255))


    def send_message(self, data_type: Literal[DataType.SPAWN, DataType.SHOOT, DataType.CHANGE_ORIENTATION, DataType.CHANGE_MOVEMENT_DIRECTION]):
        if self.connected == False:
            return
        if data_type == DataType.SPAWN or data_type == DataType.SHOOT:
            self.s_connection.send(bytes([1, 0, 0, 0, data_type]))
        elif data_type == DataType.CHANGE_ORIENTATION:
            self.s_connection.send(bytes([5, 0, 0, 0, data_type]) + struct.pack('f', self.angle))
        elif data_type == DataType.CHANGE_MOVEMENT_DIRECTION:
            msg = bytes([17, 0, 0, 0, data_type]) + struct.pack('d', directionToVelocity[self.direction].x) + struct.pack('d', directionToVelocity[self.direction].y)
            self.s_connection.send(msg)

    def read_int(num: BytesIO, length: Literal[1,2,4,8]):
        return int.from_bytes(num.read(length), 'little')

    def read_float(num: BytesIO, float_type: Literal['f', 'd']) -> float:
        buf = num.read(4) if float_type == 'f' else num.read(8)
        return struct.unpack(float_type, buf)[0]

    def read_point(point: BytesIO) -> Point:
        return Point(Game.read_float(point, 'd'), Game.read_float(point, 'd'))

    def receive_message(self):
        if self.connected is False:
            return
        val = []
        try:
            self.s_connection.setblocking(False)
            val = self.s_connection.recv(4)
        except socket.error as e:
            return
        finally:
            self.s_connection.setblocking(True)

        size = int.from_bytes(val, 'little')
        rest = BytesIO(self.s_connection.recv(size))
        type = Game.read_int(rest, 1)
        if type == DataType.WELCOME_MESSAGE:
            self.my_own_id = Game.read_int(rest, 2)
            self.player_radius = Game.read_float(rest, 'd')
            self.projectile_radius = Game.read_float(rest, 'd')
        elif type == DataType.GAME_MAP:
            self.map = Game.get_map_from_bytes(rest)
        elif type == DataType.GAME_STATE:
            self.game_state = Game.get_gamestate_from_bytes(rest)
            player = next((player for player in self.game_state.players if player.id == self.my_own_id), None)
            if player is not None and player.alive:
                # TODO should probably change this to something different
                angle = self.calculateOrientationAngle(player.position, Point(*pygame.mouse.get_pos()))
                if angle != self.angle:
                    self.angle = angle
                    player.orientation_angle = angle
                    self.send_message(DataType.CHANGE_ORIENTATION)
        else:
            print('Nie wiadomo co to za wiadomość')
            pass

    def get_map_from_bytes(map: BytesIO) -> Map:
        number_of_walls = Game.read_int(map, 2)
        number_of_obstacles = Game.read_int(map, 2)
        walls = [(Game.read_point(map), Game.read_point(map), Game.read_point(map), Game.read_point(map)) for _ in range(number_of_walls)]
        obstacles = [(Game.read_point(map), Game.read_float(map, 'd')) for _ in range(number_of_obstacles)]
        return Map(walls, obstacles)
    
    def get_gamestate_from_bytes(game_state: BytesIO) -> GameState:
        def readPlayer() -> Player:
            return Player(Game.read_int(game_state, 2), bool.from_bytes(game_state.read(1), 'little'),
                    Game.read_int(game_state, 1), Game.read_point(game_state),
                    Game.read_point(game_state), Game.read_float(game_state, 'f'))

        number_of_players = Game.read_int(game_state, 2)
        number_of_projectiles = Game.read_int(game_state, 2)
        players = [readPlayer() for _ in range(number_of_players)]
        """ FIXME sometimes got this error when tried to join as a second player with close to 250 projectiles
                    possible bug in client or host
        File "client.py", line 181, in get_gamestate_from_bytes
            projectiles = [Projectile(Game.read_int(game_state, 2), Game.read_point(game_state),
        File "client.py", line 182, in <listcomp>
            Game.read_point(game_state)) for _ in range(number_of_projectiles)]
        File "client.py", line 128, in read_point
            return Point(Game.read_float(point, 'd'), Game.read_float(point, 'd'))
        File "client.py", line 125, in read_float
            return struct.unpack(float_type, buf)[0]
        struct.error: unpack requires a buffer of 8 bytes
        """
        projectiles = [Projectile(Game.read_int(game_state, 2), Game.read_point(game_state), 
                                    Game.read_point(game_state)) for _ in range(number_of_projectiles)]
        return GameState(players, projectiles)

with Game() as game:
    game.start_game()