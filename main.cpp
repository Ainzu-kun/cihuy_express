#include <iostream>
#include <stack>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <set>
#include <fstream>
#include <regex>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif
// #include <unistd.h>

using namespace std;

// Ukuran peta
const int WIDTH = 30;
const int HEIGHT = 15;
char game_map[HEIGHT][WIDTH];

// Posisi awal kurir
int courierX = WIDTH / 2;
int courierY = HEIGHT / 2;

stack<char> carriedPackages; // Stack untuk membawa paket
set<string> registered_users; // save registered user
set<pair<int, int>> house_loc; // house location

// Queue untuk tujuan antar paket
queue< pair<int, int> > deliveryQueue; // Perbaiki spasi di sini

// Skor
int score = 0;

void load_users() {
    ifstream infile("users.txt");

    string username;
    while(infile >> username) {
        registered_users.insert(username);
    }
}

void save_user(const string &username) {
    ofstream outfile("users.txt", ios::app);
    outfile << username << endl;
}

void login_or_regis() {
    load_users();
    string username;
    regex username_pattern("^[A-Za-z0-9_]{3,}$");

    while (true) {
        cout << "Masukkan username Anda (huruf, angka, underscore, min 3 karakter): "; cin >> username;

        if (regex_match(username, username_pattern)) {
            break;
        } else {
            cout << "Username tidak valid. Silakan coba lagi.\n";
        }
    }

    if(registered_users.count(username)) {
        cout << "Selamat datang kembali " << username << "! Selamat bermain!" << endl;
    } else {
        cout << "Registrasi baru untuk " << username << " berhasil! Selamat bermain!" << endl;
        save_user(username);
    }

    #ifdef _WIN32
        system("pause");
    #else
        cout << "Tekan ENTER untuk mulai...";
        cin.ignore();
        cin.get();
    #endif
}

void show_animation() {
    const string frames[] = {
        R"( 
  🎉 Horeee!!! Paket berhasil diantar! 🎉
     \(^_^)/ 
        |
       / \
        )",

        R"(
  🎊 Paket diterima dengan bahagia! 🎊
     (^o^)/
      \|
      / \
        )",

        R"(
  ✨ Terima kasih, kurir hebat! Cihuyyyy!! ✨
     (^-^)b
        |
       / \
        )"
    };

    for (int i = 0; i < 3; ++i) {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif

        cout << frames[i] << endl;
        cout << "\nSkor: " << score << endl;

        #ifdef _WIN32
            Sleep(1000);
        #else
            usleep(1000000); // 1 detik per frame
        #endif
    }
}


// Fungsi untuk menghasilkan peta
void generateMap() {
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if (i == 0 || i == HEIGHT - 1 || j == 0 || j == WIDTH - 1) {
                game_map[i][j] = '#'; // Tembok pinggir
            } else {
                game_map[i][j] = ' '; // Jalan
            }
        }
    }

    // Menambahkan rumah (tujuan)
    int hx = WIDTH - 2, hy = HEIGHT - 2;
    game_map[hy][hx] = 'H';
    house_loc.insert({hy, hx});

    // Menambahkan beberapa tembok acak di dalam map
    int wallCount = 20; // Jumlah tembok acak
    int wallAdded = 0;

    while (wallAdded < wallCount) {
        int x = rand() % (WIDTH - 2) + 1;
        int y = rand() % (HEIGHT - 2) + 1;

        // Pastikan tidak di posisi kurir atau rumah
        if (game_map[y][x] == ' ' && !(x == courierX && y == courierY) && game_map[y][x] != 'H') {
            game_map[y][x] = '#'; // Tembok dalam map
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
        if (game_map[y][x] == ' ' && !(x == courierX && y == courierY)) {
            game_map[y][x] = 'P';
            added++;
        }
    }
}

// Fungsi untuk menampilkan peta
void printMap() {

    #ifdef _WIN32
        system("cls");
    #else
        system("clear"); // Menghapus layar
    #endif

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if (i == courierY && j == courierX) {
                cout << 'C' << " "; // Prioritaskan tampilan kurir
            } else if (house_loc.count({i, j})) {
                cout << 'H' << " "; // Tetap tampilkan rumah
            } else {
                cout << game_map[i][j] << " ";
            }
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
    if (game_map[nextY][nextX] == '#') {

        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif

        cout << "💥 GAME OVER! Kamu menabrak tembok! 💥" << endl;
        cout << "Skor akhir: " << score << endl;
        exit(0); // Keluar dari program
    }

    // Hapus posisi lama kurir
    game_map[courierY][courierX] = ' ';

    // Update posisi kurir
    courierX = nextX;
    courierY = nextY;
}


// Fungsi untuk mengambil paket
void pickUpPackage() {
    if (carriedPackages.size() < 3) {
        if (game_map[courierY][courierX] == 'P') {
            carriedPackages.push('P');
            game_map[courierY][courierX] = ' '; // Hapus paket dari peta
        }
    }
}

// Fungsi untuk mengantar paket
void deliverPackage() {
    pair<int, int> pos = {courierY, courierX};

    if(house_loc.count(pos)) {
        if(!carriedPackages.empty()) {
            carriedPackages.pop();
            deliveryQueue.push(pos);
            score += 10;

            show_animation();

            game_map[courierY][courierX] = ' '; // delete old house
            house_loc.erase(pos);

            // add new house
            while (true) {
                int x = rand() % (WIDTH - 2) + 1;
                int y = rand() % (HEIGHT - 2) + 1;

                if (game_map[y][x] == ' ' && !(x == courierX && y == courierY)) {
                    game_map[y][x] = 'H';
                    house_loc.insert({y, x});
                    break;
                }
            }
        }
    }
    // if (game_map[courierY][courierX] == 'H') {
    //     if (!carriedPackages.empty()) {
    //         carriedPackages.pop();
    //         deliveryQueue.push(make_pair(courierY, courierX));
    //         score += 10;

    //         game_map[courierY][courierX] = ' '; // delete old house

    //         // add new house at random loc
    //         while (true) {
    //             int x = rand() % (WIDTH - 2) + 1;
    //             int y = rand() % (HEIGHT - 2) + 1;

    //             if (game_map[y][x] == ' ' && !(x == courierX && y == courierY)) {
    //                 game_map[y][x] = 'H';
    //                 break;
    //             }
    //         }
    //     }
    //     // if courier doesn't bring the package, the house remains there, doing nothing.
    // }
}

int main() {
    login_or_regis();
    srand(time(0));
    generateMap();

    while (true) {
        printMap();
        char move; cin >> move; // Mengambil input arah

        moveCourier(move); // Menggerakkan kurir
        pickUpPackage(); // Ambil paket jika ada
        deliverPackage(); // Antar paket jika sudah sampai tujuan

        #ifdef _WIN32
            Sleep(200);
        #else
            usleep(200000); // Kecepatan game (200ms)
        #endif
    }

    return 0;
}
