#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>
#include <complex>
#include <csignal>
#include <mutex>
#include <condition_variable>
#include <iomanip>
#include <algorithm>
#include <numbers>

// Flag per il controllo della terminazione
std::atomic<bool> g_running{true};
std::mutex g_console_mutex;

// Per misurare il carico di lavoro
std::atomic<uint64_t> g_total_iterations{0};
std::chrono::time_point<std::chrono::steady_clock> g_start_time;

// Implementazione FFT ricorsiva (algoritmo Cooley-Tukey)
void fft(std::vector<std::complex<double>>& x) {
    const size_t N = x.size();
    if (N <= 1) return;

    // Divide
    std::vector<std::complex<double>> even(N/2), odd(N/2);
    for (size_t i = 0; i < N/2; i++) {
        even[i] = x[i*2];
        odd[i] = x[i*2+1];
    }

    // Ricorsione
    fft(even);
    fft(odd);

    // Combina
    for (size_t k = 0; k < N/2; k++) {
        std::complex<double> t = std::polar(1.0, -2 * std::numbers::pi * k / N) * odd[k];
        x[k] = even[k] + t;
        x[k + N/2] = even[k] - t;
    }
}

// Inversa FFT
void ifft(std::vector<std::complex<double>>& x) {
    // Coniuga l'input
    for (auto& val : x) {
        val = std::conj(val);
    }
    
    // Calcola FFT
    fft(x);
    
    // Coniuga e scala l'output
    for (auto& val : x) {
        val = std::conj(val) / static_cast<double>(x.size());
    }
}

// Funzione per generare dati casuali di test
std::vector<std::complex<double>> generate_test_data(size_t size) {
    std::vector<std::complex<double>> data(size);
    for (size_t i = 0; i < size; i++) {
        data[i] = std::complex<double>(std::sin(i * 0.1), std::cos(i * 0.1));
    }
    return data;
}

// Esegue un singolo ciclo di test per verificare la correttezza
bool verify_fft_implementation() {
    // Crea un vettore di test di piccole dimensioni
    const int testSize = 8;
    std::vector<std::complex<double>> test = {
        {1, 0}, {1, 0}, {1, 0}, {1, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
    };
    
    // Salva una copia dell'input
    auto original = test;
    
    // Calcola FFT
    fft(test);
    
    // Calcola IFFT
    ifft(test);
    
    // Verifica che l'output sia simile all'input originale
    double max_error = 0.0;
    for (size_t i = 0; i < testSize; i++) {
        double err = std::abs(test[i] - original[i]);
        max_error = std::max(max_error, err);
    }
    
    return max_error < 1e-10;
}

// Funzione che esegue il test di stress su un core
void stress_test_worker(int thread_id, int fft_size) {
    {
        std::lock_guard<std::mutex> lock(g_console_mutex);
        std::cout << "Thread " << thread_id << " avviato con FFT di dimensione " << fft_size << std::endl;
    }

    // Conta le iterazioni locali per questo thread
    uint64_t local_iterations = 0;

    // Continua a calcolare FFT fino a quando il flag g_running è true
    while (g_running) {
        auto data = generate_test_data(fft_size);
        
        // Calcola FFT
        fft(data);
        
        // Calcola IFFT
        ifft(data);
        
        // Incrementa il conteggio delle iterazioni
        local_iterations++;
        
        // Aggiorna il contatore globale ogni 10 iterazioni
        if (local_iterations % 10 == 0) {
            g_total_iterations += 10;
        }
    }

    // Aggiorna il conteggio finale
    g_total_iterations += (local_iterations % 10);
    
    {
        std::lock_guard<std::mutex> lock(g_console_mutex);
        std::cout << "Thread " << thread_id << " terminato dopo " << local_iterations << " iterazioni" << std::endl;
    }
}

// Gestore dei segnali (CTRL+C)
void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nTerminazione richiesta. Sto chiudendo i thread..." << std::endl;
        g_running = false;
    }
}

// Funzione per mostrare le statistiche di esecuzione
void display_stats() {
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        auto now = std::chrono::steady_clock::now();
        double seconds = std::chrono::duration<double>(now - g_start_time).count();
        uint64_t iterations = g_total_iterations.load();
        
        std::lock_guard<std::mutex> lock(g_console_mutex);
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Iterazioni totali: " << iterations 
                  << " (" << (iterations / seconds) << " it/s)" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // Verifica il numero di argomenti
    if (argc != 2) {
        std::cout << "Uso: " << argv[0] << " <numero_di_core>" << std::endl;
        return 1;
    }

    // Parsing del numero di thread
    int num_threads;
    try {
        num_threads = std::stoi(argv[1]);
        if (num_threads <= 0) throw std::out_of_range("Numero di core deve essere positivo");
    } catch (const std::exception& e) {
        std::cerr << "Errore: " << e.what() << std::endl;
        std::cout << "Uso: " << argv[0] << " <numero_di_core>" << std::endl;
        return 1;
    }

    // Limita il numero di thread ai core disponibili
    int max_threads = std::thread::hardware_concurrency();
    if (num_threads > max_threads) {
        std::cout << "Avviso: Il sistema supporta solo " << max_threads 
                  << " core. Limitando a questo valore." << std::endl;
        num_threads = max_threads;
    }

    // Verifica l'implementazione FFT
    if (!verify_fft_implementation()) {
        std::cerr << "Errore: Verifica dell'implementazione FFT fallita!" << std::endl;
        return 1;
    }

    // Registra il gestore dei segnali
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "Test di stress CPU avviato con " << num_threads << " thread" << std::endl;
    std::cout << "Premi Ctrl+C per terminare." << std::endl;

    // Container per i thread
    std::vector<std::thread> threads;

    // Avvia il timer
    g_start_time = std::chrono::steady_clock::now();

    // Crea thread di monitoraggio
    std::thread stats_thread(display_stats);

    // Dimensione FFT - usiamo dimensioni piccole come in Prime95 small FFT
    // Prime95 usa varie dimensioni FFT, ma per semplicità alterniamo tra queste
    const std::vector<int> fft_sizes = {8, 16, 32, 64, 128, 256, 512, 1024};

    // Avvia i thread dei worker
    for (int i = 0; i < num_threads; i++) {
        int fft_size = fft_sizes[i % fft_sizes.size()];
        threads.emplace_back(stress_test_worker, i, fft_size);
    }

    // Attendi che tutti i thread terminino
    for (auto& t : threads) {
        t.join();
    }

    // Terminazione del thread di monitoraggio
    stats_thread.join();

    // Calcola e mostra le statistiche finali
    auto end_time = std::chrono::steady_clock::now();
    double total_seconds = std::chrono::duration<double>(end_time - g_start_time).count();
    uint64_t total_iterations = g_total_iterations.load();

    std::cout << "\nStatistiche finali:" << std::endl;
    std::cout << "Tempo totale: " << total_seconds << " secondi" << std::endl;
    std::cout << "Iterazioni totali: " << total_iterations << std::endl;
    std::cout << "Velocità media: " << (total_iterations / total_seconds) << " it/s" << std::endl;

    std::cout << "Test di stress CPU terminato." << std::endl;
    return 0;
}