#include "settings.hpp"

#include "logger.hpp"
namespace ui_protei {

void printAppSettings(AppSettings& settings)
{
  logging::SingleThreadPresets::functionCall();

  std::cout << "========================\n";
  auto& exchanger = settings.getExchanger();

  std::cout << "Current settings:\n";

  std::cout << "IP address:\n";
  std::cout << '\t' << exchanger.getAddr() << '\n';
  auto ctxt = exchanger.getContext();
  std::cout << "Context:\n";
  std::cout << "\tIMSI:" << ctxt.imsi() << '\n';
  std::cout << "\tMSISDN:" << ctxt.msisdn() << '\n';
  std::cout << "\tIMEI:" << ctxt.imei() << '\n';
  std::cout << "\tTMSI:" << ctxt.tmsi() << '\n';
  std::cout << "\tTTL UE:" << ctxt.ttlUe() << '\n';

  //remove later
  std::cout << "Socket:\t\t" << exchanger.socket() << '\n';
  std::cout << "========================\n";
}

}  // namespace ui_protei
