// ======================================================================
/*!
 * \brief Send favicon.ico response
 */
// ======================================================================

#include "Favicon.h"
#include <fstream>
#include <string>
#include <stdexcept>

extern "C" {
#include <time.h>
}

namespace SmartMet
{
namespace Server
{
namespace Favicon
{
using namespace SmartMet::Spine;
// ----------------------------------------------------------------------
/*!
 * \brief Format a time for HTTP output
 *
 * The output is generated with strftime using format
 * "%a,  %d  %b  %Y  %H:%M:%S" as adviced in the
 * man-pages for strftime.
 */
// ----------------------------------------------------------------------

std::string format_time(const ::time_t theTime)
{
  struct ::tm t;
  gmtime_r(&theTime, &t);
  const ::size_t MAXLEN = 100;
  char buffer[MAXLEN];
  ::size_t n = strftime(buffer, MAXLEN, "%a, %d %b %Y %H:%M:%S GMT", &t);
  std::string ret(buffer, 0, n);
  return ret;
}

// ----------------------------------------------------------------------
/*!
 * \brief Send WeatherProof favicon.h
 */
// ----------------------------------------------------------------------

void contenthandler(Reactor& /* theReactor */,
                    const HTTP::Request& /* theRequest */,
                    HTTP::Response& theResponse)
{
  static bool ok = false;
  static std::string& favicon = *new std::string(894, ' ');

  if (!ok)
  {
    // Mutex not absolutely necessary. It does not matter if by bad luck
    // two separate threads end up in here and read the same bytes to
    // the same string.

    std::ifstream in("/smartmet/www/img.weatherproof.fi/favicon.ico");
    if (!!in)
    {
      in.readsome(&favicon[0], boost::numeric_cast<long>(favicon.size()));
      ok = true;
    }
  }

  ::time_t expiration_time = time(0) + 7 * 24 * 3600;  // 7 days

  theResponse.setStatus(HTTP::Status::ok);
  theResponse.setHeader("Content-Type", "image/vnd.microsoft.icon");
  theResponse.setHeader("Expires", format_time(expiration_time));
  theResponse.setContent(favicon);
}

}  // namespace Favicon
}  // namespace Server
}  // namespace SmartMet
