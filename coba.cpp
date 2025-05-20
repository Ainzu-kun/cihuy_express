#include <iostream>
#include <stack>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <unistd.h> // untuk sleep()

using namespace std;

// Ukuran peta
const int WIDTH = 30;
const int HEIGHT = 15;
char map[HEIGHT][WIDTH];

// Posisi awal kurir
int courierX = WIDTH / 2;
int courierY = HEIGHT / 2;

// Stack untuk membawa paket
stack<char> carriedPackages;

// Queue untuk tujuan antar paket
queue< pair<int, int> > deliveryQueue; // Perbaiki spasi di sini

// Skor
int score = 0;

// Fungsi untuk menghasilkan peta
void generateMap() {
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if (i == 0 || i == HEIGHT - 1 || j == 0 || j == WIDTH - 1) {
                map[i][j] = '#'; // Tembok pinggir
            } else {
                map[i][j] = ' '; // Jalan
            }
        }
    }

    // Menambahkan rumah (tujuan)
    map[HEIGHT - 2][WIDTH - 2] = 'H'; // Rumah

    // Menambahkan beberapa tembok acak di dalam map
    int wallCount = 20; // Jumlah tembok acak
    int wallAdded = 0;

    while (wallAdded < wallCount) {
        int x = rand() % (WIDTH - 2) + 1;
        int y = rand() % (HEIGHT - 2) + 1;

        // Pastikan tidak di posisi kurir atau rumah
        if (map[y][x] == ' ' && !(x == courierX && y == courierY) && map[y][x] != 'H') {
            map[y][x] = '#'; // Tembok dalam map
            wallAdded++;
        }
    }

    // Menambahkan beberapa paket secara acak
    int paketCount = 3; // Jumlah paket
    int added = 0;

    while (added < paketCount) {
        int x = rand() % (WIDTH - 2) + 1;
        int y = rand() % (HEIGHT - 2) + 1;

        // Pastikan bukan di atas rumah, kurir, atau tembok
        if (map[y][x] == ' ' && !(x == courierX && y == courierY)) {
            map[y][x] = 'P';
            added++;
        }
    }
}



// Fungsi untuk menampilkan peta
void printMap() {
    system("clear"); // Menghapus layar
    map[courierY][courierX] = 'C'; // Posisi kurir, diganti dengan simbol ASCII

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            cout << map[i][j] << " ";
        }
        cout << endl;
    }

    cout << "Skor: " << score << endl;
    cout << "Paket dibawa: " << carriedPackages.size() << "/3" << endl;
}

// Fungsi untuk kontrol arah
void moveCourier(char direction) {
    // Hitung posisi tujuan
    int nextX = courierX;
    int nextY = courierY;

    if (direction == 'w') nextY--;
    if (direction == 's') nextY++;
    if (direction == 'a') nextX--;
    if (direction == 'd') nextX++;

    // Cek apakah menabrak tembok
    if (map[nextY][nextX] == '#') {
        system("clear");
        cout << "💥 GAME OVER! Kamu menabrak tembok! 💥" << endl;
        cout << "Skor akhir: " << score << endl;
        exit(0); // Keluar dari program
    }

    // Hapus posisi lama kurir
    map[courierY][courierX] = ' ';

    // Update posisi kurir
    courierX = nextX;
    courierY = nextY;
}


// Fungsi untuk mengambil paket
void pickUpPackage() {
    if (carriedPackages.size() < 3) {
        if (map[courierY][courierX] == 'P') {
            carriedPackages.push('P');
            map[courierY][courierX] = ' '; // Hapus paket dari peta
        }
    }
}

// Fungsi untuk mengantar paket
void deliverPackage() {
    if (!carriedPackages.empty() && map[courierY][courierX] == 'H') {
        carriedPackages.pop();
        deliveryQueue.push(make_pair(courierY, courierX)); // Perbaiki sintaksis di sini
        score += 10; // Tambah skor setiap antar paket
    }
}

int main() {
    srand(time(0));
    generateMap();

    while (true) {
        printMap();
        char move;
        cin >> move; // Mengambil input arah

        moveCourier(move); // Menggerakkan kurir
        pickUpPackage(); // Ambil paket jika ada
        deliverPackage(); // Antar paket jika sudah sampai tujuan

        usleep(200000); // Kecepatan game (200ms)
    }

    return 0;
}
