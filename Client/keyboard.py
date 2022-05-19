key_dictionary = {
    65: ["A", 1],
    68: ["D", 2],
    83: ["S", 3],
    87: ["W", 4]
}


class KeyboardFunctions:
    def __repr__(self):
        return f"Keystrokes..."


    def setting_game_thread(self, game_thread):
        self.game_thread = game_thread


    def key_board(self, other):
        if self.game_thread is not None:
            pressed = other.key
            #print(key_dictionary.keys())
            if pressed in key_dictionary.keys():
                self.game_thread.pressed.append(key_dictionary[pressed][1])






