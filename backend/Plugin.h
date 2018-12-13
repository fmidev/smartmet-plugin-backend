// ======================================================================
/*!
 * \brief SmartMet Backend plugin interface
 */
// ======================================================================

#pragma once

#include <engines/sputnik/Engine.h>
#include <spine/Reactor.h>
#include <spine/SmartMetPlugin.h>

namespace SmartMet
{
namespace Plugin
{
namespace Backend
{
using namespace SmartMet::Spine;
class Plugin : public SmartMetPlugin
{
 public:
  Plugin(Reactor* theReactor, const char* theConfig);
  virtual ~Plugin() = default;

  const std::string& getPluginName() const;
  int getRequiredAPIVersion() const;
  bool queryIsFast(const HTTP::Request& theRequest) const;

 protected:
  void init();
  void shutdown();
  void requestHandler(Reactor& theReactor,
                      const HTTP::Request& theRequest,
                      HTTP::Response& theResponse);

  void faviconHandler(Reactor& theReactor,
                      const HTTP::Request& theRequest,
                      HTTP::Response& theResponse);

  void baseContentHandler(SmartMet::Spine::Reactor& /* theReactor */,
                          const SmartMet::Spine::HTTP::Request& /* theRequest */,
                          SmartMet::Spine::HTTP::Response& theResponse);

 private:
  const std::string itsModuleName;
  const std::string itsConfig;
  SmartMet::Spine::Reactor* itsReactor = nullptr;
  SmartMet::Engine::Sputnik::Engine* itsSputnik = nullptr;
  std::string itsFavicon;

};  // class Plugin

}  // namespace Backend
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
