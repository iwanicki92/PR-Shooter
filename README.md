# PR-Shooter
Gra multiplayer + serwer na projekt z Przetwarzania Rozproszonego

Do ułatwienia kompilacji/uruchamiania można używać przygotowanego makefile. Jeżeli nie napisano inaczej to make kompiluje tylko pliki które się zmieniły  
`make run` - uruchamia ./bin/host.  
`make rebuild` - usuwa wszystkie pliki z folderów ./bin/ i ./obj/ i buduje wszystko od nowa  
`make host` - buduje ./bin/host  
`make server` - kompiluje pliki .c do plików .o  
`make build_test` - buduje ./bin/test_host, ./bin/test_client oraz ./bin/test  
`make test` - uruchamia test - serwer + 4 klientów na kilka sekund  

`./bin/test [sekundy [klienci]]`  
- sekundy - Jak długo ma pracować test(liczba sekund), -1 bez limitu(przerwać CTRL+C)  
- klienci - liczba test_clientów do podłączenia się  
  
`./bin/test_client [ip [port]]`  
- ip - ip serwera, localhost jeżeli na tym samym
- port - port serwera

Dodatkowe argumenty by zmienić działanie make:  
- DEBUG=FALSE wyłącza opcje pomagające przy debugowaniu głównie dodatkowe opcje kompilatora do sprawdzania czy są wycieki pamięci albo dostęp do niealokowanej pamięci.  
- EXTENDED_FLAGS=TRUE włącza mnóstwo dodatkowych ostrzeżeń kompilatora i traktuje większość jako błędy.  
- PRINT_DEBUG=x gdzie 'x' to liczba. Wyświetla dodatkowe informacje w zależności od x aktualnie:
    - x=-1 - nie powinno wypisywać żadnych informacji debug
    - x=0 - wypisuje id nowo tworzonych wątków i przez kogo
    - x=1 - wypisuje informacje podczas blokowania/odblokowywania mutexów(wątek i mutex)  
    - x=2 - wypisuje dodatkowo zawartość tablicy synchronicznej podczas blokowania dostępu do niej  
    - x=3 - wypisuje dodatkowo informacje o dostępie do synchronicznej tablicy(tylko niektóre funkcje)  
    