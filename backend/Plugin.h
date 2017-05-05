// ======================================================================
/*!
 * \brief SmartMet Backend plugin interface
 */
// ======================================================================

#pragma once

#include <spine/SmartMetPlugin.h>
#include <spine/Reactor.h>

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
  virtual ~Plugin();

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

 private:
  const std::string itsModuleName;
  const std::string itsConfig;
  SmartMet::Spine::Reactor* itsReactor = nullptr;
  std::string itsFavicon;

};  // class Plugin

}  // namespace Backend
}  // namespace Plugin
}  // namespace SmartMet

// ======================================================================
