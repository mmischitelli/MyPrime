# Test di Stress CPU

[![CMake on multiple platforms](https://github.com/mmischitelli/MyPrime/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/mmischitelli/MyPrime/actions/workflows/cmake-multi-platform.yml)

Questo programma simula un test di stress per CPU simile a Prime95 con l'opzione "small FFT". È progettato per eseguire calcoli intensivi su un numero specificato di core utilizzando l'algoritmo FFT (Fast Fourier Transform).

## Caratteristiche

- Utilizza un numero specificato di thread per sfruttare tutti i core disponibili
- Implementa l'algoritmo Cooley-Tukey FFT per calcoli intensivi
- Ogni thread esegue calcoli FFT su piccole dimensioni (8-1024 punti)
- Fornisce statistiche in tempo reale sulle prestazioni
- Gestisce correttamente la terminazione quando richiesto dall'utente

## Compilazione

Per compilare il programma:
