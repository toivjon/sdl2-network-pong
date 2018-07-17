#ifndef PONG_APPLICATION_H
#define PONG_APPLICATION_H

#include "pong_dynamic_object.h"
#include "pong_static_object.h"

#include <type_traits>
#include <vector>

#define MAX_PACKAGE_SIZE 1024

// forward declarations.
struct SDL_Window;
struct SDL_Renderer;
struct _SDLNet_SocketSet;

namespace pong
{
  class Application
  {
    public:
      Application() = delete;
      Application(Application&) = delete;
      Application(Application&&) = delete;

      Application& operator=(Application&) = delete;
      Application& operator=(Application&&) = delete;

      Application(int maxsockets);

      virtual ~Application();

      void setRunning(bool running) { mRunning = running; }

      bool isRunning() const { return mRunning; }
    protected:
      void render();
      void tickSDL();
    protected:
      SDL_Window*         mWindow;
      SDL_Renderer*       mRenderer;
      _SDLNet_SocketSet*  mSocketSet;
      bool                mRunning;
      StaticObject        mTopWall;
      StaticObject        mBottomWall;
      StaticObject        mCenterLine;
      DynamicObject       mLeftPaddle;
      DynamicObject       mRightPaddle;
      DynamicObject       mBall;
  };
}

#endif
