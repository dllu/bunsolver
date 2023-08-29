#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Level;

struct state {
    int paq;
    int bun;
    std::vector<int> hist;

    int64_t as_int64() const { return paq + (15 * 9 * bun); }
};

class Level {
   public:
    static constexpr int width = 15;
    static constexpr int height = 9;
    // left, down, right, up
    const std::array<int, 4> directions{-1, Level::width, 1, -Level::width};
    state init_state;

   private:
    std::string world;
    std::array<int, Level::width * Level::height> degree;
    std::array<int, Level::width * Level::height> dead_end;
    std::array<int, Level::width * Level::height * Level::width * Level::height>
        bun_move_memo;

   public:
    int xy_ind(int x, int y) { return x + width * y; }
    Level(const std::string &world_)
        : world{world_}, init_state{.paq = -1, .bun = -1} {
        for (int y = 1; y < height - 1; y++) {
            for (int x = 1; x < width - 1; x++) {
                const int ind = xy_ind(x, y);

                if (world[ind] == 'p') {
                    init_state.paq = ind;
                    world[ind] = ' ';
                }
                if (world[ind] == 'b') {
                    init_state.bun = ind;
                    world[ind] = ' ';
                }
            }
        }
        if (init_state.paq == -1) {
            std::cerr << "paq not found" << std::endl;
        }
        if (init_state.bun == -1) {
            std::cerr << "bun not found" << std::endl;
        }
        precompute_degrees();
        precompute_dead_ends();
        std::fill(bun_move_memo.begin(), bun_move_memo.end(), -1);
    }

    void precompute_degrees() {
        std::fill(degree.begin(), degree.end(), 0);
        for (int y = 1; y < height - 1; y++) {
            for (int x = 1; x < width - 1; x++) {
                degree[xy_ind(x, y)] = 0;
                if (world[xy_ind(x, y)] == ' ') {
                    for (int dir = 0; dir < 4; dir++) {
                        if (world[xy_ind(x, y) + directions[dir]] == ' ') {
                            degree[xy_ind(x, y)]++;
                        }
                    }
                }
            }
        }
    }

    void precompute_dead_ends() {
        std::fill(dead_end.begin(), dead_end.end(), 0);
        for (int y = 1; y < height - 1; y++) {
            for (int x = 1; x < width - 1; x++) {
                int ind = xy_ind(x, y);
                if (degree[ind] != 1) {
                    continue;
                }
                int dead_end_dir = 0;
                for (; dead_end_dir < 4; dead_end_dir++) {
                    if (world[ind + directions[dead_end_dir]] == ' ') {
                        break;
                    }
                }
                do {
                    dead_end[ind] = 1;
                    ind += directions[dead_end_dir];
                } while (degree[ind] == 2);
            }
        }
    }

    int bun_move(state s) {
        const state original_s = s;
        if (bun_move_memo[s.as_int64()] != -1) {
            return bun_move_memo[s.as_int64()];
        }

        bool done = false;
        while (!done) {
            const int distance = s.paq - s.bun;
            int dir = 0;
            switch (distance) {
                case 2:
                case 1:
                    // bun goes left
                    dir = 0;
                    break;
                case -2:
                case -1:
                    // bun goes right
                    dir = 2;
                    break;
                case 2 * width:
                case width:
                    // bun goes up
                    dir = 3;
                    break;
                case -2 * width:
                case -width:
                    // bun goes down
                    dir = 1;
                    break;
                default:
                    // bun does nothing
                    done = true;
            }
            for (int dd : {2, -2, 2 * width, -2 * width}) {
                if (distance == dd && world[s.bun + dd / 2] != ' ') {
                    done = true;
                    break;
                }
            }
            if (done) break;
            do {
                const int original_dir = dir;
                // std::cerr << "original_dir " << original_dir << std::endl;
                bool works = false;
                int new_bun = s.bun + directions[dir];
                // choose one of the three directions if it's available and not
                // a dead end
                if (world[new_bun] == ' ' && !dead_end[new_bun]) {
                    // std::cerr << "going forward " << dir << std::endl;
                    works = true;
                }
                if (!works) {
                    // turn left
                    dir = (original_dir + 1) % 4;
                    new_bun = s.bun + directions[dir];
                    if (world[new_bun] == ' ' && !dead_end[new_bun]) {
                        // std::cerr << "going left " << dir << std::endl;
                        works = true;
                    }
                }
                if (!works) {
                    // turn right
                    dir = (original_dir + 3) % 4;
                    new_bun = s.bun + directions[dir];
                    if (world[new_bun] == ' ' && !dead_end[new_bun]) {
                        // std::cerr << "going right " << dir << std::endl;
                        works = true;
                    }
                }
                // choose one of the three directions, even if it's a dead end
                if (!works) {
                    dir = original_dir;
                    new_bun = s.bun + directions[dir];
                    if (world[new_bun] == ' ') {
                        // std::cerr << "going forward no dead end " << dir <<
                        // std::endl;
                        works = true;
                    }
                }
                if (!works) {
                    // turn left
                    dir = (original_dir + 1) % 4;
                    new_bun = s.bun + directions[dir];
                    if (world[new_bun] == ' ') {
                        // std::cerr << "going left no dead end " << dir <<
                        // std::endl;
                        works = true;
                    }
                }
                if (!works) {
                    // turn right
                    dir = (original_dir + 3) % 4;
                    new_bun = s.bun + directions[dir];
                    if (world[new_bun] == ' ') {
                        // std::cerr << "going right no dead end " << dir <<
                        // std::endl;
                        works = true;
                    }
                }

                if (works) {
                    do {
                        new_bun = s.bun + directions[dir];
                        if (world[new_bun] != ' ') {
                            break;
                        }
                        s.bun = new_bun;
                    } while (degree[new_bun] == 2 || degree[new_bun] == 1);
                } else {
                    done = true;
                    break;
                }
            } while (bun_threatened(s.paq - s.bun));
        }
        bun_move_memo[original_s.as_int64()] = s.bun;
        /*
        if (s.bun != original_s.bun) {
            std::cerr << "moving bun!!!" << std::endl;
            viz(original_s);
            viz(s);
            std::cerr << "moved successfully" << std::endl;
        }
        */
        return s.bun;
    }

    bool bun_threatened(int distance) {
        const std::array<int, 8> threats{2,         1,     -2,         -1,
                                         2 * width, width, -2 * width, -width};
        return std::find(std::begin(threats), std::end(threats), distance) !=
               std::end(threats);
    }

    state bfs(state s) {
        std::queue<state> q;
        q.push(s);
        std::unordered_set<int64_t> visited;
        while (!q.empty()) {
            state s = q.front();
            // viz(s);
            if (s.paq == s.bun) {
                return s;
            }
            q.pop();
            for (int dir = 0; dir < 4; dir++) {
                state ns{.paq = s.paq + directions[dir], .bun = s.bun};
                if (world[ns.paq] != ' ') {
                    continue;
                }
                ns.bun = bun_move(ns);
                const int64_t ns_key = ns.as_int64();
                if (visited.count(ns_key) != 0) {
                    continue;
                }
                visited.insert(ns_key);
                ns.hist = s.hist;
                ns.hist.push_back(dir);
                q.push(ns);
            }
        }
        std::cerr << "no solution found" << std::endl;
        return s;
    }

    void viz_history(const state &s) {
        state s2 = init_state;
        for (const int d : s.hist) {
            viz(s2);
            s2.paq += directions[d];
            s2.bun = bun_move(s2);
        }
        viz(s);
    }

    void viz(const state &s) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                const int ind = xy_ind(x, y);
                if (ind == s.paq) {
                    std::cerr << "p";
                } else if (ind == s.bun) {
                    std::cerr << "b";
                } else {
                    std::cerr << world[ind];
                }
            }
            std::cerr << std::endl;
        }
        for (const int h : s.hist) {
            std::cerr << "asdw"[h];
        }
        std::cerr << std::endl;
    }

    bool edge(int p) {
        int x = p % width;
        int y = p / width;
        return x == 0 || x == width - 1 || y == 0 || y == height - 1;
    }
};

int main() {
    /*
    const std::string world =
        "###############"
        "######## # ####"
        "#######     ###"
        "########## ####"
        "##   p    b ###"
        "#### ##########"
        "####o##########"
        "###############"
        "###############";
        */
    /*
    const std::string world =
        "###############"
        "##    ##   #  #"
        "#p   b   #  o##"
        "####  ##   #  #"
        "#    #### #####"
        "## # #    #####"
        "##        # ###"
        "##     #    ###"
        "###############";
    */
    const std::string world =
        "###############"
        "#  #    ##### #"
        "#           # #"
        "###### ####   #"
        "##o    ## #   #"
        "###   #b  #   #"
        "#### ##       #"
        "###  ########p#"
        "###############";
    Level level(world);
    // level.bun_move(state{.paq=39, .bun=37});

    auto start = std::chrono::steady_clock::now();

    state s = level.bfs(level.init_state);

    auto end = std::chrono::steady_clock::now();

    auto elapsed =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Elapsed time: " << elapsed.count() << " microseconds"
              << std::endl;
    level.viz_history(s);
}
