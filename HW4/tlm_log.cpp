#include <iomanip>
#include <iostream>
#include <sstream>
#include <tlm>

#include "tlm_log.h"

namespace tshsu {

std::string print(unsigned int u) {
  std::stringstream ss;
  ss << "0x" << std::setw(8) << std::setfill('0') << std::hex << u << std::dec;
  return ss.str();
}

std::string print(const sc_dt::uint64 u64) {
  std::stringstream ss;
  ss << "0x" << std::setw(16) << std::setfill('0') << std::hex << u64
     << std::dec;
  return ss.str();
}

std::string print(const tlm::tlm_command command) {
  if (command == tlm::TLM_READ_COMMAND)
    return "R";
  if (command == tlm::TLM_WRITE_COMMAND)
    return "W";
  if (command == tlm::TLM_IGNORE_COMMAND)
    return "IGNORE";

  std::stringstream ss;
  ss << "Other tlm_command: " << command;
  return ss.str();
}
std::string print(const tlm::tlm_sync_enum sync_enum) {
  if (sync_enum == tlm::TLM_ACCEPTED)
    return "TLM_ACCEPTED";
  if (sync_enum == tlm::TLM_UPDATED)
    return "TLM_UPDATED";
  if (sync_enum == tlm::TLM_COMPLETED)
    return "TLM_COMPLETED";

  std::stringstream ss;
  ss << "Other tlm_sync_enum: " << sync_enum;
  return ss.str();
}

std::string print(const sc_core::sc_time &t, bool unit) {
  std::stringstream ss;
  ss << std::setprecision(0) << std::fixed << UNIFY(t);
  if (unit) {
    ss << " " << TIME_UNIT_STRING;
  }
  return ss.str();
}

} // namespace tshsu
