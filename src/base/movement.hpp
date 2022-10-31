#pragma once
#include <SDL.h>

#include <array>
#include <tuple>

namespace base {
/// @brief Simple mapping of movement keys to directions.
struct MoveKey {
  int scancode;
  int dx;
  int dy;
};

// clang-format off
/// @brief A list of common movement keys.
static constexpr std::array MOVE_KEYS {
    MoveKey{SDL_SCANCODE_A, -1, 0},
    MoveKey{SDL_SCANCODE_D, 1, 0},
    MoveKey{SDL_SCANCODE_W, 0, -1},
    MoveKey{SDL_SCANCODE_S, 0, 1},
    MoveKey{SDL_SCANCODE_LEFT, -1, 0},
    MoveKey{SDL_SCANCODE_RIGHT, 1, 0},
    MoveKey{SDL_SCANCODE_UP, 0, -1},
    MoveKey{SDL_SCANCODE_DOWN, 0, 1},
    MoveKey{SDL_SCANCODE_KP_4, -1, 0},
    MoveKey{SDL_SCANCODE_KP_6, 1, 0},
    MoveKey{SDL_SCANCODE_KP_8, 0, -1},
    MoveKey{SDL_SCANCODE_KP_2, 0, 1},

    MoveKey{SDL_SCANCODE_KP_7, -1, -1},
    MoveKey{SDL_SCANCODE_KP_1, -1, 1},
    MoveKey{SDL_SCANCODE_KP_9, 1, -1},
    MoveKey{SDL_SCANCODE_KP_3, 1, 1},

    MoveKey{SDL_SCANCODE_HOME, -1, -1},
    MoveKey{SDL_SCANCODE_END, -1, 1},
    MoveKey{SDL_SCANCODE_PAGEUP, 1, -1},
    MoveKey{SDL_SCANCODE_PAGEDOWN, 1, 1},

    MoveKey{SDL_SCANCODE_H, -1, 0},
    MoveKey{SDL_SCANCODE_J, 0, 1},
    MoveKey{SDL_SCANCODE_K, 0, -1},
    MoveKey{SDL_SCANCODE_L, 1, 0},
    MoveKey{SDL_SCANCODE_Y, -1, -1},
    MoveKey{SDL_SCANCODE_U, 1, -1},
    MoveKey{SDL_SCANCODE_B, -1, 1},
    MoveKey{SDL_SCANCODE_N, 1, 1},
};
// clang-format on

/// @brief Return the current direction from the active keyboard state.
/// @return A {dx, dy} tuple of all held keys.
inline auto get_current_movement_dir() -> std::tuple<int, int> {
  const auto* keyboard_state = SDL_GetKeyboardState(NULL);
  int dx = 0;
  int dy = 0;
  for (const auto& move_key : MOVE_KEYS) {
    if (keyboard_state[move_key.scancode]) {
      dx += move_key.dx;
      dy += move_key.dy;
    }
  }
  return {dx, dy};
}
}  // namespace base
