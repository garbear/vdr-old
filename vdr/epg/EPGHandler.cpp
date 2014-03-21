#include "EPGHandler.h"
#include "EPGHandlers.h"

namespace VDR
{

cEpgHandler::cEpgHandler(void)
{
  cEpgHandlers::Get().Add(this);
}

cEpgHandler::~cEpgHandler()
{
  cEpgHandlers::Get().Del(this, false);
}

}
