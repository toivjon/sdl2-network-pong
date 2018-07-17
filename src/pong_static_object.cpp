#include "pong_static_object.h"

using namespace pong;

StaticObject::StaticObject() : StaticObject({ 0, 0, 0, 0})
{
  // ...
}

StaticObject::StaticObject(const SDL_Rect& rect) : StaticObject(rect, true)
{
  // ...
}

StaticObject::StaticObject(const SDL_Rect& rect, bool renderable) : mRect(rect), mRenderable(renderable)
{
  // ...
}

void StaticObject::render(SDL_Renderer& renderer)
{
  if (mRenderable) {
    SDL_SetRenderDrawColor(&renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderFillRect(&renderer, &mRect);
  }
}
