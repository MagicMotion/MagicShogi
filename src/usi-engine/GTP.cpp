
// 2019 Team AobaZero
// This is a work derived from Leela Zero (May 1, 2019).
/*
    This file is part of Leela Zero.
    Copyright (C) 2017-2019 Gian-Carlo Pascutto and contributors

    Leela Zero is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Leela Zero is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Leela Zero.  If not, see <http://www.gnu.org/licenses/>.

    Additional permission under GNU GPL version 3 section 7

    If you modify this Program, or any covered work, by linking or
    combining it with NVIDIA Corporation's libraries from the
    NVIDIA CUDA Toolkit and/or the NVIDIA CUDA Deep Neural
    Network library and/or the NVIDIA TensorRT inference library
    (or a modified version of those libraries), containing parts covered
    by the terms of the respective license agreement, the licensors of
    this Program grant you additional permission to convey the resulting
    work.
*/

#include "config.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>

#include "GTP.h"
#include "FastBoard.h"
#include "FullBoard.h"
#include "GameState.h"
#include "Network.h"
//#include "SGFTree.h"
#include "SMP.h"
//#include "Training.h"
#include "UCTSearch.h"
#include "Utils.h"

using namespace Utils;

// Configuration flags
bool cfg_gtp_mode;
bool cfg_allow_pondering;
unsigned int cfg_num_threads;
unsigned int cfg_batch_size;
int cfg_max_playouts;
int cfg_max_visits;
size_t cfg_max_memory;
size_t cfg_max_tree_size;
int cfg_max_cache_ratio_percent;
TimeManagement::enabled_t cfg_timemanage;
int cfg_lagbuffer_cs;
int cfg_resignpct;
int cfg_noise;
int cfg_random_cnt;
int cfg_random_min_visits;
float cfg_random_temp;
std::uint64_t cfg_rng_seed;
bool cfg_dumbpass;
#ifdef USE_OPENCL
std::vector<int> cfg_gpus;
bool cfg_sgemm_exhaustive;
bool cfg_tune_only;
#ifdef USE_HALF
precision_t cfg_precision;
#endif
#endif
float cfg_puct;
float cfg_logpuct;
float cfg_logconst;
float cfg_softmax_temp;
float cfg_fpu_reduction;
float cfg_fpu_root_reduction;
float cfg_ci_alpha;
float cfg_lcb_min_visit_ratio;
std::string cfg_weightsfile;
std::string cfg_logfile;
FILE* cfg_logfile_handle;
bool cfg_quiet;
std::string cfg_options_str;
bool cfg_benchmark;
bool cfg_cpu_only;
AnalyzeTags cfg_analyze_tags;

#if 0
/* Parses tags for the lz-analyze GTP command and friends */
AnalyzeTags::AnalyzeTags(std::istringstream& cmdstream, const GameState& game) {
    std::string tag;

    /* Default color is the current one */
    m_who = game.board.get_to_move();

    auto avoid_not_pass_resign_b = false, avoid_not_pass_resign_w = false;
    auto allow_b = false, allow_w = false;

    while (true) {
        cmdstream >> std::ws;
        if (isdigit(cmdstream.peek())) {
            tag = "interval";
        } else {
            cmdstream >> tag;
            if (cmdstream.fail() && cmdstream.eof()) {
                /* Parsing complete */
                m_invalid = false;
                return;
            }
        }

        if (tag == "avoid" || tag == "allow") {
            std::string textcolor, textmoves;
            size_t until_movenum;
            cmdstream >> textcolor;
            cmdstream >> textmoves;
            cmdstream >> until_movenum;
            if (cmdstream.fail()) {
                return;
            }

            std::vector<int> moves;
            std::istringstream movestream(textmoves);
            while (!movestream.eof()) {
                std::string textmove;
                getline(movestream, textmove, ',');
                auto sepidx = textmove.find_first_of(':');
                if (sepidx != std::string::npos) {
                    if (!(sepidx == 2 || sepidx == 3)) {
                        moves.clear();
                        break;
                    }
                    auto move1_compressed = game.board.text_to_move(
                        textmove.substr(0, sepidx)
                    );
                    auto move2_compressed = game.board.text_to_move(
                        textmove.substr(sepidx + 1)
                    );
                    if (move1_compressed == FastBoard::NO_VERTEX ||
                        move1_compressed == FastBoard::PASS ||
                        move1_compressed == FastBoard::RESIGN ||
                        move2_compressed == FastBoard::NO_VERTEX ||
                        move2_compressed == FastBoard::PASS ||
                        move2_compressed == FastBoard::RESIGN)
                    {
                        moves.clear();
                        break;
                    }
                    auto move1_xy = game.board.get_xy(move1_compressed);
                    auto move2_xy = game.board.get_xy(move2_compressed);
                    auto xmin = std::min(move1_xy.first, move2_xy.first);
                    auto xmax = std::max(move1_xy.first, move2_xy.first);
                    auto ymin = std::min(move1_xy.second, move2_xy.second);
                    auto ymax = std::max(move1_xy.second, move2_xy.second);
                    for (auto move_x = xmin; move_x <= xmax; move_x++) {
                        for (auto move_y = ymin; move_y <= ymax; move_y++) {
                            moves.push_back(game.board.get_vertex(move_x,move_y));
                        }
                    }
                } else {
                    auto move = game.board.text_to_move(textmove);
                    if (move == FastBoard::NO_VERTEX) {
                        moves.clear();
                        break;
                    }
                    moves.push_back(move);
                }
            }
            if (moves.empty()) {
                return;
            }

            int color;
            if (textcolor == "w" || textcolor == "white") {
                color = FastBoard::WHITE;
            } else if (textcolor == "b" || textcolor == "black") {
                color = FastBoard::BLACK;
            } else {
                return;
            }

            if (until_movenum < 1) {
                return;
            }
            until_movenum += game.get_movenum() - 1;

            for (const auto& move : moves) {
                if (tag == "avoid") {
                    add_move_to_avoid(color, move, until_movenum);
                    if (move != FastBoard::PASS && move != FastBoard::RESIGN) {
                        if (color == FastBoard::BLACK) {
                            avoid_not_pass_resign_b = true;
                        } else {
                            avoid_not_pass_resign_w = true;
                        }
                    }
                } else {
                    add_move_to_allow(color, move, until_movenum);
                    if (color == FastBoard::BLACK) {
                        allow_b = true;
                    } else {
                        allow_w = true;
                    }
                }
            }
            if ((allow_b && avoid_not_pass_resign_b) ||
                (allow_w && avoid_not_pass_resign_w)) {
                /* If "allow" is in use, it is illegal to use "avoid" with any
                 * move that is not "pass" or "resign". */
                return;
            }
        } else if (tag == "w" || tag == "white") {
            m_who = FastBoard::WHITE;
        } else if (tag == "b" || tag == "black") {
            m_who = FastBoard::BLACK;
        } else if (tag == "interval") {
            cmdstream >> m_interval_centis;
            if (cmdstream.fail()) {
                return;
            }
        } else if (tag == "minmoves") {
            cmdstream >> m_min_moves;
            if (cmdstream.fail()) {
                return;
            }
        } else {
            return;
        }
    }
}

void AnalyzeTags::add_move_to_avoid(int color, int vertex, size_t until_move) {
    m_moves_to_avoid.emplace_back(color, until_move, vertex);
}

void AnalyzeTags::add_move_to_allow(int color, int vertex, size_t until_move) {
    m_moves_to_allow.emplace_back(color, until_move, vertex);
}

int AnalyzeTags::interval_centis() const {
    return m_interval_centis;
}

int AnalyzeTags::invalid() const {
    return m_invalid;
}

int AnalyzeTags::who() const {
    return m_who;
}

size_t AnalyzeTags::post_move_count() const {
    return m_min_moves;
}

bool AnalyzeTags::is_to_avoid(int color, int vertex, size_t movenum) const {
    for (auto& move : m_moves_to_avoid) {
        if (color == move.color && vertex == move.vertex && movenum <= move.until_move) {
            return true;
        }
    }
    if (vertex != FastBoard::PASS && vertex != FastBoard::RESIGN) {
        auto active_allow = false;
        for (auto& move : m_moves_to_allow) {
            if (color == move.color && movenum <= move.until_move) {
                active_allow = true;
                if (vertex == move.vertex) {
                    return false;
                }
            }
        }
        if (active_allow) {
            return true;
        }
    }
    return false;
}

bool AnalyzeTags::has_move_restrictions() const {
    return !m_moves_to_avoid.empty() || !m_moves_to_allow.empty();
}
#endif

std::unique_ptr<Network> GTP::s_network;

void GTP::initialize(std::unique_ptr<Network>&& net) {
    s_network = std::move(net);
/*
    bool result;
    std::string message;
    std::tie(result, message) =
        set_max_memory(cfg_max_memory, cfg_max_cache_ratio_percent);
    if (!result) {
        // This should only ever happen with 60 block networks on 32bit machine.
        myprintf("LOW MEMORY SETTINGS! Couldn't set default memory limits.\n");
        myprintf("The network you are using might be too big\n");
        myprintf("for the default settings on your system.\n");
        throw std::runtime_error("Error setting memory requirements.");
    }
    myprintf("%s\n", message.c_str());
*/
}

void GTP::setup_default_parameters() {
    cfg_gtp_mode = false;
    cfg_allow_pondering = true;

    // we will re-calculate this on Leela.cpp
    cfg_num_threads = 0;
    // we will re-calculate this on Leela.cpp
    cfg_batch_size = 0;

    cfg_max_memory = UCTSearch::DEFAULT_MAX_MEMORY;
    cfg_max_playouts = UCTSearch::UNLIMITED_PLAYOUTS;
    cfg_max_visits = UCTSearch::UNLIMITED_PLAYOUTS;
    // This will be overwriiten in initialize() after network size is known.
    cfg_max_tree_size = UCTSearch::DEFAULT_MAX_MEMORY;
    cfg_max_cache_ratio_percent = 10;
    cfg_timemanage = TimeManagement::AUTO;
    cfg_lagbuffer_cs = 100;
//    cfg_weightsfile = leelaz_file("best-network");
#ifdef USE_OPENCL
    cfg_gpus = { };
    cfg_sgemm_exhaustive = false;
    cfg_tune_only = false;

#ifdef USE_HALF
    cfg_precision = precision_t::AUTO;
#endif
#endif
    cfg_puct = 0.5f;
    cfg_logpuct = 0.015f;
    cfg_logconst = 1.7f;
    cfg_softmax_temp = 1.0f;
    cfg_fpu_reduction = 0.25f;
    // see UCTSearch::should_resign
    cfg_resignpct = -1;
    cfg_noise = false;
    cfg_fpu_root_reduction = cfg_fpu_reduction;
    cfg_ci_alpha = 1e-5f;
    cfg_lcb_min_visit_ratio = 0.10f;
    cfg_random_cnt = 0;
    cfg_random_min_visits = 1;
    cfg_random_temp = 1.0f;
    cfg_dumbpass = false;
    cfg_logfile_handle = nullptr;
    cfg_quiet = false;
    cfg_benchmark = false;
#ifdef USE_CPU_ONLY
    cfg_cpu_only = true;
#else
    cfg_cpu_only = false;
#endif

    cfg_analyze_tags = AnalyzeTags{};

    // C++11 doesn't guarantee *anything* about how random this is,
    // and in MinGW it isn't random at all. But we can mix it in, which
    // helps when it *is* high quality (Linux, MSVC).
    std::random_device rd;
    std::ranlux48 gen(rd());
    std::uint64_t seed1 = (gen() << 16) ^ gen();
    // If the above fails, this is one of our best, portable, bets.
    std::uint64_t seed2 = std::chrono::high_resolution_clock::
        now().time_since_epoch().count();
    cfg_rng_seed = seed1 ^ seed2;
}
#if 0
const std::string GTP::s_commands[] = {
    "protocol_version",
    "name",
    "version",
    "quit",
    "known_command",
    "list_commands",
    "boardsize",
    "clear_board",
    "komi",
    "play",
    "genmove",
    "showboard",
    "undo",
    "final_score",
    "final_status_list",
    "time_settings",
    "time_left",
    "fixed_handicap",
    "last_move",
    "move_history",
    "place_free_handicap",
    "set_free_handicap",
    "loadsgf",
    "printsgf",
    "kgs-genmove_cleanup",
    "kgs-time_settings",
    "kgs-game_over",
    "heatmap",
    "lz-analyze",
    "lz-genmove_analyze",
    "lz-memory_report",
    "lz-setoption",
    "gomill-explain_last_move",
    ""
};

// Default/min/max could be moved into separate fields,
// but for now we assume that the GUI will not send us invalid info.
const std::string GTP::s_options[] = {
    "option name Maximum Memory Use (MiB) type spin default 2048 min 128 max 131072",
    "option name Percentage of memory for cache type spin default 10 min 1 max 99",
    "option name Visits type spin default 0 min 0 max 1000000000",
    "option name Playouts type spin default 0 min 0 max 1000000000",
    "option name Lagbuffer type spin default 0 min 0 max 3000",
    "option name Resign Percentage type spin default -1 min -1 max 30",
    "option name Pondering type check default true",
    ""
};

std::string GTP::get_life_list(const GameState & game, bool live) {
    std::vector<std::string> stringlist;
    std::string result;
    const auto& board = game.board;

    if (live) {
        for (int i = 0; i < board.get_boardsize(); i++) {
            for (int j = 0; j < board.get_boardsize(); j++) {
                int vertex = board.get_vertex(i, j);

                if (board.get_state(vertex) != FastBoard::EMPTY) {
                    stringlist.push_back(board.get_string(vertex));
                }
            }
        }
    }

    // remove multiple mentions of the same string
    // unique reorders and returns new iterator, erase actually deletes
    std::sort(begin(stringlist), end(stringlist));
    stringlist.erase(std::unique(begin(stringlist), end(stringlist)),
                     end(stringlist));

    for (size_t i = 0; i < stringlist.size(); i++) {
        result += (i == 0 ? "" : "\n") + stringlist[i];
    }

    return result;
}

void GTP::execute(GameState & game, const std::string& xinput) {
    std::string input;
    static auto search = std::make_unique<UCTSearch>(game, *s_network);

    bool transform_lowercase = true;

    // Required on Unixy systems
    if (xinput.find("loadsgf") != std::string::npos) {
        transform_lowercase = false;
    }

    /* eat empty lines, simple preprocessing, lower case */
    for (unsigned int tmp = 0; tmp < xinput.size(); tmp++) {
        if (xinput[tmp] == 9) {
            input += " ";
        } else if ((xinput[tmp] > 0 && xinput[tmp] <= 9)
                || (xinput[tmp] >= 11 && xinput[tmp] <= 31)
                || xinput[tmp] == 127) {
               continue;
        } else {
            if (transform_lowercase) {
                input += std::tolower(xinput[tmp]);
            } else {
                input += xinput[tmp];
            }
        }

        // eat multi whitespace
        if (input.size() > 1) {
            if (std::isspace(input[input.size() - 2]) &&
                std::isspace(input[input.size() - 1])) {
                input.resize(input.size() - 1);
            }
        }
    }

    std::string command;
    int id = -1;

    if (input == "") {
        return;
    } else if (input == "exit") {
        exit(EXIT_SUCCESS);
    } else if (input.find("#") == 0) {
        return;
    } else if (std::isdigit(input[0])) {
        std::istringstream strm(input);
        char spacer;
        strm >> id;
        strm >> std::noskipws >> spacer;
        std::getline(strm, command);
    } else {
        command = input;
    }

    /* process commands */
    if (command == "protocol_version") {
        gtp_printf(id, "%d", GTP_VERSION);
        return;
    } else if (command == "name") {
        gtp_printf(id, PROGRAM_NAME);
        return;
    } else if (command == "version") {
        gtp_printf(id, PROGRAM_VERSION);
        return;
    } else if (command == "quit") {
        gtp_printf(id, "");
        exit(EXIT_SUCCESS);
    } else if (command.find("known_command") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp;

        cmdstream >> tmp;     /* remove known_command */
        cmdstream >> tmp;

        for (int i = 0; s_commands[i].size() > 0; i++) {
            if (tmp == s_commands[i]) {
                gtp_printf(id, "true");
                return;
            }
        }

        gtp_printf(id, "false");
        return;
    } else if (command.find("list_commands") == 0) {
        std::string outtmp(s_commands[0]);
        for (int i = 1; s_commands[i].size() > 0; i++) {
            outtmp = outtmp + "\n" + s_commands[i];
        }
        gtp_printf(id, outtmp.c_str());
        return;
    } else if (command.find("boardsize") == 0) {
        std::istringstream cmdstream(command);
        std::string stmp;
        int tmp;

        cmdstream >> stmp;  // eat boardsize
        cmdstream >> tmp;

        if (!cmdstream.fail()) {
            if (tmp != BOARD_SIZE) {
                gtp_fail_printf(id, "unacceptable size");
            } else {
                float old_komi = game.get_komi();
                Training::clear_training();
                game.init_game(tmp, old_komi);
                gtp_printf(id, "");
            }
        } else {
            gtp_fail_printf(id, "syntax not understood");
        }

        return;
    } else if (command.find("clear_board") == 0) {
        Training::clear_training();
        game.reset_game();
        search = std::make_unique<UCTSearch>(game, *s_network);
        assert(UCTNodePointer::get_tree_size() == 0);
        gtp_printf(id, "");
        return;
    } else if (command.find("komi") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp;
        float komi = KOMI;
        float old_komi = game.get_komi();

        cmdstream >> tmp;  // eat komi
        cmdstream >> komi;

        if (!cmdstream.fail()) {
            if (komi != old_komi) {
                game.set_komi(komi);
            }
            gtp_printf(id, "");
        } else {
            gtp_fail_printf(id, "syntax not understood");
        }

        return;
    } else if (command.find("play") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp;
        std::string color, vertex;

        cmdstream >> tmp;   //eat play
        cmdstream >> color;
        cmdstream >> vertex;

        if (!cmdstream.fail()) {
            if (!game.play_textmove(color, vertex)) {
                gtp_fail_printf(id, "illegal move");
            } else {
                gtp_printf(id, "");
            }
        } else {
            gtp_fail_printf(id, "syntax not understood");
        }
        return;
    } else if (command.find("genmove") == 0
               || command.find("lz-genmove_analyze") == 0) {
        auto analysis_output = command.find("lz-genmove_analyze") == 0;

        std::istringstream cmdstream(command);
        std::string tmp;
        cmdstream >> tmp;  // eat genmove

        int who;
        AnalyzeTags tags;

        if (analysis_output) {
            tags = AnalyzeTags{cmdstream, game};
            if (tags.invalid()) {
                gtp_fail_printf(id, "cannot parse analyze tags");
                return;
            }
            who = tags.who();
        } else {
            /* genmove command */
            cmdstream >> tmp;
            if (tmp == "w" || tmp == "white") {
                who = FastBoard::WHITE;
            } else if (tmp == "b" || tmp == "black") {
                who = FastBoard::BLACK;
            } else {
                gtp_fail_printf(id, "syntax error");
                return;
            }
        }

        if (analysis_output) {
            // Start of multi-line response
            cfg_analyze_tags = tags;
            if (id != -1) gtp_printf_raw("=%d\n", id);
            else gtp_printf_raw("=\n");
        }
        // start thinking
        {
            game.set_to_move(who);
            // Outputs winrate and pvs for lz-genmove_analyze
            int move = search->think(who);
            game.play_move(move);

            std::string vertex = game.move_to_text(move);
            if (!analysis_output) {
                gtp_printf(id, "%s", vertex.c_str());
            } else {
                gtp_printf_raw("play %s\n", vertex.c_str());
            }
        }

        if (cfg_allow_pondering) {
            // now start pondering
            if (!game.has_resigned()) {
                // Outputs winrate and pvs through gtp for lz-genmove_analyze
                search->ponder();
            }
        }
        if (analysis_output) {
            // Terminate multi-line response
            gtp_printf_raw("\n");
        }
        cfg_analyze_tags = {};
        return;
    } else if (command.find("lz-analyze") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp;

        cmdstream >> tmp; // eat lz-analyze
        AnalyzeTags tags{cmdstream, game};
        if (tags.invalid()) {
            gtp_fail_printf(id, "cannot parse analyze tags");
            return;
        }
        // Start multi-line response.
        if (id != -1) gtp_printf_raw("=%d\n", id);
        else gtp_printf_raw("=\n");
        // Now start pondering.
        if (!game.has_resigned()) {
            cfg_analyze_tags = tags;
            // Outputs winrate and pvs through gtp
            game.set_to_move(tags.who());
            search->ponder();
        }
        cfg_analyze_tags = {};
        // Terminate multi-line response
        gtp_printf_raw("\n");
        return;
    } else if (command.find("kgs-genmove_cleanup") == 0) {
        std::istringstream cmdstream(command);
        std::string tmp;

        cmdstream >> tmp;  // eat kgs-genmove
        cmdstream >> tmp;

        if (!cmdstream.fail()) {
            int who;
            if (tmp == "w" || tmp == "white") {
                who = FastBoard::WHITE;
            } else if (tmp == "b" || tmp == "black") {
                who = FastBoard::BLACK;
            } else {
                gtp_fail_printf(id, "syntax error");
                return;
            }
            game.set_passes(0);
            {
                game.set_to_move(who);
                int move = search->think(who, UCTSearch::NOPASS);
                game.play_move(move);

                std::string vertex = game.move_to_text(move);
                gtp_printf(id, "%s", vertex.c_str());
            }
            if (cfg_allow_pondering) {
                // now start pondering
                if (!game.has_resigned()) {
                    search->ponder();
                }
            }
        } else {
            gtp_fail_printf(id, "syntax not understood");
        }
        return;
    } else if (command.find("undo") == 0) {
        if (game.undo_move()) {
            gtp_printf(id, "");
        } else {
            gtp_fail_printf(id, "cannot undo");
        }
        return;
    } else if (command.find("showboard") == 0) {
        gtp_printf(id, "");
        game.display_state();
        return;
    } else if (command.find("final_score") == 0) {
        float ftmp = game.final_score();
        /* white wins */
        if (ftmp < -0.1) {
            gtp_printf(id, "W+%3.1f", float(fabs(ftmp)));