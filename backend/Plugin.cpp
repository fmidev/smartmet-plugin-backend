// ======================================================================
/*!
 * \brief SmartMet Backend plugin implementation
 */
// ======================================================================

#include "Plugin.h"
#include <boost/filesystem/path.hpp>
#include <macgyver/StringConversion.h>
#include <spine/Convenience.h>
#include <spine/Reactor.h>
#include <spine/SmartMet.h>
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
// ----------------------------------------------------------------------
/*!
 * \brief Format a time for HTTP output
 */
// ----------------------------------------------------------------------

std::string format_time(const ::time_t theTime)
{
  auto t = boost::posix_time::from_time_t(theTime);
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

// ----------------------------------------------------------------------
/*!
 * \brief Reply to / requests
 */
// ----------------------------------------------------------------------

// this is the content handler for URL /
void Plugin::baseContentHandler(SmartMet::Spine::Reactor &theReactor,
                                const SmartMet::Spine::HTTP::Request & /* theRequest */,
                                SmartMet::Spine::HTTP::Response &theResponse)
{
  try
  {
    theResponse.setStatus(SmartMet::Spine::HTTP::Status::ok);
    if (theReactor.isShutdownRequested())
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
 * \brief Reply to sleep requests
 */
// ----------------------------------------------------------------------

#ifndef NDEBUG
void sleep(Reactor & /* theReactor */,
           const SmartMet::Spine::HTTP::Request &theRequest,
           SmartMet::Spine::HTTP::Response &theResponse)
{
  try
  {
    auto t = SmartMet::Spine::optional_size(theRequest.getParameter("t"), 1);

    if (t > 0)
      ::sleep(t);

    theResponse.setStatus(SmartMet::Spine::HTTP::Status::ok);
    theResponse.setContent("SmartMet Server\n");
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}
#endif

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
      boost::filesystem::path p = itsConfig;
      p.remove_filename();
      config.setIncludeDir(p.c_str());

      config.readFile(itsConfig.c_str());

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
            this, "/", boost::bind(&Plugin::baseContentHandler, this, _1, _2, _3)))
      throw Fmi::Exception(BCP, "Failed to register base content handler");

#ifndef NDEBUG
    if (!itsReactor->addContentHandler(this, "/sleep", boost::bind(&sleep, _1, _2, _3)))
      throw Fmi::Exception(BCP, "Failed to register sleep content handler");
#endif

    // Add Favicon content handler
    if (!itsReactor->addContentHandler(
            this, "/favicon.ico", boost::bind(&Plugin::faviconHandler, this, _1, _2, _3)))
      throw Fmi::Exception(BCP, "Failed to register favicon.ico content handler");
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
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
    ::time_t expiration_time = time(nullptr) + 7 * 24 * 3600;  // 7 days
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
 * \brief requestHandler is a dummy function for this plugin
 */
// ----------------------------------------------------------------------

void Plugin::requestHandler(SmartMet::Spine::Reactor & /* theReactor */,
                            const SmartMet::Spine::HTTP::Request & /* theRequest */,
                            SmartMet::Spine::HTTP::Response & /* theResponse */)
{
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
