#ifndef PONG_STATIC_OBJECT_H
#define PONG_STATIC_OBJECT_H

#include <SDL/SDL.h>

namespace pong
{
  class StaticObject final
  {
    public:
      StaticObject();
      StaticObject(const SDL_Rect& rect);
      StaticObject(const SDL_Rect& rect, bool renderable);

      StaticObject(StaticObject& o);

      void render(SDL_Renderer& renderer);
    private:
      SDL_Rect  mRect;
      bool      mRenderable;
  };
}

#endif
