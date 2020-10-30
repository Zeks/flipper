#pragma once
#include <QString>
namespace core {
struct SiteId{
    QString website;
    int identity;
};

struct SiteIdPack{
  int ffn = -1;
  int ao3 = -1;
  int sb = -1;
  int sv = -1;
  int GetPrimaryId()const {
    if(ffn != -1)
        return ffn;
    if(ao3 != -1)
        return ao3;
    if(sb!= -1)
        return sb;
    if(sv != -1)
        return sv;
    return -1;
  }
  SiteId GetPrimaryIdentity()
  {
      if(ffn != -1)
          return {"ffn", ffn};
      if(ao3 != -1)
          return {"ao3", ffn};
      if(sb!= -1)
          return {"sb", sb};
      if(sv != -1)
          return {"sv", sv};
      return {"", -1};
    }
};

struct Identity{
    int id = -1;
    QString uuid;
    SiteIdPack web;
};
}
