# PR-Shooter  
Gra multiplayer + serwer na projekt z Przetwarzania Rozproszonego  
  
Kompilacja i uruchomienie serwera(tylko na Linuxie):  
`make run`  
  
Uruchomienie gry w folderze Client: (Python 3.7 lub większy + pygame, sprawdzane na Linuxie i Windows 10)  
`python client.py` - uruchomi klienta i połączy do serwera 'localhost'  
`python client.py xxx.xxx.xxx.xxx` - uruchomi klienta i połączy się z serwerem pod ip xxx.xxx.xxx.xxx  

Obsługa gry:  
  - spacja - respawn
  - LPM - strzał
  - WASD - ruch
  - ESC - wyjście

Obsługa serwera:
  - CTRL+C - serwer odbiera INTERRUPT SIGNAL i poprawnie się wyłącza po około sekundzie
