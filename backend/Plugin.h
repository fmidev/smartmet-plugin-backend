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
  ~Plugin() override = default;

  const std::string& getPluginName() const override;
  int getRequiredAPIVersion() const override;
  bool queryIsFast(const HTTP::Request& theRequest) const override;

 protected:
  void init() override;
  void shutdown() override;
  void requestHandler(Reactor& theReactor,
                      const HTTP::Request& theRequest,
                      HTTP::Response& theResponse) override;

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
