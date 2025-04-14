static constexpr size_t road_item_width(RoadItemType);
static constexpr size_t road_item_height(RoadItemType);

static constexpr auto cell_length_m =
    0.5L; // The side of each console cell is taken to correspond to 0.5m
static constexpr size_t visible_road_cells{37};
static constexpr auto visible_road_length = cell_length_m * visible_road_cells;

struct Game : virtual Stage {
  long double score{0.0L};
  long double time_ns{0.0L};
  long double distance_m{0.0L};
  long double pos_x{17.0L}; // Center car on screen
  long double speed_ms{0.0L};
  long double acceleration_ms2{0.0L};
  Queue<RoadItem> road_items{
      {Obstacle, 60, 7.0L}, {Boost, 42, 8.0L}, {Obstacle, 16, 16.0L}};
  bool started = false;
  bool pathfinder_enabled = false;
  bool processed_one_space_keyup = true;

  Instant last_countdown_update;
  int countdown = 3;

  bool was_paused = false;

  Game(State &s);
  Game(const Game &) = default;
  virtual void update(State &s);
};

void Game::update(State &s) { // Set background
  for (size_t i{0}; i < screen_size; ++i) {
    s.screen_buffer[i] = s.game_char_buf[i];
    s.color_buffer[i] = s.game_color_buf[i];
  }

  // If returning from a pause, reset the countdown
  if (was_paused && !started) {
    countdown = 3;
    Instant i;
    last_countdown_update = i;
    was_paused = false;
  }

  // Update countdown
  const auto time_since_last_countdown_update =
      s.current_time - last_countdown_update;
  const auto should_update_countdown =
      time_since_last_countdown_update > (countdown == 0 ? 2000ms : 1000ms);
  if (countdown >= 0 && should_update_countdown) {
    last_countdown_update = s.current_time;
    if (--countdown == 0) {
      started = true;
    }
  }

  // Kinematic calculations
  if (started) {
    static constexpr auto top_speed_ms = 40.0L;
    static constexpr auto top_acceleration_ms2 = 6.0L;
    static constexpr auto min_coast_speed_ms = 6.0L;
    static constexpr auto coast_multiplier =
        0.08L; // A measure of the vehicle's engine braking
    static constexpr auto brake_deceleration = 9.0L;

    const auto dt_s = static_cast<long double>(s.dt_ns) / ns_in_s;

    if (time_ns > 0.0L) {
      const auto distance_changed = speed_ms * dt_s;
      distance_m += distance_changed;
      speed_ms =
          max(min(speed_ms + acceleration_ms2 * dt_s, top_speed_ms), 0.0L);

      score += distance_changed * 0.1L + speed_ms * speed_ms * dt_s * 0.05L;

      const auto should_accel = GetAsyncKeyState('W') & 0x8000;
      const auto should_decel = GetAsyncKeyState('S') & 0x8000;

      if (should_accel) {
        const auto diff = top_speed_ms - speed_ms;
        const auto x = diff / top_speed_ms;
        acceleration_ms2 = x * top_acceleration_ms2;
      } else if (should_decel) {
        acceleration_ms2 = -brake_deceleration;
      } else {
        acceleration_ms2 =
            -max(speed_ms - min_coast_speed_ms, 0.0L) * coast_multiplier;
      }

      const auto lateral_movement_speed_ms = 14.0L + 1.2L * speed_ms;

      const auto should_move_left = GetAsyncKeyState('A') & 0x8000;
      const auto should_move_right = GetAsyncKeyState('D') & 0x8000;

      if (should_move_left) {
        pos_x -= lateral_movement_speed_ms * dt_s;
      } else if (should_move_right) {
        pos_x += lateral_movement_speed_ms * dt_s;
      }

      pos_x = max(min(pos_x, 34.5L), 0.0L);
    }

    time_ns += static_cast<long double>(s.dt_ns);
  }

  // Update road items

  // Dequeue all road items that the car has passed
  while (!road_items.empty() && road_items.front().distance_m < distance_m) {
    road_items.dequeue();
  }

  // Check for collision with any road item
  const auto pos_x_cells = static_cast<size_t>(roundl(pos_x / cell_length_m));
  const auto road_item_list_ = road_items.list();
  size_t idx{0};
  for (const auto &item : road_item_list_) {
    if (item.distance_m - distance_m <
        static_cast<long double>(car_height) * cell_length_m) {

      const auto item_left = item.pos_x;
      const auto item_right = item_left + road_item_width(item.type);

      const auto car_left = pos_x_cells;
      const auto car_right = pos_x_cells + car_width;

      auto left_collision = item_left >= car_left && item_left < car_right;
      auto right_collision = item_right > car_left && item_right <= car_right;

      if (left_collision || right_collision) {
        const auto final_score = static_cast<size_t>(roundl(score)) / 5 * 5;
        switch (item.type) {
        case Obstacle:
          add_score(final_score);
          s.stage = std::make_unique<GameOver>(s, final_score);
          return;
        case Boost:
          score += 250;
          break;
        case Trophy:
          score += 1000;
          add_trophy();
          break;
        }

        // Remove current item from queue
        road_items.list().remove(idx);
      }
    }
    ++idx;
  }

  // Add new road items if necessary
  const auto &item_list = road_items.list();
  const auto next_stretch_distance = distance_m + visible_road_length;
  size_t items_in_next_stretch = 0;
  for (const auto &item : item_list) {
    if (item.distance_m >= next_stretch_distance &&
        item.distance_m < next_stretch_distance + visible_road_length) {
      items_in_next_stretch += 1;
    }
  }

  if (items_in_next_stretch < 1) {
    std::uniform_int_distribution<size_t> count_distrib{0, 2};
    size_t count{count_distrib(s.gen)};

    std::unique_ptr<long double[]> new_item_distances{new long double[count]};
    for (size_t i{0}; i < count; ++i) {
      std::uniform_real_distribution<long double> distance_distrib{
          next_stretch_distance, next_stretch_distance + visible_road_length};
      new_item_distances[i] = distance_distrib(s.gen);
    }
    insertion_sort(new_item_distances.get(), count);

    for (size_t i{0}; i < count; ++i) {
      std::uniform_real_distribution<double> unit_distrib(0.0, 1.0);
      double prob = unit_distrib(s.gen);

      RoadItemType new_item_type;
      if (prob > 0.1) {
        new_item_type = Obstacle;
      } else if (prob > 0.01) {
        new_item_type = Boost;
      } else {
        new_item_type = Trophy;
      }

      std::uniform_int_distribution<size_t> pos_x_distrib{
          0, 100 - road_item_width(new_item_type)};
      const auto new_item_pos_x = pos_x_distrib(s.gen);

      road_items.enqueue(
          {new_item_type, new_item_pos_x, new_item_distances[i]});
    }
  }

  // Pathfinder logic
  if (s.key_event_fresh && s.key_event->bKeyDown &&
      s.key_event->wVirtualKeyCode == VK_SPACE && processed_one_space_keyup) {
    pathfinder_enabled = !pathfinder_enabled;
    processed_one_space_keyup = false;
  }

  if (s.key_event_fresh && !s.key_event->bKeyDown &&
      s.key_event->wVirtualKeyCode == VK_SPACE) {
    processed_one_space_keyup = true;
  }

  // The pathfinder determines the shortest path from the cell to the front of
  // the car, to the center cell in the top row of the visible road area, while
  // avoiding obstacles. The pathfinder uses a graph with a vertex for each cell
  // in the visible road area in front of the car
  std::optional<DoublyLinkedList<size_t>> path;
  if (pathfinder_enabled) {
    static constexpr auto concerned_road_area_cells =
        10 + visible_road_cells - car_height;
    static constexpr auto concerned_road_area_length =
        concerned_road_area_cells * cell_length_m;
    static constexpr auto road_graph_size = concerned_road_area_cells * 80;
    Graph road_graph{road_graph_size};

    // Add edges between adjacent cells
    for (size_t i{0}; i < road_graph_size; ++i) {
      if (i % 80 < 79) {
        road_graph.add_undirected_edge(i, i + 1);
      }
      if (i / 80 < road_graph_size / 80 - 1) {
        road_graph.add_undirected_edge(i, i + 80);
      }
    }
    road_graph.add_undirected_edge(road_graph_size - 1, road_graph_size - 80);
    road_graph.add_undirected_edge(road_graph_size - 1, road_graph_size - 2);

    // Check which cells in the concerned road area each obstacle occupies, and
    // for each such cell, remove the corresponding vertex from the graph
    const auto obstacle_list = road_items.list();
    for (const auto &obstacle : obstacle_list) {
      if (obstacle.type == Obstacle) {
        const auto concerned_road_area_distance_m =
            distance_m + static_cast<long double>(car_height) * cell_length_m;
        const auto obstacle_distance_m = obstacle.distance_m;
        const auto diff = obstacle_distance_m - concerned_road_area_distance_m;
        if (obstacle_distance_m >= concerned_road_area_distance_m &&
            diff < concerned_road_area_length) {
          const auto cells_ahead =
              static_cast<size_t>(roundl(diff / cell_length_m));
          if (cells_ahead >= concerned_road_area_cells) {
            continue;
          }
          const auto obstacle_width = road_item_width(Obstacle);
          const auto obstacle_pos_x = min(obstacle.pos_x, 80 - obstacle_width);

          for (size_t i{0}; i < obstacle_width; ++i) {
            const auto obstacle_cell_idx =
                cells_ahead * 80 + obstacle_pos_x + i;
            road_graph.remove_vertex(obstacle_cell_idx);
          }
        }
      }
    }

    const auto car_center_x = pos_x_cells + car_width / 2;
    // Find the cell in the top row of the visible road area that is not
    // isolated and is closest to the car's x-position
    size_t end = road_graph_size - 41;
    size_t diff = SIZE_MAX;
    for (size_t i{0}; i < 80; ++i) {
      const auto cell = road_graph_size - 80 + i;
      if (road_graph.is_isolated(cell)) {
        continue;
      }
      const auto diff_candidate = abs_diff(car_center_x, cell % 80);
      if (diff_candidate < diff) {
        end = cell;
        diff = diff_candidate;
      }
    }

    // Find the shortest path from the cell at the front of the car to the
    // center cell in the top row of the visible road area
    path = road_graph.breadth_first_search(car_center_x, end);
  }

  // Check if need to pause
  auto should_pause = GetAsyncKeyState(VK_ESCAPE) & 0x8000;
  if (should_pause) {
    s.stage = std::make_unique<PauseMenu>(s, *this);
    return;
  }

  // Rendering

  // Draw countdown blocks if in countdown
  if (countdown >= 0) {
    static constexpr size_t countdown_pos{1014};
    switch (countdown) {
    case 3:
      insert_rect(s.screen_buffer.get(), screen_size, screen_width,
                  countdown_pos, three_width, three_height,
                  s.three_char_buf.get());
      break;
    case 2:
      insert_rect(s.screen_buffer.get(), screen_size, screen_width,
                  countdown_pos, two_width, two_height, s.two_char_buf.get());
      break;
    case 1:
      insert_rect(s.screen_buffer.get(), screen_size, screen_width,
                  countdown_pos, one_width, one_height, s.one_char_buf.get());
      break;
    case 0:
      insert_rect(s.screen_buffer.get(), screen_size, screen_width,
                  countdown_pos - 3, go_width, go_height, s.go_char_buf.get());
      break;
    }
  }

  // Draw road markings

  const auto offset =
      static_cast<size_t>(roundl(distance_m / cell_length_m)) + 3;
  for (size_t i = 0; i < visible_road_cells; ++i) {
    if ((i + offset) % 4 == 0 || (i + offset) % 4 == 1) {
      const auto buffer_row = visible_road_cells - i + 2;
      s.screen_buffer[46 + buffer_row * screen_width] = L' ';
      s.screen_buffer[72 + buffer_row * screen_width] = L' ';
    }
  }

  // Display any visible road items
  const auto road_item_list = road_items.list();
  for (const auto &road_item : road_item_list) {
    const auto road_item_distance_m = road_item.distance_m;
    const auto diff = road_item_distance_m - distance_m;
    if (diff < visible_road_length) {
      const auto cells_ahead =
          static_cast<size_t>(roundl(diff / cell_length_m));
      const auto buffer_row = visible_road_cells - cells_ahead + 2;

      auto item_width = road_item_width(road_item.type);
      auto item_height = road_item_height(road_item.type);
      const wchar_t *item_buffer;
      WORD item_color;
      switch (road_item.type) {
      case Obstacle:
        item_buffer = L"#########";
        item_color = FG_RED;
        break;
      case Boost:
        item_buffer = L"+250";
        item_color = FG_LIGHTCYAN;
        break;
      case Trophy:
        item_buffer = L"[T]";
        item_color = FG_YELLOW;
        break;
      }

      static constexpr size_t non_road_characters = screen_width * 3;
      const auto buffer_top_row = buffer_row - item_height + 1;
      const auto max_buffer_x = 100 - item_width;
      const auto buffer_x = min(road_item.pos_x + 20, max_buffer_x);
      const auto buffer_start_idx = buffer_top_row * screen_width + buffer_x;
      if (buffer_start_idx < screen_size) {
        // Modify char buffer
        insert_rect(s.screen_buffer.get() + non_road_characters,
                    screen_size - non_road_characters, screen_width,
                    buffer_start_idx - non_road_characters, item_width,
                    item_height, item_buffer);

        // Modify color buffer
        std::unique_ptr<WORD[]> color_rect{new WORD[item_width * item_height]};
        for (size_t i{0}; i < item_width * item_height; ++i) {
          color_rect[i] = item_color;
        }
        insert_rect(s.color_buffer.get() + non_road_characters,
                    screen_size - non_road_characters, screen_width,
                    buffer_start_idx - non_road_characters, item_width,
                    item_height, color_rect.get());
      }
    }
  }

  // Draw car
  insert_rect(s.screen_buffer.get(), screen_size, screen_width,
              3980 + pos_x_cells, car_width, car_height, s.car_char_buf.get());

  std::unique_ptr<WORD[]> color_rect{new WORD[car_width * car_height]};
  for (size_t i{0}; i < car_width * car_height; ++i) {
    color_rect[i] = FG_WHITE;
  }
  insert_rect(s.color_buffer.get(), screen_size, screen_width,
              3980 + pos_x_cells, car_width, car_height, color_rect.get());

  // Style keybind row
  for (size_t i{0}; i < screen_width; ++i) {
    s.color_buffer[i + screen_width * 1] = FG_LIGHTGRAY;
  }

  static constexpr size_t keybind_text_y{1};
  size_t keybind_text_x[]{3, 22, 36, 55, 75, 76, 77};
  for (size_t i{0}; i * sizeof(size_t) < sizeof(keybind_text_x); ++i) {
    s.color_buffer[keybind_text_y * screen_width + keybind_text_x[i]] =
        FG_LIGHTBLUE;
  }

  s.color_buffer[keybind_text_y * screen_width + 91] = FG_GREEN;
  s.color_buffer[keybind_text_y * screen_width + 92] = FG_GREEN;
  s.color_buffer[keybind_text_y * screen_width + 93] = FG_GREEN;

  // Display optimal path, if pathfinder is enabled
  if (path) {
    for (const auto cell_on_path : path.value()) {
      const auto buffer_row =
          max(3, screen_height - (car_height + cell_on_path / 80 + 1));
      const auto buffer_x = 20 + (cell_on_path % 80);
      s.color_buffer[buffer_row * screen_width + buffer_x] |= BG_GREEN;
    }
  }

  // Print stats
  const auto score_str = pad_left(
      std::to_wstring((static_cast<size_t>(roundl(score))) / 5 * 5), 7);
  const auto time_str =
      pad_left(round_to_1dp(std::to_wstring(time_ns / ns_in_s)) + L"s", 7);
  const auto distance_str =
      pad_left(round_to_1dp(std::to_wstring(distance_m / 1'000)) + L"km", 8);
  const auto speed_str = pad_left(
      std::to_wstring(static_cast<int>(roundl(3.6L * speed_ms))) + L"kph", 7);

  insert_wstr_in_buffer(s.screen_buffer.get() + 8, score_str);
  insert_wstr_in_buffer(s.screen_buffer.get() + 34, time_str);
  insert_wstr_in_buffer(s.screen_buffer.get() + 64, distance_str);
  insert_wstr_in_buffer(s.screen_buffer.get() + 92, speed_str);

  display_fps(s);

  s.rerender = true;
}