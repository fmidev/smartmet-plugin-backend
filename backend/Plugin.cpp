// ======================================================================
/*!
 * \brief SmartMet Backend plugin implementation
 */
// ======================================================================

#include "Plugin.h"
#include <macgyver/StringConversion.h>
#include <spine/ConfigTools.h>
#include <spine/Convenience.h>
#include <spine/Reactor.h>
#include <spine/SmartMet.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include <libconfig.h++>
#include <sstream>
#include <stdexcept>

namespace SmartMet
{
namespace Plugin
{
namespace Backend
{
namespace  // anonymous
{

// ----------------------------------------------------------------------
/*!
 * \brief Format a time for HTTP output
 */
// ----------------------------------------------------------------------

std::string format_time(const ::time_t theTime)
{
  auto t = Fmi::date_time::from_time_t(theTime);
  return Fmi::to_http_string(t);
}

// ----------------------------------------------------------------------
/*!
 * \brief Read a file into a string
 */
// ----------------------------------------------------------------------

std::string read_file(const std::string &filename)
{
  std::string content;
  std::ifstream in(filename.c_str());
  if (!in)
    throw Fmi::Exception(BCP, "Failed to open '" + filename + "' for reading!");

  content.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  return content;
}
}  // namespace

// ----------------------------------------------------------------------
/*!
 * \brief Reply to / requests
 */
// ----------------------------------------------------------------------

// this is the content handler for URL /
void Plugin::requestHandler(SmartMet::Spine::Reactor &theReactor,
                            const SmartMet::Spine::HTTP::Request & /* theRequest */,
                            SmartMet::Spine::HTTP::Response &theResponse)
{
  try
  {
    theResponse.setStatus(SmartMet::Spine::HTTP::Status::ok);
    if (Reactor::isShuttingDown())
      theResponse.setContent("SmartMet Server shutting down");
    else if (itsSputnik != nullptr && itsSputnik->isPaused())
      theResponse.setContent("SmartMet Server paused");
    else if (theReactor.isInitializing())
      theResponse.setContent("SmartMet Server initializing");
    else
      theResponse.setContent("SmartMet Server");
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Plugin constructor
 */
// ----------------------------------------------------------------------

Plugin::Plugin(SmartMet::Spine::Reactor *theReactor, const char *theConfig)
    : itsModuleName("Backend"), itsConfig(theConfig)
{
  if (theReactor->getRequiredAPIVersion() != SMARTMET_API_VERSION)
    throw Fmi::Exception(BCP, "Backend and Server API version mismatch");

  itsReactor = theReactor;
}

// ----------------------------------------------------------------------
/*!
 * \brief Initialize the plugin
 */
// ----------------------------------------------------------------------
void Plugin::init()
{
  try
  {
    // We allow the configuration file to be unset,
    // favicon.ico will then return status code204

    if (!itsConfig.empty())
    {
      // Read the configuration file
      libconfig::Config config;

      // Enable sensible relative include paths
      std::filesystem::path p = itsConfig;
      p.remove_filename();
      config.setIncludeDir(p.c_str());

      config.readFile(itsConfig.c_str());
      Spine::expandVariables(config);

      // Favicon.ico location, if any
      std::string favicon;
      config.lookupValue("favicon", favicon);
      if (!favicon.empty())
      {
        itsFavicon = read_file(favicon);
      }
    }

    // Launch a new instance of Sputnik on network ItsNetworkAddress
    itsSputnik = reinterpret_cast<SmartMet::Engine::Sputnik::Engine *>(
        itsReactor->getSingleton("Sputnik", nullptr));

    // Start Sputnik engine in backend mode
    itsSputnik->launch(SmartMet::Engine::Sputnik::Backend, itsReactor);

    if (!itsReactor->addContentHandler(
            this,
            "/",
            [this](Spine::Reactor &theReactor,
                   const Spine::HTTP::Request &theRequest,
                   Spine::HTTP::Response &theResponse)
            { callRequestHandler(theReactor, theRequest, theResponse); }))
      throw Fmi::Exception(BCP, "Failed to register base content handler");

    // Add Favicon content handler
    if (!itsReactor->addContentHandler(this,
                                       "/favicon.ico",
                                       [this](Spine::Reactor &theReactor,
                                              const Spine::HTTP::Request &theRequest,
                                              Spine::HTTP::Response &theResponse)
                                       { faviconHandler(theReactor, theRequest, theResponse); }))
      throw Fmi::Exception(BCP, "Failed to register favicon.ico content handler");

    using AdminRequestAccess = SmartMet::Spine::Reactor::AdminRequestAccess;

    // Add cluster info admin handler
    if (!itsReactor->addAdminCustomRequestHandler(
            this,
            "clusterinfo",
            AdminRequestAccess::Public,
            [this](auto &&a, auto &&b, auto &&c)
            {
              requestClusterInfo(std::forward<decltype(a)>(a),
                                 std::forward<decltype(b)>(b),
                                 std::forward<decltype(c)>(c));
            },
            "Request cluster info"))
    {
      throw Fmi::Exception(BCP, "Failed to register clusterinfo admin request handler");
    }

    // Add continue admin handler
    if (!itsReactor->addAdminStringRequestHandler(
            this,
            "continue",
            AdminRequestAccess::RequiresAuthentication,
            [this](auto &&, auto &&b) { return setContinue(std::forward<decltype(b)>(b)); },
            "Continue Sputnik"))
    {
      throw Fmi::Exception(BCP, "Failed to register continue admin request handler");
    }

    // Add pause admin handler
    if (!itsReactor->addAdminStringRequestHandler(
            this,
            "pause",
            AdminRequestAccess::RequiresAuthentication,
            [this](auto &&, auto &&b) { return setPause(std::forward<decltype(b)>(b)); },
            "Pause Sputnik"))
    {
      throw Fmi::Exception(BCP, "Failed to register pause admin request handler");
    }

    // Add backend info admin handler
    if (!itsReactor->addAdminTableRequestHandler(
            this,
            "backends",
            AdminRequestAccess::Public,
            [this](auto &&, auto &&b) { return getBackendInfo(std::forward<decltype(b)>(b)); },
            "Get backend info"))
    {
      throw Fmi::Exception(BCP, "Failed to register backends admin request handler");
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Get backend info
 */
// ----------------------------------------------------------------------

std::unique_ptr<Table> Plugin::getBackendInfo(const HTTP::Request &theRequest)
try
{
  std::string service = Spine::optional_string(theRequest.getParameter("service"), "");
  std::unique_ptr<Table> result = itsSputnik->backends(service);
  return result;
}
catch (...)
{
  throw Fmi::Exception::Trace(BCP, "Operation failed!");
}

// ----------------------------------------------------------------------
/*!
 * \brief Request cluster info
 */
// ----------------------------------------------------------------------

void Plugin::requestClusterInfo(Spine::Reactor &theReactor,
                                const HTTP::Request &theRequest,
                                HTTP::Response &theResponse) const
try
{
  const std::optional<std::string> adminUri = theReactor.getAdminUri();
  const bool full = adminUri && theRequest.getResource() == *adminUri;

  std::ostringstream out;
  itsSputnik->status(out, full);

  // Make MIME header
  std::string mime("text/html; charset=UTF-8");
  theResponse.setHeader("Content-Type", mime);

  // Set content
  std::string ret = "<html><head><title>SmartMet Admin</title></head><body>";
  ret += out.str();
  ret += "</body></html>";
  theResponse.setContent(ret);
  theResponse.setStatus(HTTP::Status::ok);
}
catch (...)
{
  throw Fmi::Exception::Trace(BCP, "Operation failed!");
}

// ----------------------------------------------------------------------
/*!
 * \brief Set continue
 */
// ----------------------------------------------------------------------

std::string Plugin::setContinue(const HTTP::Request &theRequest)
try
{
  // Optional deadline or duration:

  auto time_opt = theRequest.getParameter("time");
  auto duration_opt = theRequest.getParameter("duration");

  if (time_opt)
  {
    auto deadline = Fmi::TimeParser::parse(*time_opt);
    itsSputnik->setPauseUntil(deadline);
    return "Paused Sputnik until " + Fmi::to_iso_string(deadline);
  }

  if (duration_opt)
  {
    auto duration = Fmi::TimeParser::parse_duration(*duration_opt);
    auto deadline = Fmi::SecondClock::universal_time() + duration;
    itsSputnik->setPauseUntil(deadline);
    return "Paused Sputnik until " + Fmi::to_iso_string(deadline);
  }

  itsSputnik->setContinue();
  return "Sputnik continue request made";
}
catch (...)
{
  throw Fmi::Exception::Trace(BCP, "Operation failed!");
}

// ----------------------------------------------------------------------
/*!
 * \brief Set pause
 */
// ----------------------------------------------------------------------

std::string Plugin::setPause(const HTTP::Request &theRequest)
try
{
  // Optional deadline or duration:

  auto time_opt = theRequest.getParameter("time");
  auto duration_opt = theRequest.getParameter("duration");

  if (time_opt)
  {
    auto deadline = Fmi::TimeParser::parse(*time_opt);
    itsSputnik->setPauseUntil(deadline);
    return "Paused Sputnik until " + Fmi::to_iso_string(deadline);
  }

  if (duration_opt)
  {
    auto duration = Fmi::TimeParser::parse_duration(*duration_opt);
    auto deadline = Fmi::SecondClock::universal_time() + duration;
    itsSputnik->setPauseUntil(deadline);
    return "Paused Sputnik until " + Fmi::to_iso_string(deadline);
  }

  itsSputnik->setPause();
  return "Paused Sputnik until a continue request arrives";
}
catch (...)
{
  throw Fmi::Exception::Trace(BCP, "Operation failed!");
}

// ----------------------------------------------------------------------
/*!
 * \brief Send favicon.ico
 */
// ----------------------------------------------------------------------

void Plugin::faviconHandler(Reactor & /* theReactor */,
                            const HTTP::Request & /* theRequest */,
                            HTTP::Response &theResponse)
{
  if (itsFavicon.empty())
  {
    theResponse.setStatus(HTTP::Status::no_content);
  }
  else
  {
    ::time_t expiration_time = time(nullptr) + 7UL * 24UL * 3600UL;  // 7 days
    theResponse.setStatus(HTTP::Status::ok);
    theResponse.setHeader("Content-Type", "image/vnd.microsoft.icon");
    theResponse.setHeader("Expires", format_time(expiration_time));
    theResponse.setContent(itsFavicon);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Shutdown the plugin
 */
// ----------------------------------------------------------------------

void Plugin::shutdown()
{
  std::cout << "  -- Shutdown requested (BackendPlugin)\n";
}

// ----------------------------------------------------------------------
/*!
 * \brief Return the plugin name
 */
// ----------------------------------------------------------------------

const std::string &Plugin::getPluginName() const
{
  return itsModuleName;
}

// ----------------------------------------------------------------------
/*!
 * \brief Return the required version
 */
// ----------------------------------------------------------------------

int Plugin::getRequiredAPIVersion() const
{
  return SMARTMET_API_VERSION;
}

// ----------------------------------------------------------------------
/*!
 * \brief Backend must respond immediately to show it is alive
 */
// ----------------------------------------------------------------------

bool Plugin::queryIsFast(const SmartMet::Spine::HTTP::Request & /* theRequest */) const
{
  return true;
}

}  // namespace Backend
}  // namespace Plugin
}  // namespace SmartMet

/*
 * Server knows us through the 'SmartMetPlugin' virtual interface, which
 * the 'Plugin' class implements.
 */

extern "C" SmartMetPlugin *create(SmartMet::Spine::Reactor *them, const char *config)
{
  return new SmartMet::Plugin::Backend::Plugin(them, config);
}

extern "C" void destroy(SmartMetPlugin *us)
{
  // This will call 'Plugin::~Plugin()' since the destructor is virtual
  delete us;  // NOLINT(cppcoreguidelines-owning-memory)
}

// ======================================================================
