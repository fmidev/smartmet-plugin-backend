// ======================================================================
/*!
 * \brief SmartMet Backend plugin interface
 */
// ======================================================================

#pragma once

#include <engines/sputnik/Engine.h>
#include <spine/Reactor.h>
#include <spine/SmartMetPlugin.h>
#include <spine/Table.h>

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

  Plugin(const Plugin& other) = delete;
  Plugin& operator=(const Plugin& other) = delete;
  Plugin(Plugin&& other) = delete;
  Plugin& operator=(Plugin&& other) = delete;

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

 private:
  std::unique_ptr<Table> getBackendInfo(const HTTP::Request& theRequest);
  void requestClusterInfo(const HTTP::Request& theRequest, HTTP::Response& theResponse) const;
  std::string setContinue(const HTTP::Request& theRequest);
  std::string setPause(const HTTP::Request& theRequest);

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
