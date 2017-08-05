#include "baseai.h"

BaseAi::BaseAi(int id){
  this->punter_id = id;
  this->map = Map::Map();
}

BaseAi::BaseAi(int id, class Map map) {
  this->punter_id = id;
  this->map = map;
}
