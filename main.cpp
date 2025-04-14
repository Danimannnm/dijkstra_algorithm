#include "Graph.h"
#include "Queue.h"
#include "Trophy.h"
#include "Util.h"
#include "WinUtil.h"
#include "algorithm.h"
#include <Windows.h>
#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <random>
#include <string>

static_assert(sizeof(WORD) == sizeof(unsigned short));

using namespace std::string_literals;
using namespace std::chrono_literals;
using Instant = std::chrono::time_point<std::chrono::high_resolution_clock>;

// Buffer dimension constants

// Screen
static constexpr size_t screen_width{120};
static constexpr size_t screen_height{40};
static constexpr size_t screen_size{screen_width * screen_height};
static constexpr size_t screen_buffer_size{screen_size + 1};

// Numeric constants
static constexpr auto ns_in_ms = 1'000'000.0L;

struct Stage;

struct State {
  std::unique_ptr<wchar_t[]> screen_buffer{
      new wchar_t[screen_buffer_size]}; // Extra character for null terminator
  std::unique_ptr<WORD[]> color_buffer{new WORD[screen_size]};
  HANDLE h_output;
  HANDLE h_input;
  ConsoleWriter writer;
  std::unique_ptr<Stage> stage;
  bool rerender{false};
  Instant current_time;
  Queue<Instant> frametimes{};
  long long dt_ns = 0;
  bool exit = false;
  std::optional<MOUSE_EVENT_RECORD> mouse_state;
  bool mouse_state_fresh = false;
  std::optional<KEY_EVENT_RECORD> key_event;
  bool key_event_fresh = false;
  std::random_device rd;
  std::mt19937 gen{rd()};
  std::optional<std::wstring> player_name;

  State();
  void init();
  ~State();

  // Preloaded buffers
  const std::unique_ptr<wchar_t[]> name_char_buf{
      new wchar_t[screen_buffer_size]};
  const std::unique_ptr<wchar_t[]> menu_char_buf{
      new wchar_t[screen_buffer_size]};
  const std::unique_ptr<wchar_t[]> difficulty_char_buf{
      new wchar_t[screen_buffer_size]};
  const std::unique_ptr<wchar_t[]> game_char_buf{
      new wchar_t[screen_buffer_size]};
  const std::unique_ptr<wchar_t[]> game_over_buf{
      new wchar_t[screen_buffer_size]};
  const std::unique_ptr<wchar_t[]> pause_menu_buf{
      new wchar_t[screen_buffer_size]};
  const std::unique_ptr<wchar_t[]> inventory_buf{
      new wchar_t[screen_buffer_size]};
  const std::unique_ptr<wchar_t[]> scoreboard_buf{
      new wchar_t[screen_buffer_size]};
  const std::unique_ptr<WORD[]> all_white_buf{new WORD[screen_size]};
  const std::unique_ptr<WORD[]> menu_color_buf{new WORD[screen_size]};
  const std::unique_ptr<WORD[]> game_color_buf{new WORD[screen_size]};
};

struct Stage {
  virtual void update(State &s) = 0;
  virtual ~Stage();
};

struct WindowAdjustment : virtual Stage {
  COORD current_dimensions;
  std::optional<Instant> success_time;
  virtual void update(State &s);
};

struct NameInput : virtual Stage {
  std::wstring name{};
  std::optional<Instant> last_blink_time;
  bool show_cursor{true};
  NameInput(State &s);
  virtual void update(State &s);
};

enum MenuOption { ContinueGame, NewGame, InventoryOpt, ScoreboardOpt, Exit };

struct Menu : virtual Stage {
  MenuOption highlighted_option = NewGame;
  std::optional<Instant> last_blink_time;
  bool show_arrow = true;
  bool save_game_exists = false;
  Menu(State &s);
  virtual void update(State &s);
};

enum Difficulty { Normal, Hard, Insane, Impossible };

struct DifficultyPicker : virtual Stage {
  Difficulty highlighted_option = Normal;
  std::optional<Instant> last_blink_time;
  bool show_arrow = true;
  DifficultyPicker(State &s);
  virtual void update(State &s);
};

static constexpr size_t map_width = 50;
static constexpr size_t map_height = 30;
static constexpr size_t map_size = map_width * map_height;

enum MapCell { Empty, Player, Start, Finish, Foe, Wall, PowerUp, Trophy };

DoublyLinkedList<size_t> shortest_path(const MapCell map[map_size],
                                       const size_t src, const size_t target) {
  auto graph = Graph{map_size};
  // Make complete grid from map
  for (size_t i{0}; i < map_size; ++i) {
    if (i % map_width < map_width - 1) {
      graph.add_undirected_edge(i, i + 1);
    }
    if (i < map_size - map_width) {
      graph.add_undirected_edge(i, i + map_width);
    }
  }

  graph.add_undirected_edge(map_size - 1, map_size - map_width);
  graph.add_undirected_edge(map_size - 1, map_size - 2);

  // Check which cells in the map are walls, and remove them from the graph
  for (size_t i{0}; i < map_size; ++i) {
    if (map[i] == Wall) {
      graph.remove_vertex(i);
    }
  }

  return graph.breadth_first_search(src, target);
}

struct Game;

void generate_map(Game &game, State &s);

static constexpr wchar_t map_cell_char(const MapCell cell) {
  switch (cell) {
  case Empty:
    return L' ';
  case Player:
    return L'X';
  case Start:
    return L'S';
  case Finish:
    return L'F';
  case Foe:
    return L'^';
  case Wall:
    return L'#';
  case PowerUp:
    return L'$';
  case Trophy:
    return L'T';
  }
}

static constexpr unsigned short map_cell_color(const MapCell cell) {
  switch (cell) {
  case Empty:
    return FG_WHITE;
  case Player:
    return FG_BLACK | BG_WHITE;
  case Start:
    return FG_WHITE;
  case Finish:
    return FG_WHITE | BG_GREEN;
  case Foe:
    return FG_RED;
  case Wall:
    return FG_MAGENTA | BG_MAGENTA;
  case PowerUp:
    return FG_CYAN;
  case Trophy:
    return FG_YELLOW;
  }
}

static constexpr auto base_move_duration = 150ms;

static constexpr auto foe_move_duration(const Difficulty d) {
  switch (d) {
  case Normal:
    return base_move_duration * 2.0L;
  case Hard:
    return base_move_duration * 1.75L;
  case Insane:
    return base_move_duration * 1.25L;
  case Impossible:
    return base_move_duration * 1.0L;
  }
}

static constexpr size_t tips_count{9};

static const std::wstring tips[tips_count]{
    L"Make a move to start the game!"s,
    L"Keep your distance from the stalkers!"s,
    L"Grab powerups to increase your speed!"s,
    L"Collect the trophy for a huge score bonus!"s,
    L"Stay on the move to increase your chances of survival!"s,
    L"Reach the end as quickly as possible to maximize your score!"s,
    L"Not even breaking a sweat? Try raising the difficulty next time!"s,
    L"Finding it tough? Select an easier difficulty next time!"s,
    L"See your collected trophies in your inventory!"s};

struct Game : virtual Stage {
  MapCell map[map_size];
  long double pos_x{0.0L};
  long double pos_y;
  long double foe_pos_x[5];
  long double foe_pos_y[5];
  size_t start_idx;
  size_t finish_idx;
  size_t foe_count = 0;
  size_t tip_index = 0;
  bool pathfinder = false;
  bool processed_one_space_keyup = true;
  bool autopilot = false;
  bool processed_one_enter_keyup = true;
  Difficulty difficulty;
  std::wstring notification{L""s};
  unsigned short notification_color = FG_WHITE;
  bool trophy_collected = false;

  std::optional<Instant> start_time;
  Instant last_tip_refresh;
  std::optional<Instant> last_powerup_pickup;
  std::optional<Instant> notification_start;

  Game(State &s, const Difficulty difficulty);
  virtual void update(State &s);

  static Game from_save();
};

enum PauseMenuOption { Resume, RestartP, ReturnToMenuP };

struct PauseMenu : virtual Stage {
  PauseMenuOption highlighted_option{Resume};
  Game paused_game;
  PauseMenu(State &s, const Game &);
  std::optional<Instant> last_blink_time;
  bool show_arrow{true};
  bool processed_one_escape_keyup = false;
  virtual void update(State &s);
};

enum GameOverOption { Restart, ReturnToMenu };

struct GameOver : virtual Stage {
  GameOverOption highlighted_option = Restart;
  const long long score;
  const Difficulty difficulty;
  bool won;
  GameOver(State &s, const long long score, const Difficulty difficulty,
           bool won);
  std::optional<Instant> last_blink_time;
  bool show_arrow = true;
  virtual void update(State &s);
};

struct SavedScore {
  wchar_t name[64];
  long long score;
};

struct Scoreboard : virtual Stage {
  std::optional<Instant> last_blink_time;
  bool processed_keyup = false;
  bool show_arrow = true;
  Scoreboard(State &s);
  virtual void update(State &s);
};

struct Inventory : virtual Stage {
  std::optional<Instant> last_blink_time;
  bool processed_keyup = false;
  bool show_arrow = true;
  Inventory(State &s);
  virtual void update(State &s);
};

int calc_fps(Queue<Instant> &frametimes);

void display_fps(State &s);

void add_score(long long score, const std::wstring &name);

DoublyLinkedList<SavedScore> get_scores();

int main() {
  // Intialise game state
  State s{};

  // Run game loop
  while (!s.exit) {
    // Prepare state object for the new frame
    s.init();

    // Run logic of the current stage
    s.stage->update(s);

    // Render the screen buffer if it has been invalidated
    if (s.rerender) {
      DWORD dummy;
      MB_ASSERT(WriteConsoleOutputCharacterW(s.h_output, s.screen_buffer.get(),
                                             screen_buffer_size, {0, 0},
                                             &dummy));
      MB_ASSERT(WriteConsoleOutputAttribute(s.h_output, s.color_buffer.get(),
                                            screen_size, {0, 0}, &dummy));
    }
  }
}

State::State()
    : h_output{GetStdHandle(STD_OUTPUT_HANDLE)},
      h_input{GetStdHandle(STD_INPUT_HANDLE)}, writer{h_output} {
  MB_ASSERT(h_output != INVALID_HANDLE_VALUE);
  MB_ASSERT(h_input != INVALID_HANDLE_VALUE);

  // Hide the cursor
  CONSOLE_CURSOR_INFO cci;
  MB_ASSERT(GetConsoleCursorInfo(h_output, &cci));
  cci.bVisible = false;
  MB_ASSERT(SetConsoleCursorInfo(h_output, &cci));

  // Enable all relevant events from console input buffer
  MB_ASSERT(SetConsoleMode(h_input, ENABLE_WINDOW_INPUT |
                                        ENABLE_PROCESSED_INPUT |
                                        ENABLE_MOUSE_INPUT));

  // Reset text color to white, and intialize the all_white preloaded buffer
  for (size_t i = 0; i < screen_size; ++i) {
    color_buffer.get()[i] = all_white_buf.get()[i] = FG_WHITE;
  }

  // Initalize the remaining preloaded buffers
  load_char_buffer(name_char_buf.get(), screen_size, "assets/name.chars");
  load_char_buffer(menu_char_buf.get(), screen_size, "assets/menu.chars");
  load_char_buffer(difficulty_char_buf.get(), screen_size,
                   "assets/difficulty.chars");
  load_char_buffer(game_char_buf.get(), screen_size, "assets/game.chars");
  load_char_buffer(game_over_buf.get(), screen_size, "assets/gameover.chars");
  load_char_buffer(pause_menu_buf.get(), screen_size, "assets/pause.chars");
  load_char_buffer(inventory_buf.get(), screen_size, "assets/trophy.chars");
  load_char_buffer(scoreboard_buf.get(), screen_size,
                   "assets/scoreboard.chars");

  load_color_buffer(menu_color_buf.get(), screen_size, "assets/menu.colors");
  load_color_buffer(game_color_buf.get(), screen_size, "assets/game.colors");

  // Add null terminator at the end of char buffers
  screen_buffer[screen_size] = L'\0';
  name_char_buf[screen_size] = L'\0';
  difficulty_char_buf[screen_size] = L'\0';
  menu_char_buf[screen_size] = L'\0';
  game_char_buf[screen_size] = L'\0';
  game_over_buf[screen_size] = L'\0';
  pause_menu_buf[screen_size] = L'\0';
  inventory_buf[screen_size] = L'\0';
  scoreboard_buf[screen_size] = L'\0';

  // Select the appropriate initial stage
  const auto current_dimensions = get_console_dimensions(h_output);
  if (current_dimensions.X == screen_width &&
      current_dimensions.Y == screen_height) {
    stage = std::make_unique<NameInput>(*this);
  } else {
    stage = std::make_unique<WindowAdjustment>();
  }
}

// Prepares state object for each frame
void State::init() {
  // Extract last frametime, if it exists
  std::optional<Instant> last_frametime;
  if (!frametimes.empty()) {
    last_frametime = frametimes.back();
  }

  // Set the current time
  current_time = std::chrono::high_resolution_clock::now();
  frametimes.enqueue(current_time);

  // Record the time difference from the last frame
  if (last_frametime) {
    dt_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                current_time - last_frametime.value())
                .count();
  }

  // Make sure the queue never contains more than 100 frametimes
  if (frametimes.size() > 100) {
    frametimes.dequeue();
  }

  // Read console input
  mouse_state_fresh = key_event_fresh = false;

  DWORD event_count;
  MB_ASSERT(GetNumberOfConsoleInputEvents(h_input, &event_count));

  if (event_count > 0) {
    constexpr size_t input_record_buffer_size{128};
    INPUT_RECORD input_record_buffer[input_record_buffer_size];
    MB_ASSERT(ReadConsoleInputW(h_input, input_record_buffer,
                                input_record_buffer_size, &event_count));

    for (size_t i{0}; i < event_count; ++i) {
      switch (input_record_buffer[i].EventType) {
      case MOUSE_EVENT:
        mouse_state = input_record_buffer[i].Event.MouseEvent;
        mouse_state_fresh = true;
        break;
      case KEY_EVENT:
        key_event = input_record_buffer[i].Event.KeyEvent;
        key_event_fresh = true;
        break;
      // Unhandled for now
      case WINDOW_BUFFER_SIZE_EVENT:
        break;
      case FOCUS_EVENT:
        break;
      case MENU_EVENT:
        break;
      default:
        MB_ASSERT(false); // Unrecognized event type
      }
    }
  }
}

State::~State() { clear(h_output); }

Stage::~Stage(){};

void WindowAdjustment::update(State &s) {

  // Get current console dimensions
  auto updated_dimensions = get_console_dimensions(s.h_output);

  // If the adjustment instructions have previously been printed and the
  // console dimensions have not changed, there's nothing to be done
  if (!success_time && updated_dimensions.X == current_dimensions.X &&
      updated_dimensions.Y == current_dimensions.Y) {
    return;
  }

  current_dimensions = updated_dimensions;

  // Check if the current console dimensions match the target console
  // dimensions
  if (current_dimensions.X == screen_width &&
      current_dimensions.Y == screen_height) {

    if (!success_time) {
      success_time = s.current_time;

      // Write success message
      clear(s.h_output);
      s.writer.writeline(L"Running at required resolution.");

    } else {

      static constexpr auto success_message_dur = 2'500ms;
      if (s.current_time >= *success_time + success_message_dur) {
        s.stage = std::make_unique<NameInput>(s);
      }
    }

    return;
  }

  success_time.reset();

  // Print message
  clear(s.h_output);
  s.writer
      .writeline(L"Please adjust your console to meet the required dimensions.")
      .writeline(L"You may need to resize the window and/or decrease the "
                 L"font size.")
      .writeline()
      .writeline(L"Required console dimensions: ")
      .write(L"X: ")
      .writeline(screen_width)
      .write(L"Y: ")
      .writeline(screen_height)
      .writeline()
      .writeline(L"Current console dimensions:")
      .write(L"X: ")
      .write(current_dimensions.X)
      .write(L"\nY: ")
      .writeline(current_dimensions.Y)
      .writeline(
          L"\nAdditionally, it is recommended to set your console to use "
          L"the Consolas font.");
}

NameInput::NameInput(State &s) { s.frametimes.clear(); }

void NameInput::update(State &s) {
  // Copy over the name input background
  for (size_t i = 0; i < screen_size; ++i) {
    s.screen_buffer[i] = s.name_char_buf[i];
    s.color_buffer[i] = s.menu_color_buf[i];
  }

  // Handle keyboard input
  if (s.key_event_fresh && s.key_event->bKeyDown) {
    switch (s.key_event->wVirtualKeyCode) {
    case VK_BACK:
      if (!name.empty()) {
        name.pop_back();
      }
      break;
    case VK_RETURN:
      if (!name.empty() && name.at(0) != L' ') {
        s.player_name = name;
        s.stage = std::make_unique<Menu>(s);
      }
      return;
    default:
      if (is_alphanumeric_vk_code(s.key_event->wVirtualKeyCode) &&
          name.size() <= 31 &&
          !(name.size() == 0 && s.key_event->uChar.UnicodeChar == L' ')) {
        name += s.key_event->uChar.UnicodeChar;
      }
      break;
    }
  }

  // Handle blinking cursor
  static constexpr auto blink_duration = 500ms;

  if (!last_blink_time) {
    last_blink_time = s.current_time;
  }

  if (s.current_time - *last_blink_time > blink_duration) {
    show_cursor = !show_cursor;
    last_blink_time = s.current_time;
  }

  static constexpr size_t cursor_x{44};
  static constexpr size_t cursor_y{21};
  if (show_cursor) {

    s.screen_buffer[cursor_y * screen_width + cursor_x + name.size()] = L'_';
  }

  // Display name

  insert_wstr_in_buffer(
      s.screen_buffer.get() + cursor_y * screen_width + cursor_x, name);

  display_fps(s);

  s.rerender = true;
}

Menu::Menu(State &s) {
  save_game_exists = file_exists("save.bin"s);
  if (save_game_exists) {
    highlighted_option = ContinueGame;
  }
  s.frametimes.clear();
}

void Menu::update(State &s) {
  // Copy over the menu background
  for (size_t i = 0; i < screen_size; ++i) {
    s.screen_buffer[i] = s.menu_char_buf[i];
    s.color_buffer[i] = s.menu_color_buf[i];
  }

  // Grey out Continue Game option if no save game exists
  if (!save_game_exists) {
    static constexpr size_t continue_game_x{49};
    static constexpr size_t continue_game_y{22};
    static constexpr size_t continue_game_width{13};
    for (size_t i{0}; i < continue_game_width; ++i) {
      s.color_buffer[continue_game_y * screen_width + continue_game_x + i] =
          FG_GRAY;
    }
  }

  // Handle keyboard input
  if (s.key_event_fresh && s.key_event->bKeyDown) {
    switch (s.key_event->wVirtualKeyCode) {
    case VK_DOWN:
      highlighted_option = static_cast<MenuOption>(
          (static_cast<int>(highlighted_option) + 1) % 5);
      if (!save_game_exists && highlighted_option == ContinueGame) {
        highlighted_option = NewGame;
      }
      show_arrow = true;
      last_blink_time = s.current_time;
      break;

    case VK_UP:
      highlighted_option = static_cast<MenuOption>(
          (static_cast<int>(highlighted_option) + 4) % 5);
      if (!save_game_exists && highlighted_option == ContinueGame) {
        highlighted_option = Exit;
      }
      show_arrow = true;
      last_blink_time = s.current_time;
      break;

    case VK_RETURN:
      switch (highlighted_option) {
      case ContinueGame:
        // s.stage = std::make_unique<Game>();
        break;
      case NewGame:
        s.stage = std::make_unique<DifficultyPicker>(s);
        break;
      case InventoryOpt:
        s.stage = std::make_unique<Inventory>(s);
        break;
      case ScoreboardOpt:
        s.stage = std::make_unique<Scoreboard>(s);
        break;
      case Exit:
        s.exit = true;
        break;
      }
      return;
    }
  }

  static constexpr auto blink_duration = 500ms;

  if (!last_blink_time) {
    last_blink_time = s.current_time;
  }

  if (s.current_time - *last_blink_time > blink_duration) {
    show_arrow = !show_arrow;
    last_blink_time = s.current_time;
  }

  if (show_arrow) {
    static constexpr size_t arrow_x{46};

    // Find y-coord of highlighted option arrow
    size_t arrow_y;
    switch (highlighted_option) {
    case ContinueGame:
      arrow_y = 22;
      break;
    case NewGame:
      arrow_y = 24;
      break;
    case InventoryOpt:
      arrow_y = 26;
      break;
    case ScoreboardOpt:
      arrow_y = 28;
      break;
    case Exit:
      arrow_y = 30;
      break;
    }

    s.screen_buffer[arrow_y * screen_width + arrow_x] = L'>';
  }

  display_fps(s);

  s.rerender = true;
}

DifficultyPicker::DifficultyPicker(State &s) { s.frametimes.clear(); }

void DifficultyPicker::update(State &s) {
  // Copy over the difficulty picker background
  for (size_t i = 0; i < screen_size; ++i) {
    s.screen_buffer[i] = s.difficulty_char_buf[i];
    s.color_buffer[i] = s.all_white_buf[i];
  }

  // Handle keyboard input
  if (s.key_event_fresh && s.key_event->bKeyDown) {
    switch (s.key_event->wVirtualKeyCode) {
    case VK_DOWN:
      highlighted_option = static_cast<Difficulty>(
          (static_cast<int>(highlighted_option) + 1) % 4);
      show_arrow = true;
      last_blink_time = s.current_time;
      break;

    case VK_UP:
      highlighted_option = static_cast<Difficulty>(
          (static_cast<int>(highlighted_option) + 3) % 4);
      show_arrow = true;
      last_blink_time = s.current_time;
      break;

    case VK_RETURN:
      const auto message = L"Generating map..."s;
      DWORD dummy;
      WriteConsoleOutputCharacterW(s.h_output, L"Generating map...",
                                   message.size(), {43, 31}, &dummy);
      s.stage = std::make_unique<Game>(s, highlighted_option);
      return;
    }
  }

  static constexpr auto blink_duration = 500ms;

  if (!last_blink_time) {
    last_blink_time = s.current_time;
  }

  if (s.current_time - *last_blink_time > blink_duration) {
    show_arrow = !show_arrow;
    last_blink_time = s.current_time;
  }

  if (show_arrow) {
    static constexpr size_t arrow_x{46};

    // Find y-coord of highlighted option arrow
    size_t arrow_y;
    switch (highlighted_option) {
    case Normal:
      arrow_y = 21;
      break;
    case Hard:
      arrow_y = 23;
      break;
    case Insane:
      arrow_y = 25;
      break;
    case Impossible:
      arrow_y = 27;
      break;
    }

    s.screen_buffer[arrow_y * screen_width + arrow_x] = L'>';
  }

  display_fps(s);

  s.rerender = true;
}

Game::Game(State &s, const Difficulty difficulty) : difficulty{difficulty} {
  s.frametimes.clear();
  generate_map(*this, s);
  last_tip_refresh = s.current_time;
}

void Game::update(State &s) {
  // Copy over the game background
  for (size_t i = 0; i < screen_size; ++i) {
    s.screen_buffer[i] = s.game_char_buf[i];
    s.color_buffer[i] = s.all_white_buf[i];
  }

  const auto old_pos_x = pos_x;
  const auto old_pos_y = pos_y;

  const auto old_pos_x_disc = discretize(old_pos_x);
  const auto old_pos_y_disc = discretize(old_pos_y);
  const auto old_pos_disc = old_pos_x_disc + old_pos_y_disc * map_width;

  static constexpr auto speed_boost = 2.0L;
  static constexpr auto speed_boost_duration = 3'000ms;

  // If powerup is active, check if it has expired
  if (last_powerup_pickup &&
      s.current_time - last_powerup_pickup.value() >= speed_boost_duration) {
    last_powerup_pickup = std::nullopt;
  }

  // Kinematics

  // Player kinematics
  auto player_movement_up = static_cast<bool>(GetAsyncKeyState('W') & 0x8000);
  auto player_movement_left = static_cast<bool>(GetAsyncKeyState('A') & 0x8000);
  auto player_movement_down = static_cast<bool>(GetAsyncKeyState('S') & 0x8000);
  auto player_movement_right =
      static_cast<bool>(GetAsyncKeyState('D') & 0x8000);

  auto vertical_mash = player_movement_up && player_movement_down;
  auto horizontal_mash = player_movement_left && player_movement_right;

  player_movement_up &= !vertical_mash;
  player_movement_down &= !vertical_mash;
  player_movement_left &= !horizontal_mash;
  player_movement_left &= start_time.has_value();
  player_movement_right &= !horizontal_mash;

  const auto dt_ms = static_cast<long double>(s.dt_ns) / ns_in_ms;

  pos_y -= player_movement_up *
           (last_powerup_pickup.has_value() ? speed_boost : 1.0L) * dt_ms /
           base_move_duration.count();
  pos_y += player_movement_down *
           (last_powerup_pickup.has_value() ? speed_boost : 1.0L) * dt_ms /
           base_move_duration.count();

  pos_x -= player_movement_left *
           (last_powerup_pickup.has_value() ? speed_boost : 1.0L) * dt_ms /
           base_move_duration.count();
  pos_x += player_movement_right *
           (last_powerup_pickup.has_value() ? speed_boost : 1.0L) * dt_ms /
           base_move_duration.count();

  auto pos_x_disc = discretize(pos_x);
  auto pos_y_disc = discretize(pos_y);
  auto pos_disc = pos_x_disc + pos_y_disc * map_width;

  // Foe kinematics
  if (start_time) {
    for (size_t i{0}; i < foe_count; ++i) {
      const auto foe_pos_x_disc = discretize(foe_pos_x[i]);
      const auto foe_pos_y_disc = discretize(foe_pos_y[i]);
      const auto foe_pos_disc = foe_pos_x_disc + foe_pos_y_disc * map_width;

      // Find relative direction of player
      const auto player_to_right = pos_x_disc > foe_pos_x_disc;
      const auto x_diff = abs_diff(pos_x_disc, foe_pos_x_disc);
      const auto player_to_down = pos_y_disc > foe_pos_y_disc;
      const auto y_diff = abs_diff(pos_y_disc, foe_pos_y_disc);

      const size_t neighbouring_cells[4] = {foe_pos_disc - 1, foe_pos_disc + 1,
                                            foe_pos_disc - map_width,
                                            foe_pos_disc + map_width};

      const MapCell neighbouring_cell_types[4] = {
          map[neighbouring_cells[0]], map[neighbouring_cells[1]],
          map[neighbouring_cells[2]], map[neighbouring_cells[3]]};

      size_t preference[4]{};
      if (x_diff > y_diff) {
        preference[0] = player_to_right ? 1 : 0;
        preference[1] = player_to_down ? 3 : 2;
        preference[2] = player_to_down ? 2 : 3;
        preference[3] = player_to_right ? 0 : 1;
      } else {
        preference[0] = player_to_down ? 3 : 2;
        preference[1] = player_to_right ? 1 : 0;
        preference[2] = player_to_right ? 0 : 1;
        preference[3] = player_to_down ? 2 : 3;
      }

      size_t selected_direction = 5;
      for (size_t i{0}; i < 4; ++i) {
        if (neighbouring_cell_types[preference[i]] != Wall &&
            neighbouring_cell_types[preference[i]] != Foe &&
            neighbouring_cell_types[preference[i]] != Start &&
            neighbouring_cell_types[preference[i]] != Finish) {
          selected_direction = preference[i];
          break;
        }
      }

      switch (selected_direction) {
      case 0: {
        foe_pos_x[i] -= dt_ms / foe_move_duration(difficulty).count();
      } break;
      case 1: {
        foe_pos_x[i] += dt_ms / foe_move_duration(difficulty).count();
      } break;
      case 2: {
        foe_pos_y[i] -= dt_ms / foe_move_duration(difficulty).count();
      } break;
      case 3: {
        foe_pos_y[i] += dt_ms / foe_move_duration(difficulty).count();
      } break;
      }
    }
  }

  // Check collisions and interactions with game items
  const auto new_cell = map[pos_disc];

  // Check for game start
  if (!start_time && new_cell != Start) {
    start_time = s.current_time;
  }

  // If ran into a wall or back into start position, revert to old position
  if (new_cell == Wall || (start_time && new_cell == Start)) {
    pos_x = old_pos_x;
    pos_y = old_pos_y;
    pos_x_disc = old_pos_x_disc;
    pos_y_disc = old_pos_y_disc;
    pos_disc = old_pos_disc;
    notification = L"Bumped into wall"s;
    notification_color = FG_WHITE;
    notification_start = s.current_time;
  }

  if (new_cell == PowerUp) {
    last_powerup_pickup = s.current_time;
    map[pos_disc] = Empty;
    notification = L"POWERED UP: 2x speed for the next three seconds!"s;
    notification_color = map_cell_color(PowerUp);
    notification_start = s.current_time;
  }

  if (new_cell == Trophy) {
    trophy_collected = true;
    map[pos_disc] = Empty;
    notification = L"Trophy collected! +1000 score bonus!"s;
    notification_color = map_cell_color(Trophy);
    add_trophy();
  }

  // Update player position on map
  if (old_pos_x_disc > 0) {
    map[old_pos_disc] = Empty;
  }
  if (pos_x_disc > 0) {
    map[pos_disc] = Player;
  }

  // Check pause
  if (s.key_event_fresh && s.key_event->bKeyDown &&
      s.key_event->wVirtualKeyCode == VK_ESCAPE) {
    s.stage = std::make_unique<PauseMenu>(s, *this);
    return;
  }

  // Check pathfinder toggle
  if (processed_one_space_keyup && s.key_event_fresh && s.key_event->bKeyDown &&
      s.key_event->wVirtualKeyCode == VK_SPACE) {
    pathfinder = !pathfinder;
    processed_one_space_keyup = false;
  }

  if (s.key_event_fresh && !s.key_event->bKeyDown &&
      s.key_event->wVirtualKeyCode == VK_SPACE) {
    processed_one_space_keyup = true;
  }

  // Check autopilot toggle
  if (processed_one_enter_keyup && s.key_event_fresh && s.key_event->bKeyDown &&
      s.key_event->wVirtualKeyCode == VK_RETURN) {
    autopilot = !autopilot;
    processed_one_enter_keyup = false;
  }

  if (s.key_event_fresh && !s.key_event->bKeyDown &&
      s.key_event->wVirtualKeyCode == VK_RETURN) {
    processed_one_enter_keyup = true;
  }

  static constexpr auto initial_score = 1'000LL;
  const auto time_elapsed =
      s.current_time - start_time.value_or(s.current_time);
  const auto score =
      initial_score + static_cast<long long>(difficulty) * 250 -
      10 * (std::chrono::duration_cast<std::chrono::milliseconds>(time_elapsed)
                .count() /
            500) +
      trophy_collected * 1'000;

  // If ran into a foe, it's game over
  for (size_t i{0}; i < foe_count; ++i) {
    const auto foe_pos_x_disc = discretize(foe_pos_x[i]);
    const auto foe_pos_y_disc = discretize(foe_pos_y[i]);
    const auto foe_pos_disc = foe_pos_x_disc + foe_pos_y_disc * map_width;
    if (foe_pos_disc == pos_disc) {
      const auto final_score = score / 5;
      add_score(final_score, s.player_name.value());
      s.stage = std::make_unique<GameOver>(s, final_score, difficulty, false);
      return;
    }
  }

  // Check for win
  if (new_cell == Finish) {
    add_score(score, s.player_name.value());
    s.stage = std::make_unique<GameOver>(s, score, difficulty, true);
    return;
  }

  // Update tips
  static constexpr auto tip_duration = 10'000ms;
  if (s.current_time - last_tip_refresh >= tip_duration) {
    ++tip_index;
    tip_index %= tips_count;
    last_tip_refresh = s.current_time;
  }

  // Reset notification, if needed
  static constexpr auto notification_duration = 1'500ms;
  if (notification_start &&
      s.current_time - notification_start.value() >= notification_duration) {
    notification = L""s;
    notification_color = FG_WHITE;
    notification_start = std::nullopt;
  }

  // Rendering

  // First row
  const auto score_str = pad_left(std::to_wstring(score), 7);
  const auto time_str = pad_left(
      round_to_1dp(
          std::to_wstring(std::chrono::duration_cast<std::chrono::milliseconds>(
                              time_elapsed)
                              .count() /
                          1'000) +
          L"s"),
      7);

  insert_wstr_in_buffer(s.screen_buffer.get() + 9, score_str);
  insert_wstr_in_buffer(s.screen_buffer.get() + 28, time_str);
  insert_wstr_in_buffer(s.screen_buffer.get() + 42, notification);
  for (size_t i{42}; i < screen_width - 10; ++i) {
    s.color_buffer[i] = notification_color;
  }

  // Second row
  static constexpr size_t keybind_x_count{16};
  const size_t keybind_x[keybind_x_count]{3,  15, 24, 29, 43, 58, 59,  60,
                                          75, 76, 77, 97, 98, 99, 100, 101};
  for (size_t i{0}; i < keybind_x_count; ++i) {
    s.color_buffer[screen_width + keybind_x[i]] = FG_BLUE;
  }

  // Third row
  static constexpr size_t third_row_offset{2 * screen_width};
  s.color_buffer[third_row_offset + 2] = map_cell_color(Player);
  s.color_buffer[third_row_offset + 14] = map_cell_color(Start);
  s.color_buffer[third_row_offset + 28] = map_cell_color(Finish);
  s.color_buffer[third_row_offset + 43] = map_cell_color(Foe);
  s.color_buffer[third_row_offset + 59] = map_cell_color(PowerUp);
  s.color_buffer[third_row_offset + 74] = map_cell_color(Trophy);
  s.color_buffer[third_row_offset + 88] = map_cell_color(Wall);

  // Fourth row
  static constexpr size_t fourth_row_offset{3 * screen_width};
  insert_wstr_in_buffer(s.screen_buffer.get() + fourth_row_offset + 8,
                        pad_right(tips[tip_index], 110));

  // Display map
  // Add foes to map
  MapCell old_cells[5];
  for (size_t i{0}; i < foe_count; ++i) {
    const auto foe_pos_x_disc = discretize(foe_pos_x[i]);
    const auto foe_pos_y_disc = discretize(foe_pos_y[i]);
    const auto foe_pos_disc = foe_pos_x_disc + foe_pos_y_disc * map_width;
    old_cells[i] = map[foe_pos_disc];
    map[foe_pos_disc] = Foe;
  }

  static constexpr size_t map_offset_x{4};
  static constexpr size_t map_offset_y{6};
  for (size_t y{0}; y < map_height; ++y) {
    for (size_t x{0}; x < map_width; ++x) {
      const auto cell = map[y * map_width + x];
      s.screen_buffer[(y + map_offset_y) * screen_width + x + map_offset_x] =
          map_cell_char(cell);
      s.color_buffer[(y + map_offset_y) * screen_width + x + map_offset_x] =
          map_cell_color(cell);
    }
  }

  // Display the shortest path if pathfinder is enabled
  if (pathfinder) {
    const auto path = shortest_path(map, pos_disc, finish_idx);
    for (const auto path_idx : path) {
      const auto path_x = path_idx % map_width;
      const auto path_y = path_idx / map_width;
      s.color_buffer[(path_y + map_offset_y) * screen_width + path_x +
                     map_offset_x] |= BG_GREEN;
    }
  }

  // Remove foes from map
  for (size_t i{0}; i < foe_count; ++i) {
    const auto foe_pos_x_disc = discretize(foe_pos_x[i]);
    const auto foe_pos_y_disc = discretize(foe_pos_y[i]);
    const auto foe_pos_disc = foe_pos_x_disc + foe_pos_y_disc * map_width;
    map[foe_pos_disc] = old_cells[i];
    if (map[foe_pos_disc] == Foe) {
      map[foe_pos_disc] = Empty;
    }
  }

  for (size_t i = 0; i < map_size; ++i) {
    auto b = static_cast<int>(map[i]);
    MB_ASSERT(b < 8);
  }

  display_fps(s);

  s.rerender = true;
}

void generate_map(Game &game, State &s) {
  // Initialize map with empty cells
  for (size_t i{0}; i < map_size; ++i) {
    game.map[i] = Empty;
  }

  // Draw in the four walls

  // Top wall
  for (size_t i{0}; i < map_width; ++i) {
    game.map[i] = Wall;
  }

  // Bottom wall
  for (size_t i{0}; i < map_width; ++i) {
    game.map[map_size - map_width + i] = Wall;
  }

  // Left wall
  for (size_t i{0}; i < map_height; ++i) {
    game.map[i * map_width] = Wall;
  }

  // Right wall
  for (size_t i{0}; i < map_height; ++i) {
    game.map[i * map_width + map_width - 1] = Wall;
  }

  // Pick random start and finish positions on the left and right walls
  // respectively
  std::uniform_int_distribution<size_t> y_dist{1, map_height - 2};
  const auto start_y = y_dist(s.gen);
  const auto finish_y = y_dist(s.gen);

  game.start_idx = start_y * map_width;
  game.finish_idx = finish_y * map_width + map_width - 1;

  game.map[game.start_idx] = Start;
  game.map[game.finish_idx] = Finish;

  game.pos_y = static_cast<long double>(start_y);

  std::uniform_int_distribution<size_t> x_dist{1, map_width - 2};

  // Generate walls
  do {
    std::uniform_int_distribution<size_t> wall_length_dist{3, 10};
    std::uniform_int_distribution<size_t> wall_direction_dist{0, 1};

    for (size_t i{0}; i < 25; ++i) {
      const auto wall_length = wall_length_dist(s.gen);
      const auto wall_direction = wall_direction_dist(s.gen);
      if (wall_direction == 0) {
        auto wall_x_dist = std::uniform_int_distribution<size_t>{
            1, map_width - 1 - wall_length};
        const auto wall_x = wall_x_dist(s.gen);
        const auto wall_y = y_dist(s.gen);
        for (size_t i{0}; i < wall_length; ++i) {
          game.map[wall_y * map_width + wall_x + i] = Wall;
        }
      } else {
        auto wall_y_dist = std::uniform_int_distribution<size_t>{
            1, map_height - 1 - wall_length};
        const auto wall_x = x_dist(s.gen);
        const auto wall_y = wall_y_dist(s.gen);
        for (size_t i{0}; i < wall_length; ++i) {
          game.map[(wall_y + i) * map_width + wall_x] = Wall;
        }
      }
    }
  } while (shortest_path(game.map, game.start_idx, game.finish_idx).size() ==
           0);

  // Generate foes
  switch (game.difficulty) {
  case Normal:
    game.foe_count = 1;
    break;
  case Hard:
    game.foe_count = 2;
    break;
  case Insane:
    game.foe_count = 3;
    break;
  case Impossible:
    game.foe_count = 4;
    break;
  }

  std::uniform_int_distribution<size_t> right_half_x_dist{map_width / 2 - 1,
                                                          map_width - 2};
  size_t foes_placed = 0;
  while (foes_placed < game.foe_count) {
    const auto x = right_half_x_dist(s.gen);
    const auto y = y_dist(s.gen);
    const auto idx = y * map_width + x;
    if (game.map[idx] == Empty) {
      game.foe_pos_x[foes_placed] = x;
      game.foe_pos_y[foes_placed] = y;
      ++foes_placed;
    }
  }

  // Generate powerups
  for (size_t i{0}; i < 5; ++i) {
    const auto x = x_dist(s.gen);
    const auto y = y_dist(s.gen);
    const auto idx = y * map_width + x;
    if (game.map[idx] == Empty) {
      game.map[idx] = PowerUp;
    }
  }

  // Generate trophy
  size_t trophy_idx;
  while (true) {
    const auto x = right_half_x_dist(s.gen);
    const auto y = y_dist(s.gen);
    trophy_idx = y * map_width + x;
    if (game.map[trophy_idx] == Empty) {
      game.map[trophy_idx] = Trophy;
    } else {
      continue;
    }
    auto shortest_path_to_finish =
        shortest_path(game.map, trophy_idx, game.finish_idx);
    if (shortest_path_to_finish.size() >= 4) {
      break;
    } else {
      game.map[trophy_idx] = Empty;
    }
  };

  for (size_t i = 0; i < map_size; ++i) {
    auto b = static_cast<int>(game.map[i]);
    MB_ASSERT(b < 8);
  }
}

PauseMenu::PauseMenu(State &s, const Game &game) : paused_game(game) {
  s.frametimes.clear();
}

void PauseMenu::update(State &s) {
  // Copy over the pause menu background
  for (size_t i = 0; i < screen_size; ++i) {
    s.screen_buffer[i] = s.pause_menu_buf[i];
    s.color_buffer[i] = s.all_white_buf[i];
  }

  // Handle keyboard input
  if (s.key_event_fresh && s.key_event->bKeyDown) {
    switch (s.key_event->wVirtualKeyCode) {
    case VK_DOWN:
      highlighted_option = static_cast<PauseMenuOption>(
          (static_cast<int>(highlighted_option) + 1) % 3);
      show_arrow = true;
      last_blink_time = s.current_time;
      break;

    case VK_UP:
      highlighted_option = static_cast<PauseMenuOption>(
          (static_cast<int>(highlighted_option) + 2) % 3);
      show_arrow = true;
      last_blink_time = s.current_time;
      break;

    case VK_RETURN:
      switch (highlighted_option) {
      case Resume:
        s.stage = std::make_unique<Game>(paused_game);
        break;
      case RestartP:
        s.stage = std::make_unique<Game>(s, paused_game.difficulty);
        break;
      case ReturnToMenuP:
        s.stage = std::make_unique<Menu>(s);
        break;
      }
      return;
    }
  } else if (s.key_event_fresh && !s.key_event->bKeyDown &&
             s.key_event->wVirtualKeyCode == VK_ESCAPE) {
    if (processed_one_escape_keyup) {
      s.stage = std::make_unique<Game>(paused_game);
    } else {
      processed_one_escape_keyup = true;
    }
  }

  // Produce blink effect
  static constexpr auto blink_duration = 500ms;

  if (!last_blink_time) {
    last_blink_time = s.current_time;
  }

  if (s.current_time - *last_blink_time > blink_duration) {
    show_arrow = !show_arrow;
    last_blink_time = s.current_time;
  }

  if (show_arrow) {
    static constexpr size_t arrow_x{46};

    // Find y-coord of highlighted option arrow
    size_t arrow_y;
    switch (highlighted_option) {
    case Resume:
      arrow_y = 18;
      break;
    case RestartP:
      arrow_y = 20;
      break;
    case ReturnToMenuP:
      arrow_y = 22;
      break;
    }

    s.screen_buffer[arrow_y * screen_width + arrow_x] = L'>';
  }

  display_fps(s);

  s.rerender = true;
}

GameOver::GameOver(State &s, const long long score, const Difficulty difficulty,
                   const bool won)
    : score{score}, difficulty{difficulty}, won{won} {
  s.frametimes.clear();
}

void GameOver::update(State &s) {
  // Copy over the gameover background
  for (size_t i = 0; i < screen_size; ++i) {
    s.screen_buffer[i] = s.game_over_buf[i];
    s.color_buffer[i] = s.all_white_buf[i];
  }

  // Print result
  const auto result = L"YOU "s + (won ? L"WON!"s : L"LOST!"s);
  insert_wstr_in_buffer(s.screen_buffer.get() + 14 * screen_width + 42, result);

  // Print score
  const auto score_str = pad_left(std::to_wstring(score), 11);
  insert_wstr_in_buffer(s.screen_buffer.get() + 16 * screen_width + 67,
                        score_str);

  // Handle keyboard input
  if (s.key_event_fresh && s.key_event->bKeyDown) {
    switch (s.key_event->wVirtualKeyCode) {
    case VK_DOWN:
      highlighted_option = static_cast<GameOverOption>(
          (static_cast<int>(highlighted_option) + 1) % 2);
      show_arrow = true;
      last_blink_time = s.current_time;
      break;

    case VK_UP:
      highlighted_option = static_cast<GameOverOption>(
          (static_cast<int>(highlighted_option) + 1) % 2);
      show_arrow = true;
      last_blink_time = s.current_time;
      break;

    case VK_RETURN:
      switch (highlighted_option) {
      case Restart:
        s.stage = std::make_unique<Game>(s, difficulty);
        break;
      case ReturnToMenu:
        s.stage = std::make_unique<Menu>(s);
        break;
      }
      return;
    }
  }

  static constexpr auto blink_duration = 500ms;

  if (!last_blink_time) {
    last_blink_time = s.current_time;
  }

  if (s.current_time - *last_blink_time > blink_duration) {
    show_arrow = !show_arrow;
    last_blink_time = s.current_time;
  }

  if (show_arrow) {
    static constexpr size_t arrow_x{46};

    // Find y-coord of highlighted option arrow
    size_t arrow_y;
    switch (highlighted_option) {
    case Restart:
      arrow_y = 21;
      break;
    case ReturnToMenu:
      arrow_y = 23;
      break;
    }

    s.screen_buffer[arrow_y * screen_width + arrow_x] = L'>';
  }

  display_fps(s);

  s.rerender = true;
}

Scoreboard::Scoreboard(State &s) { s.frametimes.clear(); }

void Scoreboard::update(State &s) {
  // Copy over the inventory background
  for (size_t i = 0; i < screen_size; ++i) {
    s.screen_buffer[i] = s.scoreboard_buf[i];
    s.color_buffer[i] = s.all_white_buf[i];
  }

  // Handle keyboard input
  if (processed_keyup && s.key_event_fresh && s.key_event->bKeyDown &&
      s.key_event->wVirtualKeyCode == VK_RETURN) {
    s.stage = std::make_unique<Menu>(s);
  }

  if (s.key_event_fresh && !s.key_event->bKeyDown &&
      s.key_event->wVirtualKeyCode == VK_RETURN) {
    processed_keyup = true;
  }

  // List all scores
  const auto &scores = get_scores();
  for (size_t i{0}; i < scores.size(); ++i) {
    insert_wstr_in_buffer(s.screen_buffer.get() + screen_width * (14 + i) + 42,
                          pad_right(std::wstring(scores.get(i).name), 25));
    insert_wstr_in_buffer(s.screen_buffer.get() + screen_width * (14 + i) + 68,
                          pad_left(std::to_wstring(scores.get(i).score), 10)

    );
  }

  static constexpr auto blink_duration = 500ms;

  if (!last_blink_time) {
    last_blink_time = s.current_time;
  }

  if (s.current_time - *last_blink_time > blink_duration) {
    show_arrow = !show_arrow;
    last_blink_time = s.current_time;
  }

  if (show_arrow) {
    static constexpr size_t arrow_x{46};
    static constexpr size_t arrow_y{33};

    s.screen_buffer[arrow_y * screen_width + arrow_x] = L'>';
  }

  display_fps(s);

  s.rerender = true;
}

Inventory::Inventory(State &s) { s.frametimes.clear(); }

void Inventory::update(State &s) {
  // Copy over the inventory background
  for (size_t i = 0; i < screen_size; ++i) {
    s.screen_buffer[i] = s.inventory_buf[i];
    s.color_buffer[i] = s.all_white_buf[i];
  }

  // Handle keyboard input
  if (processed_keyup && s.key_event_fresh && s.key_event->bKeyDown &&
      s.key_event->wVirtualKeyCode == VK_RETURN) {
    s.stage = std::make_unique<Menu>(s);
  }

  if (s.key_event_fresh && !s.key_event->bKeyDown &&
      s.key_event->wVirtualKeyCode == VK_RETURN) {
    processed_keyup = true;
  }

  // List all trophies
  const auto &trophies = get_trophies();
  for (size_t i{0}; i < trophies.size(); ++i) {
    insert_wstr_in_buffer(s.screen_buffer.get() + screen_width * (14 + i) + 42,
                          std::to_wstring(i + 1));
    insert_wstr_in_buffer(s.screen_buffer.get() + screen_width * (14 + i) + 45,
                          pad_left(trophies.get(i), 33)

    );
  }

  static constexpr auto blink_duration = 500ms;

  if (!last_blink_time) {
    last_blink_time = s.current_time;
  }

  if (s.current_time - *last_blink_time > blink_duration) {
    show_arrow = !show_arrow;
    last_blink_time = s.current_time;
  }

  if (show_arrow) {
    static constexpr size_t arrow_x{46};
    static constexpr size_t arrow_y{33};

    s.screen_buffer[arrow_y * screen_width + arrow_x] = L'>';
  }

  display_fps(s);

  s.rerender = true;
}

int calc_fps(Queue<Instant> &frametimes) {
  if (frametimes.size() < 2) {
    return 0;
  }

  const auto frame_count = frametimes.size() - 1;

  // Get the difference in nanoseconds between the latest frame time and the
  // earliest frame time in queue
  const auto diff_ns = static_cast<long double>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(frametimes.back() -
                                                           frametimes.front())
          .count());

  const auto avg_frametime_s =
      diff_ns / static_cast<long double>(frame_count) / (ns_in_ms * 1'000.0L);

  return static_cast<int>(1.0L / avg_frametime_s);
}

// Inserts FPS information in the screen buffer
void display_fps(State &s) {
  const auto fps_str = pad_left(std::to_wstring((calc_fps(s.frametimes))), 6);
  insert_wstr_in_buffer(s.screen_buffer.get() + (screen_width - 11), L" FPS:");
  insert_wstr_in_buffer(s.screen_buffer.get() + (screen_width - 6), fps_str);
}

static constexpr auto scores_filename = "scores.bin";

void add_score(const long long score, const std::wstring &player_name) {
  SavedScore score_record;
  score_record.score = score;
  wcscpy_s(score_record.name, 64, player_name.c_str());
  OutputDebugStringW(score_record.name);
  append_bytes(scores_filename, &score_record);
}

// Gets the 15 highest scores
DoublyLinkedList<SavedScore> get_scores() {

  SavedScore score_record_buffer[64];
  size_t count = read_bytes(scores_filename, score_record_buffer, 64);

  long long scores[64];
  for (size_t i{0}; i < count; ++i) {
    scores[i] = score_record_buffer[i].score;
  }
  insertion_sort(scores, count);
  reverse(scores, count);

  DoublyLinkedList<SavedScore> score_records{};
  size_t count_needed = min(count, 15);
  for (size_t i{0}; i < count_needed; ++i) {
    // Find the index of the score in the buffer
    size_t index = 0;
    for (; index < count; ++index) {
      if (score_record_buffer[index].score == scores[i]) {
        break;
      }
    }
    // Add to list
    score_records.push_back(score_record_buffer[index]);
  }

  return score_records;
}
