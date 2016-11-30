// ======================================================================
/*!
 * \brief SmartMet Backend plugin implementation
 */
// ======================================================================

#include "Plugin.h"
#include "Favicon.h"
#include <spine/SmartMet.h>
#include <spine/Convenience.h>
#include <spine/Reactor.h>

#include <iostream>
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
 * \brief Reply to / requests
 */
// ----------------------------------------------------------------------

// this is the content handler for URL /
void baseContentHandler(SmartMet::Spine::Reactor & /* theReactor */,
                        const SmartMet::Spine::HTTP::Request & /* theRequest */,
                        SmartMet::Spine::HTTP::Response &theResponse)
{
  try
  {
    theResponse.setStatus(SmartMet::Spine::HTTP::Status::ok);
    theResponse.setContent("SmartMet Server\n");
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
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
    unsigned long t = SmartMet::Spine::optional_unsigned_long(theRequest.getParameter("t"), 1);

    if (t > 0)
      ::sleep(t);

    theResponse.setStatus(SmartMet::Spine::HTTP::Status::ok);
    theResponse.setContent("SmartMet Server\n");
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}
#endif

// ----------------------------------------------------------------------
/*!
 * \brief Plugin constructor
 */
// ----------------------------------------------------------------------

Plugin::Plugin(SmartMet::Spine::Reactor *theReactor, const char * /* theConfig */)
    : SmartMetPlugin(), itsModuleName("Backend")
{
  try
  {
    if (theReactor->getRequiredAPIVersion() != SMARTMET_API_VERSION)
      throw SmartMet::Spine::Exception(BCP, "Backend and Server API version mismatch");

    // Launch a new instance of BSputnik on network ItsNetworkAddress
    SmartMet::Engine::Sputnik::Engine *sputnik =
        reinterpret_cast<SmartMet::Engine::Sputnik::Engine *>(
            theReactor->getSingleton("Sputnik", (void *)NULL));

    // Start Sputnik engine in backend mode
    sputnik->launch(SmartMet::Engine::Sputnik::Backend, theReactor);

    if (!theReactor->addContentHandler(this, "/", boost::bind(&baseContentHandler, _1, _2, _3)))
      throw SmartMet::Spine::Exception(BCP, "Failed to register base content handler");

#ifndef NDEBUG
    if (!theReactor->addContentHandler(this, "/sleep", boost::bind(&sleep, _1, _2, _3)))
      throw SmartMet::Spine::Exception(BCP, "Failed to register sleep content handler");
#endif

    // Add Favicon content handler
    if (!theReactor->addContentHandler(
            this, "/favicon.ico", boost::bind(&Server::Favicon::contenthandler, _1, _2, _3)))
      throw SmartMet::Spine::Exception(BCP, "Failed to register favicon.ico content handler");
  }
  catch (...)
  {
    throw SmartMet::Spine::Exception(BCP, "Operation failed!", NULL);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Initialize the plugin (trivial in this case)
 */
// ----------------------------------------------------------------------
void Plugin::init()
{
}
// ----------------------------------------------------------------------
/*!
 * \brief Destructor
 */
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
/*!
 * \brief Shutdown the plugin
 */
// ----------------------------------------------------------------------

void Plugin::shutdown()
{
  std::cout << "  -- Shutdown requested (BackendPlugin)\n";
}

Plugin::~Plugin()
{
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
  delete us;
}

// ======================================================================
