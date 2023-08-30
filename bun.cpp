#include <algorithm>
#include <array>
#include <chrono>
#include <functional>
#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Game {
   public:
    static constexpr int width = 15;
    static constexpr int height = 9;
    // left, down, right, up
    static constexpr std::array<int, 4> directions{-1, Game::width, 1,
                                                   -Game::width};
    static int xy_ind(int x, int y) { return x + Game::width * y; }
};

class Level;

struct state {
    int paq;
    std::vector<int> buns;
    std::vector<int> bunstacks;
    std::vector<int> hist;

    Level *level;

    int64_t key() const {
        int64_t x = paq;
        for (int i = 0; i < 4; i++) {
            x *= Game::width * Game::height;
            if (i < buns.size()) {
                x += buns[i];
            }
        }
        return x;
    }
};

class Level {
   public:
    state init_state;
    std::string world;

   private:
    std::array<int, Game::width * Game::height> degree;
    std::array<int, Game::width * Game::height> dead_end;
    std::array<int, Game::width * Game::height * Game::width * Game::height>
        bun_move_memo;

   public:
    Level(const std::string &world_) : init_state{.paq = -1}, world{world_} {
        for (int y = 1; y < Game::height - 1; y++) {
            for (int x = 1; x < Game::width - 1; x++) {
                const int ind = Game::xy_ind(x, y);

                if (world[ind] == 'p') {
                    init_state.paq = ind;
                    world[ind] = ' ';
                }
                if (world[ind] == 'b') {
                    init_state.buns.push_back(ind);
                    world[ind] = ' ';
                }
            }
        }
        if (init_state.paq == -1) {
            std::cerr << "paq not found" << std::endl;
        }
        if (init_state.buns.size() == 0) {
            std::cerr << "bun not found" << std::endl;
        }
        init_state.level = this;
        precompute_degrees();
        precompute_dead_ends();
        std::fill(bun_move_memo.begin(), bun_move_memo.end(), -1);
    }

    void precompute_degrees() {
        std::fill(degree.begin(), degree.end(), 0);
        for (int y = 1; y < Game::height - 1; y++) {
            for (int x = 1; x < Game::width - 1; x++) {
                const int ind = Game::xy_ind(x, y);
                degree[ind] = 0;
                if (world[ind] == ' ') {
                    for (int dir = 0; dir < 4; dir++) {
                        if (world[ind + Game::directions[dir]] == ' ') {
                            degree[ind]++;
                        }
                    }
                }
            }
        }
    }

    void precompute_dead_ends() {
        std::fill(dead_end.begin(), dead_end.end(), 0);
        for (int y = 1; y < Game::height - 1; y++) {
            for (int x = 1; x < Game::width - 1; x++) {
                int ind = Game::xy_ind(x, y);
                if (degree[ind] != 1) {
                    continue;
                }
                int dead_end_dir = 0;
                for (; dead_end_dir < 4; dead_end_dir++) {
                    if (world[ind + Game::directions[dead_end_dir]] == ' ') {
                        break;
                    }
                }
                do {
                    dead_end[ind] = 1;
                    ind += Game::directions[dead_end_dir];
                } while (degree[ind] == 2);
            }
        }
    }

    int bun_move(const int paq, const int original_bun) {
        int bun = original_bun;
        const int memo_ind = paq + Game::width * Game::height * original_bun;
        if (bun_move_memo[memo_ind] != -1) {
            return bun_move_memo[memo_ind];
        }

        bool done = false;
        while (!done) {
            const int distance = paq - bun;
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
                case 2 * Game::width:
                case Game::width:
                    // bun goes up
                    dir = 3;
                    break;
                case -2 * Game::width:
                case -Game::width:
                    // bun goes down
                    dir = 1;
                    break;
                default:
                    // bun does nothing
                    done = true;
            }
            for (int dd : {2, -2, 2 * Game::width, -2 * Game::width}) {
                if (distance == dd && world[bun + dd / 2] != ' ') {
                    done = true;
                    break;
                }
            }
            if (done) break;

            do {
                const int original_dir = dir;
                bool works = false;

                const std::array<std::function<bool(int)>, 3> predicates = {
                    [&](int b) -> bool {
                        return world[b] == ' ' && !dead_end[b];
                    },
                    [&](int b) -> bool { return world[b] == ' '; },
                    [&](int b) -> bool { return world[b] == 'o'; }};

                int new_bun = 0;
                for (const auto &predicate : predicates) {
                    // choose one of the three directions if it's available
                    // straight, left, right
                    for (const int delta_dir : {0, 1, 3}) {
                        if (!works) {
                            dir = (original_dir + delta_dir) % 4;
                            new_bun = bun + Game::directions[dir];
                            if (predicate(new_bun)) {
                                works = true;
                                break;
                            }
                        }
                    }
                }

                if (works) {
                    do {
                        new_bun = bun + Game::directions[dir];
                        if (world[new_bun] != ' ') {
                            break;
                        }
                        bun = new_bun;
                    } while (degree[new_bun] == 2 || degree[new_bun] == 1);
                } else {
                    done = true;
                    break;
                }
            } while (bun_threatened(paq - bun));
        }

        bun_move_memo[memo_ind] = bun;
        return bun;
    }

    bool bun_threatened(int distance) {
        const std::array<int, 8> threats{2,
                                         1,
                                         -2,
                                         -1,
                                         2 * Game::width,
                                         Game::width,
                                         -2 * Game::width,
                                         -Game::width};
        return std::find(std::begin(threats), std::end(threats), distance) !=
               std::end(threats);
    }
};

void viz(const state &s) {
    std::string world = s.level->world;
    world[s.paq] = 'p';
    for (const int bun : s.buns) {
        world[bun] = 'b';
    }
    for (int y = 0; y < Game::height; y++) {
        for (int x = 0; x < Game::width; x++) {
            const int ind = Game::xy_ind(x, y);
            std::cerr << world[ind];
        }
        std::cerr << std::endl;
    }
    for (const int h : s.hist) {
        std::cerr << "<v>^"[h];
    }
    std::cerr << std::endl;
}

void viz_history(state init_s, const state &s) {
    for (const int d : s.hist) {
        viz(init_s);

        init_s.paq += Game::directions[d];
        std::vector<int> new_buns;
        for (const int bun : init_s.buns) {
            int new_bun = init_s.level->bun_move(init_s.paq, bun);
            new_buns.push_back(new_bun);
        }
        init_s.buns = new_buns;
    }
    viz(s);
}

state bfs(state s) {
    std::queue<state> q;
    q.push(s);
    std::unordered_set<int64_t> visited;
    while (!q.empty()) {
        const state s = q.front();
        q.pop();
        // viz(s);
        bool finished = true;
        for (const int bun  : s.buns) {
            if (s.paq != bun) {
                finished = false;
            }
        }
        if (finished) {
            return s;
        }
        for (int dir = 0; dir < 4; dir++) {
            const int new_paq = s.paq + Game::directions[dir];
            if (s.level->world[new_paq] != ' ') {
                continue;
            }
            std::vector<int> new_buns;
            for (const int bun : s.buns) {
                int new_bun = s.level->bun_move(new_paq, bun);
                new_buns.push_back(new_bun);
            }

            state ns{.paq = new_paq, .buns = new_buns, .level = s.level};
            const int64_t ns_key = ns.key();
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
        "###############"
        "####o###     ##"
        "#  # ### # # ##"
        "#      b     ##"
        "#  # ##### ####"
        "####p####   ###"
        "########## ####"
        "###############";
        */
    /*
    const std::string world =
        "###############"
        "############# #"
        "####p  #      #"
        "#### # # ## ###"
        "####   b  # ###"
        "######## ##  o#"
        "########    ###"
        "###############"
        "###############";
        */
    /*
    const std::string world =
        "###############"
        "#####     #####"
        "#o    # #b    #"
        "#####     ### #"
        "####  # #   # #"
        "#####     # #p#"
        "### ### ### ###"
        "###          ##"
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
        "#     o#  #####"
        "#   #### ###p##"
        "##  ####     ##"
        "#          #  #"
        "#   ###  #    #"
        "## #   ##  b# #"
        "##b     #     #"
        "###############";
    /*
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
        */
    Level level(world);
    // level.bun_move(state{.paq=39, .bun=37});

    auto start = std::chrono::steady_clock::now();

    state init_s = level.init_state;
    state s = bfs(init_s);

    auto end = std::chrono::steady_clock::now();

    auto elapsed =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Elapsed time: " << elapsed.count() << " microseconds"
              << std::endl;
    viz_history(init_s, s);
}
