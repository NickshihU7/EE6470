#ifndef TLM_LOG_H_
#define TLM_LOG_H_

#include <string>

const sc_core::sc_time TIME_UNIT = sc_core::sc_time(1, sc_core::SC_PS);
static const char *const TIME_UNIT_STRING = "PS";
inline double UNIFY(sc_core::sc_time t) { return (t / TIME_UNIT); }

namespace tlm {
class tlm_generic_payload;
}

namespace tshsu {
std::string print(const unsigned int u);
std::string print(const sc_dt::uint64 u64);
std::string print(const tlm::tlm_command command);
std::string print(const tlm::tlm_sync_enum sync_enum);
std::string print(const sc_core::sc_time &t, bool unit = true);
} // namespace tshsu

#endif
