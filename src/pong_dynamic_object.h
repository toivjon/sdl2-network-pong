#ifndef PONG_DYNAMIC_OBJECT_H
#define PONG_DYNAMIC_OBJECT_H

#include <SDL/SDL.h>
#include <vector>

namespace pong
{
  class DynamicObject final
  {
    public:
      struct State {
        long     time;
        SDL_Rect rect;
      };

      DynamicObject();
      DynamicObject(const SDL_Rect& rect);

      void render(SDL_Renderer& renderer, long time);
    private:
      std::vector<State> mStates;
  };
}

#endif
