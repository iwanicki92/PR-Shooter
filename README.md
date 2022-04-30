# PR-Shooter
Gra multiplayer + serwer na projekt z Przetwarzania Rozproszonego

Do ułatwienia kompilacji/uruchamiania można używać przygotowanego makefile. Jeżeli nie napisano inaczej to make kompiluje tylko pliki które się zmieniły  
`make run` - uruchamia ./bin/host.  
`make rebuild` - usuwa wszystkie pliki z folderów ./bin/ i ./obj/ i buduje wszystko od nowa  
`make host` - buduje ./bin/host  
`make server` - kompiluje pliki .c do plików .o  

Dodatkowe argumenty by zmienić działanie make:  
- DEBUG=FALSE wyłącza opcje pomagające przy debugowaniu głównie dodatkowe opcje kompilatora do sprawdzania czy są wycieki pamięci albo dostęp do niealokowanej pamięci.  
- EXTENDED_FLAGS=TRUE włącza mnóstwo dodatkowych ostrzeżeń kompilatora i traktuje większość jako błędy.  
- PRINT_DEBUG=x gdzie 'x' to liczba. Wyświetla dodatkowe informacje w zależności od x aktualnie:  
    - x=0 - brak dodatkowych informacji  
    - x=1 - wypisuje informacje podczas blokowania/odblokowywania mutexów(wątek i mutex)  
    - x=2 - wypisuje dodatkowo zawartość tablicy synchronicznej podczas blokowania dostępu do niej  
    - x=3 - wypisuje dodatkowo informacje o dostępie do synchronicznej tablicy(tylko niektóre funkcje)  
    