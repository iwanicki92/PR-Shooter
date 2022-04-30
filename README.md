# PR-Shooter
Gra multiplayer + serwer na projekt z Przetwarzania Rozproszonego

Do ułatwienia kompilacji/uruchamiania można używać przygotowanego makefile. Jeżeli nie napisano inaczej to make kompiluje tylko pliki które się zmieniły
`make run` - uruchamia ./bin/host.
`make rebuild` - usuwa wszystkie pliki z folderów ./bin/ i ./obj/ i buduje wszystko od nowa
`make host` - buduje ./bin/host
`make server` - kompiluje pliki .c do plików .o