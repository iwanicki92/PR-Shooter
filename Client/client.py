from __future__ import annotations
from collections import deque
import math
import struct
import time
import pygame
import socket
from typing import Literal
from io import BytesIO
from game_objects import *
from drawing_utils import *

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
        self.draw_offset: Point = Point(0, 0)
        self.s_connection = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.background_color = (255,255,255)
        self.display_scores = False
        self.latency: deque[float] = deque([0], maxlen=15)
        self.last_ping: tuple[int, float] = 0, time.perf_counter()
        self.ping_period = 0.2

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
            if event.type == pygame.VIDEORESIZE:
                self.display_width = event.w
                self.display_height = event.h
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
                if event.key == pygame.K_TAB:
                    self.display_scores = True

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
                if event.key == pygame.K_TAB:
                    self.display_scores = False

    def start_game(self):
        pygame.init()

        self.display = pygame.display.set_mode((800,600), pygame.RESIZABLE)
        self.display_width, self.display_height = self.display.get_size()
        self.display.fill((255,255,255))

        pygame.display.set_caption('PR-Shooter')
        pygame.key.set_repeat()

        total_event_handler, no_event_handler = 0,0
        total_receive, no_receive = 0,0
        total_draw, no_draw = 0,0
        print_time = time.perf_counter()

        while self.run:
            start = time.perf_counter_ns()
            self.event_handler()
            total_event_handler += time.perf_counter_ns() - start
            no_event_handler +=1

            start = time.perf_counter_ns()
            self.receive_message()
            total_receive += time.perf_counter_ns() - start
            no_receive +=1

            start = time.perf_counter_ns()
            self.draw_game()
            total_draw += time.perf_counter_ns() - start
            no_draw += 1

            if time.perf_counter() - self.last_ping[1] > self.ping_period:
                self.send_message(DataType.PING)

            if time.perf_counter() - print_time > 2:
                print('Average event handler time: '.rjust(30) + f'{total_event_handler / (no_event_handler * 1e6):10.04f} ms')
                print('Average message receive time: '.rjust(30) + f'{total_receive / (no_receive * 1e6):10.04f} ms')
                print('Average draw time: '.rjust(30) + f'{total_draw / (no_draw * 1e6):10.04f} ms\n')
                print_time = time.perf_counter()

        pygame.quit()

    def draw_game(self):
        #draw projectiles
        for projectile in self.game_state.projectiles:
            pygame.draw.circle(self.display, (255, 165, 0), sub_points(projectile.position, self.draw_offset), self.projectile_radius)
        #draw obstacles
        for obstacle in self.map.obstacles:
            pygame.draw.circle(self.display, (0,0,0), sub_points(obstacle.position, self.draw_offset), obstacle.radius)
        #draw walls
        for wall in self.map.walls:
            pygame.draw.polygon(self.display, (0,0,0), [sub_points(vertex, self.draw_offset) for vertex in wall.vertices])
        #draw map border
        if len(self.map.border) > 1:
            pygame.draw.polygon(self.display, (0,0,0), [sub_points(point, self.draw_offset) for point in self.map.border], 5)
        #draw alive players
        for player in self.game_state.players:
            if player.alive == False:
                continue
            self.draw_player(player)
        #display scores if requested
        if self.display_scores:
            self.draw_scores()

        pygame.display.update()
        self.display.fill(self.background_color)

    def draw_scores(self):
        # background (semi-transaparent, grey rectangle)
        scores_background = (
        self.display.get_size()[0] / 3, self.display.get_size()[1] / 4, self.display.get_size()[0] / 3,
        self.display.get_size()[1] / 3)
        rect_alpha = pygame.Surface(pygame.Rect(scores_background).size, pygame.SRCALPHA)
        pygame.draw.rect(rect_alpha, (105, 105, 105, 155), rect_alpha.get_rect())
        # Text
        text_color = (105, 105, 105)

        # latency
        font_latency = pygame.font.Font('freesansbold.ttf', 20)
        latency_text = font_latency.render(f'latency: {average(self.latency) * 1000 : .02f} ms', True, text_color)
        latency_text_rect = latency_text.get_rect()
        latency_text_rect.center = (self.display.get_size()[0] / 3 * 1.15, self.display.get_size()[1] / 4 + 40)

        self.display.blit(latency_text, latency_text_rect)

        # title
        font_title = pygame.font.Font('freesansbold.ttf', 32)
        title = font_title.render('Scores:', True, text_color)
        titleRect = title.get_rect()
        titleRect.center = (self.display.get_size()[0] / 2, self.display.get_size()[1] / 4 + 64)

        self.display.blit(title, titleRect)

        # player scores
        font_scores = pygame.font.Font('freesansbold.ttf', 20)

        # list of players sorted descending by their KD
        players_sorted_by_score = sorted(self.game_state.players, key=lambda x: (x.kills / (x.deaths + 1)),
                                         reverse=True)
        # displaying top5 players by KD
        for idx, player in enumerate(players_sorted_by_score[:5]):
            # calculate KD
            KD = player.kills / (player.deaths + 1)
            # different color for your score
            text_color = (105, 105, 105) if player.id != self.my_own_id else (0, 0, 255)
            player_score_text = font_scores.render(
                f'{idx + 1}. {player.name}: kills - {player.kills}, deaths - {player.deaths}, KD - {KD}', True,
                text_color)
            player_score_rect = player_score_text.get_rect()
            player_score_rect.center = (
            self.display.get_size()[0] / 2, self.display.get_size()[1] / 4 + 64 + (idx + 1) * 30)
            self.display.blit(player_score_text, player_score_rect)

            self.display.blit(rect_alpha, scores_background)
        return

    def draw_weapon(self, player):
        """
        :param player: which player weapon are we drawing
        """
        # Weapon size and offset
        weapon_len = 40
        weapon_width = 7
        weapon_side_offset = -0.5 * weapon_width  # 0.9 * self.player_radius - jeżeli chcemy mieć z boku
        weapon_vertical_offset = 0

        # Polygon representing a weapon
        weapon_poly = [Point(player.position.x - weapon_vertical_offset, player.position.y - weapon_side_offset),
                       Point(player.position.x - weapon_vertical_offset,
                             player.position.y - weapon_side_offset - weapon_width),
                       Point(player.position.x + weapon_len - weapon_vertical_offset,
                             player.position.y - weapon_side_offset - weapon_width),
                       Point(player.position.x + weapon_len - weapon_vertical_offset,
                             player.position.y - weapon_side_offset)]

        # pivot point and angle of rotation definition
        pivot_point = player.position
        rotation_angle = player.orientation_angle

        # polygon rotation
        weapon_poly = rotate_polygon(weapon_poly, rotation_angle, pivot_point)

        # polygon translation (draw_offset)
        weapon_poly = translate_polygon(weapon_poly, Point(-self.draw_offset.x, -self.draw_offset.y))

        # draw weapon
        pygame.draw.polygon(self.display, (0, 0, 0), weapon_poly)

    def draw_player(self, player):
        """
        :param player: player we want to draw
        """
        #choose a color blue if yourslef, red if enemy
        color = (255, 0, 0) if player.id != self.my_own_id else (0, 0, 255)

        pygame.draw.circle(self.display, color, sub_points(player.position, self.draw_offset), self.player_radius)
        color = (0, 0, 0)
        pygame.draw.circle(self.display, color, sub_points(player.position, self.draw_offset), self.player_radius, 3)

        # hp_bar (color different if enemy
        color_hp_bar = (255, 0, 0) if player.id != self.my_own_id else (0, 255, 0)
        pygame.draw.rect(self.display, (64, 64, 64), pygame.Rect(player.position.x - self.draw_offset.x - 50,
                                                                 player.position.y - self.draw_offset.y - 55, 100,
                                                                 15))  # background
        pygame.draw.rect(self.display, color_hp_bar, pygame.Rect(player.position.x - self.draw_offset.x - 50,
                                                                 player.position.y - self.draw_offset.y - 55,
                                                                 100 * player.health_ratio, 15))  # health_bar
        pygame.draw.rect(self.display, (0, 0, 0), pygame.Rect(player.position.x - self.draw_offset.x - 50,
                                                              player.position.y - self.draw_offset.y - 55, 100, 15),2)  # border
        #draw player's weapon
        self.draw_weapon(player)

    def send_message(self, data_type: DataType):
        if self.connected == False:
            return
        if data_type == DataType.SPAWN or data_type == DataType.SHOOT:
            self.s_connection.send(bytes([1, 0, 0, 0, data_type]))
        elif data_type == DataType.CHANGE_ORIENTATION:
            self.s_connection.send(bytes([5, 0, 0, 0, data_type]) + struct.pack('f', self.angle))
        elif data_type == DataType.CHANGE_MOVEMENT_DIRECTION:
            msg = bytes([17, 0, 0, 0, data_type]) + struct.pack('d', directionToVelocity[self.direction].x) + struct.pack('d', directionToVelocity[self.direction].y)
            self.s_connection.send(msg)
        elif data_type == DataType.PING:
            msg = bytes([3,0,0,0, data_type]) + (self.last_ping[0] + 1).to_bytes(2, 'little')
            self.s_connection.send(msg)
            self.last_ping = self.last_ping[0] + 1, time.perf_counter()

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
        elif type == DataType.PING:
            self.latency.append(time.perf_counter() - self.last_ping[1] + (self.last_ping[0] - Game.read_int(rest, 2)) * self.ping_period)
        elif type == DataType.GAME_STATE:
            self.game_state = Game.get_gamestate_from_bytes(rest)
            player = next((player for player in self.game_state.players if player.id == self.my_own_id), None)
            if player is not None and player.alive:
                self.draw_offset = add_points(player.position, Point(-self.display_width / 2, -self.display_height / 2))
                # TODO should probably change this to something different
                angle = self.calculateOrientationAngle(sub_points(player.position, self.draw_offset), Point(*pygame.mouse.get_pos()))
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
        number_of_border_points = Game.read_int(map, 2)
        walls = [Rectangle(Game.read_point(map), Game.read_point(map), Game.read_point(map), Game.read_point(map)) for _ in range(number_of_walls)]
        obstacles = [Circle(Game.read_point(map), Game.read_float(map, 'd')) for _ in range(number_of_obstacles)]
        border = [Game.read_point(map) for _ in range(number_of_border_points)]
        return Map(walls, obstacles, border)
    
    def get_gamestate_from_bytes(game_state: BytesIO) -> GameState:
        def readPlayer() -> Player:
            return Player(Game.read_int(game_state, 2), bool.from_bytes(game_state.read(1), 'little'),
                    Game.read_int(game_state, 1), Game.read_point(game_state),
                    Game.read_point(game_state), Game.read_float(game_state, 'f'),
                    Game.read_int(game_state, 2), Game.read_int(game_state, 2))

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