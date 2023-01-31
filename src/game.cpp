
#include <iostream>
#include <algorithm>
#include <functional>
#include "SDL.h"
#include "game.h"
#include "controller.h"



Game::Game(std::size_t grid_width, std::size_t grid_height, int _nPlayers)
    : engine(dev()),
      nPlayers(_nPlayers),
      random_w(0, static_cast<int>(grid_width - 1)),
      random_h(0, static_cast<int>(grid_height - 1)) {
  
  for(int i = 0; i < nPlayers; i++){
    _snakes.push_back(std::make_shared<Snake>(grid_width, grid_height, i));
    score.push_back(0);
    snake_sizes.push_back(0);
  }
  PlaceFood();
  PlaceObstacle();
}

SDL_Keycode Game::GetKeypress(int ind){

  return result.at(ind);

}



void Game::Run(Renderer &renderer, std::size_t target_frame_duration) {
  Uint32 title_timestamp = SDL_GetTicks();
  Uint32 frame_start;
  Uint32 frame_end;
  Uint32 frame_duration;
  int frame_count = 0;
  bool running{true};

  for(int i = 0; i < nPlayers; i++){
    if(i == 0){
      _controllers.push_back(std::make_unique<Controller>(SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, _snakes.at(i), i));
      _controllers.at(i)->setGameHandle(this);
    }
    else{
      _controllers.push_back(std::make_unique<Controller>(SDLK_w, SDLK_s, SDLK_a, SDLK_d, _snakes.at(i), i));
      _controllers.at(i)->setGameHandle(this);      
    }
  }

  std::for_each(_snakes.begin(), _snakes.end(), [](std::shared_ptr<Snake> &itr){itr->launch();});
  std::for_each(_controllers.begin(), _controllers.end(), [](std::unique_ptr<Controller> &ctr){ctr->launch();});
  
  
  while (running) {

    SDL_Keycode key_code;
    SDL_Event e;
    while(SDL_PollEvent(&e)){
      if(e.type == SDL_QUIT){
        running = false;
      }
      else if(e.type == SDL_KEYDOWN){
        key_code = e.key.keysym.sym;

        if(any_of(key_codes_controller1.begin(), key_codes_controller1.end(), [key_code](SDL_Keycode itr){return itr == key_code;}))
          result.at(0) = key_code;

        else if(any_of(key_codes_controller2.begin(), key_codes_controller2.end(), [key_code](SDL_Keycode itr){return itr == key_code;}))
          result.at(1) = key_code;
        
        }
      }
  


    frame_start = SDL_GetTicks();

    // Input, Update, Render - the main game loop.

    Update();
    renderer.Render(_snakes, _food, _obstacles);

    frame_end = SDL_GetTicks();

    // Keep track of how long each loop through the input/update/render cycle
    // takes.
    frame_count++;
    frame_duration = frame_end - frame_start;

    // After every second, update the window title.
    if (frame_end - title_timestamp >= 1000) {
      renderer.UpdateWindowTitle(score, frame_count);
      frame_count = 0;
      title_timestamp = frame_end;
    }

    // If the time for this frame is too small (i.e. frame_duration is
    // smaller than the target ms_per_frame), delay the loop to
    // achieve the correct frame rate.
    if (frame_duration < target_frame_duration) {
      SDL_Delay(target_frame_duration - frame_duration);
    }
  
    

  }
  std::for_each(_snakes.begin(), _snakes.end(), [](std::shared_ptr<Snake> &itr){itr->alive = false;});
  std::for_each(_controllers.begin(), _controllers.end(), [](std::unique_ptr<Controller> &ctr){ctr->control_running = false;});
}

void Game::PlaceFood() {
  if(_food.empty()){
    for(int i = 0; i < nFood; i++){
      int x, y;
      SDL_Point fd;
      while(true){
        x = random_w(engine);
        y = random_h(engine);
        if(std::any_of(_snakes.begin(), _snakes.end(), [x,y](std::shared_ptr<Snake> &itr){return !itr->SnakeCell(x,y);})){
          fd.x = x;
          fd.y = y;
          _food.push_back(fd);
          break;
        }
      }
    }
  }
  else{
    int x, y;
    SDL_Point fd;
    while(true){
      x = random_w(engine);
      y = random_h(engine);
      if(std::any_of(_obstacles.begin(), _obstacles.end(), [x, y](std::shared_ptr<Obstacle> &itr){return !(itr->GetObstacleXCoord() == x && itr->GetObstacleYCoord() == y);}) && std::any_of(_snakes.begin(), _snakes.end(), [x,y](std::shared_ptr<Snake> &itr) {return !itr->SnakeCell(x,y);})){
        fd.x = x;
        fd.y = y;
        _food.push_back(fd);
        break;
      }
    }
  }
}



void Game::PlaceObstacle() {
  for(int i = 0; i < nObstacles; i++){
    Obstacle obstacle;
    int x, y;
    while(true){
      x = random_w(engine);
      y = random_h(engine);
      if(std::any_of(_food.cbegin(), _food.cend(), [x,y](SDL_Point itr){return !(itr.x == x && itr.y == y);}) && std::any_of(_snakes.begin(), _snakes.end(), [x,y](std::shared_ptr<Snake> &itr){return !itr->SnakeCell(x,y);})){
        obstacle.SetObstacleCoords(x,y);
        _obstacles.push_back(std::make_shared<Obstacle>(obstacle));
        break;
      }
    }
  }
  for(auto itr : _snakes){
    itr->GetObstacles(_obstacles);
    }  
}


void Game::Update() {



  
  for(int i = 0; i < nPlayers; i++){
    int x = static_cast<int>(_snakes.at(i)->head_x);
    int y = static_cast<int>(_snakes.at(i)->head_y);

    if(nPlayers > 1){
      if(i < 1){
        if(std::any_of(_snakes.at(i+1)->body.begin(), _snakes.at(i+1)->body.end(),[x, y](SDL_Point itr){return (itr.x == x && itr.y == y);})){
          _snakes.at(i)->alive = false;
        }
      }
      else{
        if(std::any_of(_snakes.at(i-1)->body.begin(), _snakes.at(i-1)->body.end(),[x, y](SDL_Point itr){return (itr.x == x && itr.y == y);})){
          _snakes.at(i)->alive = false;
        } 
      }
    }
    
    int t = 0;
    for(auto fdr : _food){
      if(fdr.x == x && fdr.y == y){
        _food.erase(_food.begin() + t);
        score[i]++;
        _snakes.at(i)->GrowBody();
        _snakes.at(i)->speed += 0.02;
        snake_sizes.at(i) = _snakes.at(i)->size;
        PlaceFood();
        break;
      }
      t++;
    }
  }
}



Game::~Game(){


  if(snake_sizes.size() > 1){
  std::cout << "Score for Player 1: " << score.at(0) << "  " << "Score for Player 2: " << score.at(1) << std::endl;
  std::cout << "Size of Snake 1: " << snake_sizes.at(0) << "  " << "Size of Snake 2: " << snake_sizes.at(1) << std::endl;
  }
  else{
  std::cout << "Score for Player 1: " << score.at(0) << std::endl;
  std::cout << "Size of Snake 1: " << snake_sizes.at(0) <<  std::endl;
  }

  std::cout << "Thank you for playing!" << std::endl;


}

