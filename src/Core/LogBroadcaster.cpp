#include "LogBroadcaster.hpp"

namespace Acheron {
namespace Core {

LogBroadcaster &LogBroadcaster::instance()
{
    static LogBroadcaster broadcaster;
    return broadcaster;
}

} // namespace Core
} // namespace Acheron
