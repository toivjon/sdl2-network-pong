#include "pong_dynamic_object.h"

#include <cassert>
#include <iostream>

using namespace pong;

DynamicObject::DynamicObject()
{
  // ...
}

DynamicObject::DynamicObject(const SDL_Rect& rect)
{
  // append the given state as the initial state.
  mStates.push_back({0l, rect});
}

void DynamicObject::render(SDL_Renderer& renderer, long time)
{
  assert(mStates.empty() == false);
  SDL_SetRenderDrawColor(&renderer, 0xff, 0xff, 0xff, 0xff);
  State previousState = mStates[0];
  for (auto& state : mStates) {
    if (time >= previousState.time && time <= state.time) {
      // perform a linear interpolation of the state positions.
      auto t = (time == 0l ? 0l : static_cast<float>(previousState.time - time) / static_cast<float>(state.time - previousState.time));
      auto x = previousState.rect.x + static_cast<int>(t * (static_cast<float>(state.rect.x) - static_cast<float>(previousState.rect.x)));
      auto y = previousState.rect.y + static_cast<int>(t * (static_cast<float>(state.rect.y) - static_cast<float>(previousState.rect.y)));
      SDL_Rect r = {x, y, state.rect.w, state.rect.h};
      SDL_RenderFillRect(&renderer, &r);
      previousState = state;
      break;
    }
  }
}
