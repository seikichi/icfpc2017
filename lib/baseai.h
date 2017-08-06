#pragma once

#include <boost/serialization/serialization.hpp>
#include "move.h"

class BaseAi {
  int punter_id;

  friend class boost::serialization::access;
  template <class Archive>
    void serialize(Archive& ar, unsigned int /*version*/)
    {
      ar & punter_id;
    }
protected:
  BaseAi(int id);
public:
  int PunterId() const { return punter_id; }
  virtual Move NextMove() = 0;
};
