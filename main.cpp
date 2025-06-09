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
#include <iomanip>
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
const string EMOJI_CLOCK = "⏰";   // Clock
const string EMOJI_ROAD = "  ";    // Empty road (double space for alignment)

// Flag to determine if emojis can be used
bool use_emojis = false;

// Posisi awal kurir
int courierX = WIDTH / 2;
int courierY = HEIGHT / 2;

stack<char> carriedPackages;                       // Stack untuk membawa paket
set<string> registered_users;                      // save registered user
vector<pair<int, int>> house_locations;            // house locations - fixed spacing
map<pair<int, int>, vector<pair<int, int>>> graph; // Graph structure for navigation - fixed spacing
map<string, int> user_scores;
bool all_packages_delivered();
bool is_time_up();
int get_remaining_time();
void show_win_animation();
void post_game_options();
void show_leaderboard();
void show_intro();
void ask_house_count();

string current_user;
int score = 0;      // Skor
int move_limit = 0; // move limit courier
int old_highscore = 0;
int house_count = 3; // Number of houses (default 3)

time_t start_time;
int TIME_LIMIT = 45;

// Helper function to check if terminal supports UTF-8
bool check_utf8_support()
{
#ifdef _WIN32
    // Check if Windows terminal supports UTF-8
    UINT originalCP = GetConsoleOutputCP();
    bool success = SetConsoleOutputCP(CP_UTF8);
    if (success)
    {
        SetConsoleOutputCP(originalCP);
        return true;
    }
    return false;
#else
    // Most Unix-like systems support UTF-8
    const char *term = getenv("TERM");
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


void load_users()
{
    ifstream infile("users.txt");

    string username;
    int highscore;

    while (infile >> username >> highscore)
    {
        registered_users.insert(username);
        user_scores[username] = highscore;
    }
}

void save_user(const string &username, int score)
{
    user_scores[username] = score;

    ofstream outfile("users.txt", ios::trunc);
    for (map<string, int>::const_iterator it = user_scores.begin(); it != user_scores.end(); ++it)
    {
        outfile << it->first << " " << it->second << endl;
    }
}

void login_or_regis()
{
    load_users();
    string username;
    regex username_pattern("^[A-Za-z0-9_]{3,}$");

    while (true)
    {
        cout << "\nMasukkan username Anda (huruf, angka, underscore, min 3 karakter): ";
        cin >> username;

        if (regex_match(username, username_pattern))
        {
            break;
        }
        else
        {
            cout << "Username tidak valid. Silakan coba lagi.\n";
        }
    }

    current_user = username;

    if (registered_users.count(username))
    {
        old_highscore = user_scores[username];
        cout << "\nSelamat datang kembali " << username << "! High Score kamu sebelumnya: " << user_scores[username] << " point!" << endl
             << endl;
    }
    else
    {
        cout << "\nRegistrasi baru untuk " << username << " berhasil! Selamat bermain!\n\n";
        user_scores[username] = 0;
        old_highscore = 0;
        save_user(username, 0);
    }

#ifdef _WIN32
    system("pause");
    cout << "\n";
#else
    cout << "Tekan ENTER untuk mulai...";
    cin.ignore();
    cin.get();
#endif
}

// Build graph for map navigation
void build_graph()
{
    graph.clear();

    // Define directions: up, right, down, left
    int dx[4] = {0, 1, 0, -1};
    int dy[4] = {-1, 0, 1, 0};

    // Check each cell and create edges to neighbors
    for (int y = 1; y < HEIGHT - 1; y++)
    {
        for (int x = 1; x < WIDTH - 1; x++)
        {
            // Skip walls
            if (game_map[y][x] == '#')
                continue;

            pair<int, int> current = make_pair(y, x);
            vector<pair<int, int>> neighbors; // Fixed spacing

            // Check all four directions
            for (int d = 0; d < 4; d++)
            {
                int ny = y + dy[d];
                int nx = x + dx[d];

                // If the neighbor is valid and not a wall, add it as an edge
                if (ny >= 1 && ny < HEIGHT - 1 && nx >= 1 && nx < WIDTH - 1 && game_map[ny][nx] != '#')
                {
                    neighbors.push_back(make_pair(ny, nx));
                }
            }

            // Add all neighbors to the graph
            graph[current] = neighbors;
        }
    }
}

int calculate_shortest_path(int startY, int startX, int targetY, int targetX)
{
    queue<pair<int, int>> q; // Fixed spacing
    int dist[HEIGHT][WIDTH];
    bool visited[HEIGHT][WIDTH] = {false};

    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            dist[i][j] = -1;
        }
    }

    pair<int, int> start = make_pair(startY, startX);
    q.push(start);
    dist[startY][startX] = 0;
    visited[startY][startX] = true;

    while (!q.empty())
    {
        pair<int, int> current = q.front();
        int y = current.first;
        int x = current.second;
        q.pop();

        if (y == targetY && x == targetX)
        {
            return dist[y][x]; // shortest path found
        }

        // Use the graph structure for navigation
        vector<pair<int, int>> neighbors = graph[make_pair(y, x)]; // Fixed spacing
        for (vector<pair<int, int>>::iterator it = neighbors.begin(); it != neighbors.end(); ++it)
        { // Fixed spacing
            int ny = it->first;
            int nx = it->second;

            if (!visited[ny][nx])
            {
                visited[ny][nx] = true;
                dist[ny][nx] = dist[y][x] + 1;
                q.push(make_pair(ny, nx));
            }
        }
    }

    return -1; // no path found
}

// Find nearest house from current position
pair<int, int> find_nearest_house()
{
    int min_dist = INT_MAX;
    pair<int, int> nearest = make_pair(-1, -1);

    for (vector<pair<int, int>>::iterator it = house_locations.begin(); it != house_locations.end(); ++it)
    { // Fixed spacing
        int dist = calculate_shortest_path(courierY, courierX, it->first, it->second);
        if (dist > 0 && dist < min_dist)
        {
            min_dist = dist;
            nearest = *it;
        }
    }

    return nearest;
}

void show_animation()
{
    const string frames[] = {
        " \n  Horeee!!! Paket berhasil diantar!\n     \\(^_^)/ \n        |\n       / \\\n        ",
        "\n  Paket diterima dengan bahagia!\n     (^o^)/\n      \\|\n      / \\\n        ",
        "\n  Terima kasih, kurir hebat! Cihuyyyy!!\n     (^-^)\n       |\n      / \\\n       "};

    for (int i = 0; i < 3; ++i)
    {
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

void add_house_at_random_location()
{
    int attempts = 0;
    while (attempts < 100)
    { // Limit attempts to avoid infinite loop
        int x = rand() % (WIDTH - 2) + 1;
        int y = rand() % (HEIGHT - 2) + 1;

        if (game_map[y][x] == ' ' && !(x == courierX && y == courierY))
        {
            game_map[y][x] = 'H';
            house_locations.push_back(make_pair(y, x));
            break;
        }
        attempts++;
    }
}

// Fungsi untuk menghasilkan peta
void generateMap()
{
    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            if (i == 0 || i == HEIGHT - 1 || j == 0 || j == WIDTH - 1)
            {
                game_map[i][j] = '#'; // Tembok pinggir
            }
            else
            {
                game_map[i][j] = ' '; // Jalan
            }
        }
    }

    // Clear house locations
    house_locations.clear();

    // Add multiple houses at random locations
    for (int i = 0; i < house_count; i++)
    {
        add_house_at_random_location();
    }

    // Menambahkan beberapa tembok acak di dalam map
    int wallCount = WIDTH / 2; // Jumlah tembok acak (scaled to map size)
    int wallAdded = 0;

    while (wallAdded < wallCount)
    {
        int x = rand() % (WIDTH - 2) + 1;
        int y = rand() % (HEIGHT - 2) + 1;

        // Pastikan tidak di posisi kurir atau rumah
        bool isHouse = false;
        for (vector<pair<int, int>>::iterator it = house_locations.begin(); it != house_locations.end(); ++it)
        { // Fixed spacing
            if (it->first == y && it->second == x)
            {
                isHouse = true;
                break;
            }
        }

        // Pastikan tidak di posisi kurir atau rumah
        if (game_map[y][x] == ' ' && !(x == courierX && y == courierY) && !isHouse)
        {
            game_map[y][x] = '#'; // Tembok dalam map
            wallAdded++;
        }
    }

    // Menambahkan beberapa paket secara acak
    int paketCount = house_count + 1; // Jumlah paket (one more than houses)
    int added = 0;

    while (added < paketCount)
    {
        int x = rand() % (WIDTH - 2) + 1;
        int y = rand() % (HEIGHT - 2) + 1;

        // Pastikan bukan di atas rumah, kurir, atau tembok
        bool isHouse = false;
        for (vector<pair<int, int>>::iterator it = house_locations.begin(); it != house_locations.end(); ++it)
        { // Fixed spacing
            if (it->first == y && it->second == x)
            {
                isHouse = true;
                break;
            }
        }

        // Pastikan bukan di atas rumah, kurir, atau tembok
        if (game_map[y][x] == ' ' && !(x == courierX && y == courierY) && !isHouse)
        {
            game_map[y][x] = 'P';
            added++;
        }
    }

    // Build navigation graph
    build_graph();
}

// Check if a position is a house
bool is_house(int y, int x)
{
    for (vector<pair<int, int>>::iterator it = house_locations.begin(); it != house_locations.end(); ++it)
    { // Fixed spacing
        if (it->first == y && it->second == x)
        {
            return true;
        }
    }
    return false;
}

// Fungsi untuk menampilkan peta dengan emoji support
void printMap()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear"); // Menghapus layar
#endif

    for (int i = 0; i < HEIGHT; i++)
    {
        for (int j = 0; j < WIDTH; j++)
        {
            if (i == courierY && j == courierX)
            {
                if (use_emojis)
                {
                    cout << EMOJI_COURIER;
                }
                else
                {
                    cout << "C ";
                }
            }
            else if (is_house(i, j))
            {
                if (use_emojis)
                {
                    cout << EMOJI_HOUSE;
                }
                else
                {
                    cout << "H ";
                }
            }
            else
            {
                if (use_emojis)
                {
                    if (game_map[i][j] == '#')
                    {
                        cout << EMOJI_WALL;
                    }
                    else if (game_map[i][j] == 'P')
                    {
                        cout << EMOJI_PACKAGE;
                    }
                    else
                    {
                        cout << EMOJI_ROAD;
                    }
                }
                else
                {
                    cout << game_map[i][j] << " ";
                }
            }
        }
        cout << endl;
    }

    cout << "\nSkor: " << score << endl;
    cout << "Paket dibawa: " << carriedPackages.size() << "/3" << endl;
    cout << "Langkah tersisa: " << move_limit << endl;
    cout << EMOJI_CLOCK << " Sisa waktu: " << get_remaining_time() << " detik" << endl;

    // Display direction to nearest house if carrying packages
    if (!house_locations.empty() && !carriedPackages.empty())
    {
        pair<int, int> nearest = find_nearest_house();
        if (nearest.first != -1)
        {
            string direction = "";
            if (nearest.first < courierY)
                direction += "Utara";
            else if (nearest.first > courierY)
                direction += "Selatan";

            if (nearest.second < courierX)
            {
                if (!direction.empty())
                    direction += "-";
                direction += "Barat";
            }
            else if (nearest.second > courierX)
            {
                if (!direction.empty())
                    direction += "-";
                direction += "Timur";
            }

            cout << "Arah ke rumah terdekat: " << direction << endl;
        }
    }

    cout << "\nKontrol: WASD = Bergerak, Q = Keluar" << endl;
}

// Fungsi untuk kontrol arah
void moveCourier(char direction)
{
    // Hitung posisi tujuan
    int nextX = courierX;
    int nextY = courierY;

    if (direction == 'w')
        nextY--;
    if (direction == 's')
        nextY++;
    if (direction == 'a')
        nextX--;
    if (direction == 'd')
        nextX++;

    // Cek apakah menabrak tembok
    if (game_map[nextY][nextX] == '#')
    {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif

        cout << "💥 GAME OVER! Kamu menabrak tembok! 💥" << endl;
        cout << "Skor akhir: " << score << endl;

        cout << "High score sebelumnya: " << old_highscore << " point" << endl;

        if (score > old_highscore)
        {
            cout << "\n🎉 Selamat! Skor baru kamu (" << score << ") adalah rekor baru! 🎉\n";
            save_user(current_user, score);
            old_highscore = score;
        }
        else
        {
            cout << "\nSkor kamu belum mengalahkan rekor sebelumnya 😢\n";
            cout << "Skor tertinggi kamu tetap: " << old_highscore << " point\n";
        }

#ifdef _WIN32
        cout << "\n";
        system("pause");
#else
        cout << "\nTekan ENTER untuk mulai...";
        cin.ignore();
        cin.get();
#endif

        post_game_options();
    }

    // Update posisi kurir
    courierX = nextX;
    courierY = nextY;
}

// Fungsi untuk mengambil paket
void pickUpPackage()
{
    if (carriedPackages.size() < 3)
    {
        if (game_map[courierY][courierX] == 'P')
        {
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
void remove_house(int y, int x)
{
    for (vector<pair<int, int>>::iterator it = house_locations.begin(); it != house_locations.end(); ++it)
    { // Fixed spacing
        if (it->first == y && it->second == x)
        {
            house_locations.erase(it);
            break;
        }
    }
}

// Fungsi untuk mengantar paket
void deliverPackage()
{
    if (is_house(courierY, courierX) && !carriedPackages.empty())
    {
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

        if (house_count == 1)
            TIME_LIMIT += 15;
        else if (house_count == 2)
            TIME_LIMIT += 13;
        else if (house_count == 3)
            TIME_LIMIT += 10;
        else if (house_count == 4)
            TIME_LIMIT += 7;
        else if (house_count == 5)
            TIME_LIMIT += 5;

        if (all_packages_delivered())
        {
            show_win_animation();
        }
    }
}

bool all_packages_delivered()
{
    for (int i = 1; i < HEIGHT - 1; ++i)
    {
        for (int j = 1; j < WIDTH - 1; ++j)
        {
            if (game_map[i][j] == 'P')
            {
                return false;
            }
        }
    }
    return carriedPackages.empty();
}

void show_win_animation()
{
    const string win_frame[] = {
        "\n🏆 SELAMAT! KAMU TELAH MENGANTARKAN SEMUA PAKET! 🏆\n",
        "     \\(^_^)/     🎉🎉🎉\n\n",
        "   Kurir terbaik sepanjang masa!\n",
    };

#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif

    for (int i = 0; i < 3; ++i)
    {
        cout << win_frame[i];

#ifdef _WIN32
        Sleep(700);
#else
        usleep(700000); // 0.7 detik antar baris
#endif
    }

    cout << "Skor akhir kamu: " << score << " point" << endl;
    cout << "High score sebelumnya: " << old_highscore << " point" << endl;

    if (score > old_highscore)
    {
        cout << "\n🎉 Selamat! Skor baru kamu (" << score << ") adalah rekor baru! 🎉\n";
        save_user(current_user, score);
        old_highscore = score;
    }
    else
    {
        cout << "\nSkor kamu belum mengalahkan rekor sebelumnya 😢\n";
        cout << "Skor tertinggi kamu tetap: " << old_highscore << " point\n";
    }

#ifdef _WIN32
    cout << "\n";
    system("pause");
#else
    cout << "\nTekan ENTER untuk mulai...";
    cin.ignore();
    cin.get();
#endif

    post_game_options();
}

void post_game_options()
{
    string choice;

    while (true)
    {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif

        cout << "=======================================================" << endl;
        cout << "          CIHUY EXPRESS - DELIVERY GAME                " << endl;
        cout << "=======================================================" << endl;
        cout << "\n[1] Main lagiiii!!!\n[2] Lihat leaderbord\n[3] Keluar ah cape\n\nSilakan pilih opsi (1-3): ";
        cin >> choice;

        if (choice == "1")
        {
            score = 0;
            carriedPackages = stack<char>();
            TIME_LIMIT = 45;

            courierX = WIDTH / 2;
            courierY = HEIGHT / 2;

            show_intro();
            ask_house_count();
            generateMap();
            start_time = time(NULL);
            break;
        }
        else if (choice == "2")
        {
            show_leaderboard();
            break;
        }
        else if (choice == "3")
        {
            cout << "\n 👋 Terima kasih telah bermain " << current_user << "! Sampai jumpa lagi! 👋\n\n";
            exit(0);
        }
        else
        {
            cout << "❌ Pilihan tidak valid! Silakan pilih 1, 2, atau 3." << endl;
#ifdef _WIN32
            Sleep(2000);
#else
            usleep(2000000);
#endif
        }
    }
}

void show_leaderboard()
{
    vector<pair<string, int>> leaderboard;

    ifstream infile("users.txt");
    string line;
    while (getline(infile, line))
    {
        size_t space_pos = line.find(' ');
        if (space_pos != string::npos)
        {
            string username = line.substr(0, space_pos);
            int score = stoi(line.substr(space_pos + 1));
            leaderboard.push_back(make_pair(username, score));
        }
    }

    // Sort from highest to lowest score using custom comparison function
    for (size_t i = 0; i < leaderboard.size(); ++i)
    {
        for (size_t j = i + 1; j < leaderboard.size(); ++j)
        {
            if (leaderboard[i].second < leaderboard[j].second)
            {
                pair<string, int> temp = leaderboard[i];
                leaderboard[i] = leaderboard[j];
                leaderboard[j] = temp;
            }
        }
    }

    cout << "\n=============== 📊 LEADERBOARD 📊 ===============\n";
    cout << "  Peringkat     | Nama              | Skor\n";
    cout << "-------------------------------------------------\n";

    int rank = 1;
    for (size_t i = 0; i < leaderboard.size() && rank <= 10; ++i)
    {
        cout << "     " << rank << "     | " << setw(12) << left << leaderboard[i].first
             << " | " << leaderboard[i].second << " point" << endl;
        rank++;
    }

    // Find current_user's position in the leaderboard
    int user_rank = 1;
    for (size_t i = 0; i < leaderboard.size(); ++i)
    {
        if (leaderboard[i].first == current_user)
        {
            break;
        }
        user_rank++;
    }

    cout << "-------------------------------------------------\n";
    cout << "Posisi kamu, " << current_user << ": peringkat ke-" << user_rank << " dari " << leaderboard.size() << " pemain\n";
    cout << "=================================================\n\n";

#ifdef _WIN32
    cout << "\n";
    system("pause");
#else
    cout << "\nTekan ENTER untuk kembali...";
    cin.ignore();
    cin.get();
#endif

    post_game_options();
}

void show_intro()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif

    cout << "================================================================================" << endl;
    cout << "                          CIHUY EXPRESS - DELIVERY GAME                         " << endl;
    cout << "================================================================================" << endl;
    cout << "Kamu adalah seorang kurir yang harus mengantar paket ke berbagai rumah di kota." << endl;
    cout << "Ambil paket dan antarkan ke rumah sebelum waktunya habis untuk mendapatkan poin!" << endl;
    cout << "Setiap kamu berhasil mengantar paket ke rumah, kamu akan dapat waktu tambahan!" << endl;
    cout << "Jangan lupa, hindari tembok yang menghalangi jalanmu!  " << endl;
    cout << endl;

    if (use_emojis)
    {
        cout << "Petunjuk:" << endl;
        cout << EMOJI_COURIER << " = Kurir (kamu)" << endl;
        cout << EMOJI_PACKAGE << " = Paket (ambil ini!)" << endl;
        cout << EMOJI_HOUSE << " = Rumah (antar paket ke sini!)" << endl;
        cout << EMOJI_WALL << " = Tembok (hindari ini!)" << endl;
        cout << EMOJI_CLOCK << " = Waktu / time (sisa waktu kamu!)" << endl;
    }
    else
    {
        cout << "Petunjuk:" << endl;
        cout << "C = Kurir (kamu)" << endl;
        cout << "P = Paket (ambil ini!)" << endl;
        cout << "H = Rumah (antar paket ke sini!)" << endl;
        cout << "# = Tembok (hindari ini!)" << endl;
        cout << "T = Waktu / time (sisa waktu kamu!)" << endl;
    }

    cout << endl;
    cout << "Level: " << endl;
    cout << "1. Mudah         ==>> 1 rumah, 2 paket, tambahan waktu 15 detik" << endl;
    cout << "2. Biasa aja     ==>> 2 rumah, 3 paket, tambahan waktu 13 detik" << endl;
    cout << "3. Sedang        ==>> 3 rumah, 4 paket, tambahan waktu 10 detik" << endl;
    cout << "4. Lumayan Sulit ==>> 4 rumah, 5 paket, tambahan waktu 7 detik" << endl;
    cout << "5. Sulit         ==>> 5 rumah, 6 paket, tambahan waktu 5 detik" << endl;

    cout << endl;
    cout << "Kontrol: W (atas), A (kiri), S (bawah), D (kanan)" << endl;
    cout << "=======================================================" << endl
         << endl;
}

void ask_house_count()
{
    int input;
    while (true)
    {
        cout << "Pilih level berapa yang ingin kamu tantang? (1-5): ";
        cin >> input;

        // Cek apakah input gagal (bukan angka)
        if (cin.fail())
        {
            cin.clear();             // Clear error flag
            cin.ignore(10000, '\n'); // Ignore invalid characters
            cout << "Input tidak valid! Harap masukkan angka antara 1-5." << endl;
            continue;
        }

        if (input >= 1 && input <= 5)
        {
            house_count = input;
            cout << "Level " << input << " dipilih!" << endl;
            break;
        }
        else
        {
            cout << "Level tidak tersedia! Silakan pilih level 1-5." << endl;
        }
    }
}

bool is_time_up()
{
    time_t now = time(NULL);
    return difftime(now, start_time) >= TIME_LIMIT;
}

int get_remaining_time()
{
    time_t now = time(NULL);
    int remaining = TIME_LIMIT - static_cast<int>(difftime(now, start_time));
    return remaining > 0 ? remaining : 0;
}

int main()
{
    // Setup terminal for emojis if supported
    setup_terminal();
    use_emojis = check_utf8_support();

    login_or_regis();
    srand(time(0));

    show_intro();
    ask_house_count();

    generateMap();

    cout << EMOJI_CLOCK << " Waktu kamu adalah: " << TIME_LIMIT << " detik " << EMOJI_CLOCK << "\n"
         << endl;

#ifdef _WIN32
    system("pause");
#else
    cout << "Tekan ENTER untuk mulai...";
    cin.ignore();
    cin.get();
#endif

    start_time = time(NULL);

    while (true)
    {
        printMap();
        char move;
        cout << "\nMasukkan gerakan (W/A/S/D) atau Q untuk keluar: ";
        cin >> move;

        // Convert to lowercase untuk konsistensi
        move = tolower(move);

        if (is_time_up())
        {
            cout << "\n"
                 << EMOJI_CLOCK << "Waktu habis! Kamu gagal mengantar semua paket!" << endl;
            cout << "Skor akhir kamu: " << score << " point" << endl;

            cout << "High score sebelumnya: " << old_highscore << " point" << endl;

            if (score > old_highscore)
            {
                cout << "\n🎉 Selamat! Skor baru kamu (" << score << ") adalah rekor baru! 🎉\n";
                save_user(current_user, score);
                old_highscore = score;
            }
            else
            {
                cout << "\nSkor kamu belum mengalahkan rekor sebelumnya 😢\n";
                cout << "Skor tertinggi kamu tetap: " << old_highscore << " point\n";
            }

#ifdef _WIN32
            cout << "\n";
            system("pause");
#else
            cout << "\nTekan ENTER untuk mulai...";
            cin.ignore();
            cin.get();
#endif

            post_game_options();
        }

        if (move == 'q')
        {
            cout << "Game berakhir! Skor akhir: " << score << " point" << endl;

            cout << "High score sebelumnya: " << old_highscore << " point" << endl;

            if (score > old_highscore)
            {
                cout << "\n🎉 Selamat! Skor baru kamu (" << score << ") adalah rekor baru! 🎉\n";
                save_user(current_user, score);
                old_highscore = score;
            }
            else
            {
                cout << "\nSkor kamu belum mengalahkan rekor sebelumnya 😢\n";
                cout << "Skor tertinggi kamu tetap: " << old_highscore << " point\n";
            }

#ifdef _WIN32
            cout << "\n";
            system("pause");
#else
            cout << "\nTekan ENTER untuk mulai...";
            cin.ignore();
            cin.get();
#endif

            post_game_options();
        }

        if (move == 'w' || move == 'a' || move == 's' || move == 'd')
        {
            moveCourier(move); // Menggerakkan kurir
            pickUpPackage();   // Ambil paket jika ada
            deliverPackage();  // Antar paket jika sudah sampai tujuan
        }
        else
        {
            cout << "Input tidak valid! Gunakan W/A/S/D untuk bergerak atau Q untuk keluar." << endl;
#ifdef _WIN32
            Sleep(1500);
#else
            usleep(1500000);
#endif
            continue; // Kembali ke awal loop tanpa sleep di akhir
        }

#ifdef _WIN32
        Sleep(200);
#else
        usleep(200000); // Kecepatan game (200ms)
#endif
    }
}