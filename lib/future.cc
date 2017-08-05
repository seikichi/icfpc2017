#include "future.h"

string Future::SerializeString(bool prettify) const {
  picojson::object o = SerializeJson();
  return picojson::value(o).serialize(prettify);
}
picojson::object Future::SerializeJson() const {
  picojson::object l_future;
  l_future["source"] = picojson::value((double)source.original_id);
  l_future["target"] = picojson::value((double)target.original_id);
  return l_future;
}
