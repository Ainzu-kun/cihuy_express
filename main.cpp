#include <iostream>
#include <stack>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <set>
#include <map>
#include <vector>
#include <fstream>
#include <regex>
#include <algorithm>
#ifdef _WIN32
    #include <windows.h>
    #include <fcntl.h>
    #include <io.h>
#else
    #include <unistd.h>
#endif

using namespace std;

// Ukuran peta
const int WIDTH = 30;
const int HEIGHT = 15;
char game_map[HEIGHT][WIDTH];

// Emoji characters
const string EMOJI_COURIER = "🛵"; // Motorcycle
const string EMOJI_PACKAGE = "📦"; // Package
const string EMOJI_HOUSE = "🏠";   // House
const string EMOJI_WALL = "🧱";    // Wall/brick
const string EMOJI_ROAD = "  ";    // Empty road (double space for alignment)

// Flag to determine if emojis can be used
bool use_emojis = false;

// Posisi awal kurir
int courierX = WIDTH / 2;
int courierY = HEIGHT / 2;

stack<char> carriedPackages; // Stack untuk membawa paket
set<string> registered_users; // save registered user
vector<pair<int, int> > house_locations; // house locations - fixed spacing
map<pair<int, int>, vector<pair<int, int> > > graph; // Graph structure for navigation - fixed spacing
bool all_packages_delivered();
void show_win_animation();
bool is_time_up();
int get_remaining_time();

int score = 0; // Skor
int move_limit = 0; // move limit courier
int house_count = 3; // Number of houses (default 3)

time_t start_time;
int TIME_LIMIT = 45;

// Helper function to check if terminal supports UTF-8
bool check_utf8_support() {
#ifdef _WIN32
    // Check if Windows terminal supports UTF-8
    UINT originalCP = GetConsoleOutputCP();
    bool success = SetConsoleOutputCP(CP_UTF8);
    if (success) {
        SetConsoleOutputCP(originalCP);
        return true;
    }
    return false;
#else
    // Most Unix-like systems support UTF-8
    const char* term = getenv("TERM");
    if (term && (string(term).find("xterm") != string::npos || 
                string(term).find("utf") != string::npos))
        return true;
    return false;
#endif
}

void setup_terminal() {
#ifdef _WIN32
    // Set UTF-8 code page
    SetConsoleOutputCP(CP_UTF8);
    // Enable virtual terminal processing for ANSI colors
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
}

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
        cout << "\nMasukkan username Anda (huruf, angka, underscore, min 3 karakter): "; cin >> username;

        if (regex_match(username, username_pattern)) {
            break;
        } else {
            cout << "Username tidak valid. Silakan coba lagi.\n";
        }
    }

    if(registered_users.count(username)) {
        cout << "\nSelamat datang kembali " << username << "! Selamat bermain!\n" << endl;
    } else {
        cout << "\nRegistrasi baru untuk " << username << " berhasil! Selamat bermain!\n" << endl;
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

// Build graph for map navigation
void build_graph() {
    graph.clear();
    
    // Define directions: up, right, down, left
    int dx[4] = {0, 1, 0, -1};
    int dy[4] = {-1, 0, 1, 0};
    
    // Check each cell and create edges to neighbors
    for (int y = 1; y < HEIGHT - 1; y++) {
        for (int x = 1; x < WIDTH - 1; x++) {
            // Skip walls
            if (game_map[y][x] == '#') continue;
            
            pair<int, int> current = make_pair(y, x);
            vector<pair<int, int> > neighbors; // Fixed spacing
            
            // Check all four directions
            for (int d = 0; d < 4; d++) {
                int ny = y + dy[d];
                int nx = x + dx[d];
                
                // If the neighbor is valid and not a wall, add it as an edge
                if (ny >= 1 && ny < HEIGHT - 1 && nx >= 1 && nx < WIDTH - 1 && game_map[ny][nx] != '#') {
                    neighbors.push_back(make_pair(ny, nx));
                }
            }
            
            // Add all neighbors to the graph
            graph[current] = neighbors;
        }
    }
}

int calculate_shortest_path(int startY, int startX, int targetY, int targetX) {
    queue<pair<int, int> > q; // Fixed spacing
    int dist[HEIGHT][WIDTH];
    bool visited[HEIGHT][WIDTH] = {false};

    for(int i = 0; i < HEIGHT; i++) {
        for(int j = 0; j < WIDTH; j++) {
            dist[i][j] = -1;
        }
    }

    pair<int, int> start = make_pair(startY, startX);
    q.push(start);
    dist[startY][startX] = 0;
    visited[startY][startX] = true;

    while(!q.empty()) {
        pair<int, int> current = q.front();
        int y = current.first;
        int x = current.second;
        q.pop();

        if (y == targetY && x == targetX) {
            return dist[y][x]; // shortest path found
        }

        // Use the graph structure for navigation
        vector<pair<int, int> > neighbors = graph[make_pair(y, x)]; // Fixed spacing
        for (vector<pair<int, int> >::iterator it = neighbors.begin(); it != neighbors.end(); ++it) { // Fixed spacing
            int ny = it->first;
            int nx = it->second;
            
            if (!visited[ny][nx]) {
                visited[ny][nx] = true;
                dist[ny][nx] = dist[y][x] + 1;
                q.push(make_pair(ny, nx));
            }
        }
    }

    return -1; // no path found
}

// Calculate a reasonable move limit based on the minimum distance to any house
// int calculate_move_limit() {
//     vector<pair<int, int>> package_locations;

//     // Cari semua posisi paket
//     for (int y = 1; y < HEIGHT - 1; y++) {
//         for (int x = 1; x < WIDTH - 1; x++) {
//             if (game_map[y][x] == 'P') {
//                 package_locations.push_back(make_pair(y, x));
//             }
//         }
//     }

//     if (package_locations.empty() || house_locations.empty()) {
//         return 30; // fallback jika tidak ada paket/rumah
//     }

//     int min_total_distance = INT_MAX;

//     // Hitung semua kombinasi: kurir -> paket -> rumah
//     for (auto& pkg : package_locations) {
//         int dist_to_pkg = calculate_shortest_path(courierY, courierX, pkg.first, pkg.second);
//         if (dist_to_pkg == -1) continue;

//         for (auto& house : house_locations) {
//             int dist_to_house = calculate_shortest_path(pkg.first, pkg.second, house.first, house.second);
//             if (dist_to_house == -1) continue;

//             int total = dist_to_pkg + dist_to_house;
//             if (total < min_total_distance) {
//                 min_total_distance = total;
//             }
//         }
//     }

//     return (min_total_distance > 0 && min_total_distance < INT_MAX) ? min_total_distance * 2 : 30;
// }

// Find nearest house from current position
pair<int, int> find_nearest_house() {
    int min_dist = INT_MAX;
    pair<int, int> nearest = make_pair(-1, -1);
    
    for (vector<pair<int, int> >::iterator it = house_locations.begin(); it != house_locations.end(); ++it) { // Fixed spacing
        int dist = calculate_shortest_path(courierY, courierX, it->first, it->second);
        if (dist > 0 && dist < min_dist) {
            min_dist = dist;
            nearest = *it;
        }
    }
    
    return nearest;
}

void show_animation() {
    const string frames[] = {
        " \n  Horeee!!! Paket berhasil diantar!\n     \\(^_^)/ \n        |\n       / \\\n        ",
        "\n  Paket diterima dengan bahagia!\n     (^o^)/\n      \\|\n      / \\\n        ",
        "\n  Terima kasih, kurir hebat! Cihuyyyy!!\n     (^-^)\n       |\n      / \\\n       "
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
            Sleep(700);
        #else
            usleep(700000); // 0.7 detik per frame
        #endif
    }
}

void add_house_at_random_location() {
    int attempts = 0;
    while (attempts < 100) { // Limit attempts to avoid infinite loop
        int x = rand() % (WIDTH - 2) + 1;
        int y = rand() % (HEIGHT - 2) + 1;

        if (game_map[y][x] == ' ' && !(x == courierX && y == courierY)) {
            game_map[y][x] = 'H';
            house_locations.push_back(make_pair(y, x));
            break;
        }
        attempts++;
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

    // Clear house locations
    house_locations.clear();
    
    // Add multiple houses at random locations
    for (int i = 0; i < house_count; i++) {
        add_house_at_random_location();
    }

    // Menambahkan beberapa tembok acak di dalam map
    int wallCount = WIDTH / 2; // Jumlah tembok acak (scaled to map size)
    int wallAdded = 0;

    while (wallAdded < wallCount) {
        int x = rand() % (WIDTH - 2) + 1;
        int y = rand() % (HEIGHT - 2) + 1;

        // Pastikan tidak di posisi kurir atau rumah
        bool isHouse = false;
        for (vector<pair<int, int> >::iterator it = house_locations.begin(); it != house_locations.end(); ++it) { // Fixed spacing
            if (it->first == y && it->second == x) {
                isHouse = true;
                break;
            }
        }

        // Pastikan tidak di posisi kurir atau rumah
        if (game_map[y][x] == ' ' && !(x == courierX && y == courierY) && !isHouse) {
            game_map[y][x] = '#'; // Tembok dalam map
            wallAdded++;
        }
    }

    // Menambahkan beberapa paket secara acak
    int paketCount = house_count + 1; // Jumlah paket (one more than houses)
    int added = 0;

    while (added < paketCount) {
        int x = rand() % (WIDTH - 2) + 1;
        int y = rand() % (HEIGHT - 2) + 1;

        // Pastikan bukan di atas rumah, kurir, atau tembok
        bool isHouse = false;
        for (vector<pair<int, int> >::iterator it = house_locations.begin(); it != house_locations.end(); ++it) { // Fixed spacing
            if (it->first == y && it->second == x) {
                isHouse = true;
                break;
            }
        }

        // Pastikan bukan di atas rumah, kurir, atau tembok
        if (game_map[y][x] == ' ' && !(x == courierX && y == courierY) && !isHouse) {
            game_map[y][x] = 'P';
            added++;
        }
    }
    
    // Build navigation graph
    build_graph();
}

// Check if a position is a house
bool is_house(int y, int x) {
    for (vector<pair<int, int> >::iterator it = house_locations.begin(); it != house_locations.end(); ++it) { // Fixed spacing
        if (it->first == y && it->second == x) {
            return true;
        }
    }
    return false;
}

// Fungsi untuk menampilkan peta dengan emoji support
void printMap() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear"); // Menghapus layar
    #endif

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if (i == courierY && j == courierX) {
                if (use_emojis) {
                    cout << EMOJI_COURIER;
                } else {
                    cout << "C ";
                }
            } else if (is_house(i, j)) {
                if (use_emojis) {
                    cout << EMOJI_HOUSE;
                } else {
                    cout << "H ";
                }
            } else {
                if (use_emojis) {
                    if (game_map[i][j] == '#') {
                        cout << EMOJI_WALL;
                    } else if (game_map[i][j] == 'P') {
                        cout << EMOJI_PACKAGE;
                    } else {
                        cout << EMOJI_ROAD;
                    }
                } else {
                    cout << game_map[i][j] << " ";
                }
            }
        }
        cout << endl;
    }

    cout << "\nSkor: " << score << endl;
    cout << "Paket dibawa: " << carriedPackages.size() << "/3" << endl;
    cout << "Langkah tersisa: " << move_limit << endl;
    cout << "Sisa waktu: " << get_remaining_time() << " detik" << endl;
    
    // Display direction to nearest house if carrying packages
    if (!house_locations.empty() && !carriedPackages.empty()) {
        pair<int, int> nearest = find_nearest_house();
        if (nearest.first != -1) {
            string direction = "";
            if (nearest.first < courierY) direction += "Utara";
            else if (nearest.first > courierY) direction += "Selatan";
            
            if (nearest.second < courierX) {
                if (!direction.empty()) direction += "-";
                direction += "Barat";
            } else if (nearest.second > courierX) {
                if (!direction.empty()) direction += "-";
                direction += "Timur";
            }
            
            cout << "Arah ke rumah terdekat: " << direction << endl;
        }
    }
    
    cout << "\nKontrol: WASD = Bergerak, Q = Keluar" << endl;
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

    // Update posisi kurir
    courierX = nextX;
    courierY = nextY;

    // move_limit--;
    // if (move_limit <= 0) {
    //     cout << "Move limit habis! Kamu kelelahan dan gagal mengantar!" << endl;
    //     cout << "Skor akhir: " << score << endl;
    //     exit(0);
    // }
}

// Fungsi untuk mengambil paket
void pickUpPackage() {
    if (carriedPackages.size() < 3) {
        if (game_map[courierY][courierX] == 'P') {
            carriedPackages.push('P');
            game_map[courierY][courierX] = ' '; // Hapus paket dari peta
            cout << "Paket diambil! (" << carriedPackages.size() << "/3)" << endl;
            
            #ifdef _WIN32
                Sleep(500);
            #else
                usleep(500000); // 0.5 second feedback
            #endif
        }
    }
}

// Remove a house from locations
void remove_house(int y, int x) {
    for (vector<pair<int, int> >::iterator it = house_locations.begin(); it != house_locations.end(); ++it) { // Fixed spacing
        if (it->first == y && it->second == x) {
            house_locations.erase(it);
            break;
        }
    }
}

// Fungsi untuk mengantar paket
void deliverPackage() {
    if (is_house(courierY, courierX) && !carriedPackages.empty()) {
        carriedPackages.pop();
        score += 10;
        
        // Remove current house
        remove_house(courierY, courierX);
        game_map[courierY][courierX] = ' '; // Remove from map
        
        show_animation();
        
        // Add a new house in random location
        add_house_at_random_location();
        
        // Rebuild the graph after map changes
        build_graph();

        if (house_count == 1) TIME_LIMIT += 15;
        else if (house_count == 2) TIME_LIMIT += 13;
        else if (house_count == 3) TIME_LIMIT += 10;
        else if (house_count == 4) TIME_LIMIT += 7;
        else if (house_count == 5) TIME_LIMIT += 5;

        if(all_packages_delivered()) {
            show_win_animation();
        }
    }
}

bool all_packages_delivered() {
    for (int i = 1; i < HEIGHT - 1; ++i) {
        for (int j = 1; j < WIDTH - 1; ++j) {
            if (game_map[i][j] == 'P') {
                return false;
            }
        }
    }
    return carriedPackages.empty();
}

void show_win_animation() {
    const string win_frame[] = {
        "\n🏆 SELAMAT! KAMU TELAH MENGANTARKAN SEMUA PAKET! 🏆\n",
        "     \\(^_^)/     🎉🎉🎉\n",
        "   Kurir terbaik sepanjang masa!\n",
        " Skor akhir kamu: "
    };

    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif

    for (const string& line : win_frame) {
        cout << line;

        #ifdef _WIN32
            Sleep(700);
        #else
            usleep(700000); // 0.7 detik antar baris
        #endif
    }

    cout << score << " point" << endl;
    exit(0);
}


void show_intro() {
    cout << "=======================================================" << endl;
    cout << "          CIHUY EXPRESS - DELIVERY GAME                " << endl;
    cout << "=======================================================" << endl;
    cout << "Kamu adalah seorang kurir yang harus mengantar paket   " << endl;
    cout << "ke berbagai rumah di kota. Ambil paket dan antarkan    " << endl;
    cout << "ke rumah untuk mendapatkan poin!                       " << endl;
    cout << endl;
    
    if (use_emojis) {
        cout << "Petunjuk:" << endl;
        cout << EMOJI_COURIER << " = Kurir (kamu)" << endl;
        cout << EMOJI_PACKAGE << " = Paket (ambil ini!)" << endl;
        cout << EMOJI_HOUSE << " = Rumah (antar paket ke sini!)" << endl;
        cout << EMOJI_WALL << " = Tembok (hindari ini!)" << endl;
    } else {
        cout << "Petunjuk:" << endl;
        cout << "C = Kurir (kamu)" << endl;
        cout << "P = Paket (ambil ini!)" << endl;
        cout << "H = Rumah (antar paket ke sini!)" << endl;
        cout << "# = Tembok (hindari ini!)" << endl;
    }
    
    cout << endl;
    cout << "Kontrol: W (atas), A (kiri), S (bawah), D (kanan)" << endl;
    cout << "=======================================================" << endl;
    cout << "Tekan ENTER untuk mulai..." << endl;
    
    cin.ignore();
    cin.get();
}

void ask_house_count() {
    int input;
    cout << "Berapa rumah ingin kamu sediakan di peta? (1-5): "; cin >> input;
    
    if (input >= 1 && input <= 5) {
        house_count = input;
    } else {
        cout << "Input tidak valid. Menggunakan nilai default (3)." << endl;
        house_count = 3;
    }
}

bool is_time_up() {
    time_t now = time(NULL);
    return difftime(now, start_time) >= TIME_LIMIT;
}

int get_remaining_time() {
    time_t now = time(NULL);
    int remaining = TIME_LIMIT - static_cast<int>(difftime(now, start_time));
    return remaining > 0 ? remaining : 0;
}

int main() {
    // Setup terminal for emojis if supported
    setup_terminal();
    use_emojis = check_utf8_support();
    
    login_or_regis();
    srand(time(0));
    
    show_intro();
    ask_house_count();
    
    generateMap();    

    cout << "Waktu kamu adalah: " << TIME_LIMIT << " detik" << endl;
    
    // Calculate move limit based on shortest path to houses
    // move_limit = calculate_move_limit();
    // cout << "Move limit kamu adalah: " << move_limit << " langkah." << endl;
    
    #ifdef _WIN32
        system("pause");
    #else
        cout << "Tekan ENTER untuk mulai...";
        cin.ignore();
        cin.get();
    #endif

    start_time = time(NULL);

    while (true) {
        printMap();
        char move;
        cin >> move;

        if(is_time_up()) {
            cout << "\n⏰ Waktu habis! Kamu gagal mengantar semua paket!" << endl;
            cout << "Skor akhir: " << score << " point" << endl;
            break;
        }
        
        if (move == 'q' || move == 'Q') {
            cout << "Game berakhir! Skor akhir: " << score << " point" << endl;
            break;
        }
        
        if (move == 'w' || move == 'a' || move == 's' || move == 'd') {
            moveCourier(move); // Menggerakkan kurir
            pickUpPackage(); // Ambil paket jika ada
            deliverPackage(); // Antar paket jika sudah sampai tujuan
        }

        #ifdef _WIN32
            Sleep(200);
        #else
            usleep(200000); // Kecepatan game (200ms)
        #endif
    }

    return 0;
}