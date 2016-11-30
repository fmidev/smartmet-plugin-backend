// ----------------------------------------------------------------------
/*!
 * \brief favicon.ico handler
 */
// ----------------------------------------------------------------------

#pragma once
#include <spine/HTTP.h>
#include <spine/Reactor.h>

namespace SmartMet
{
namespace Server
{
namespace Favicon
{
void contenthandler(SmartMet::Spine::Reactor& theReactor,
                    const SmartMet::Spine::HTTP::Request& theRequest,
                    SmartMet::Spine::HTTP::Response& theResponse);
}  // namespace Favicon
}  // namespace Server
}  // namespace SmartMet
