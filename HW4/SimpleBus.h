#ifndef SIMPLEBUSLT_H__
#define SIMPLEBUSLT_H__

#include <tlm>

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include "MemoryMap.h"
#include "tlm_log.h"
#define DMI_DEBUG 0

template <int NR_OF_TARGET_SOCKETS, int NR_OF_INITIATOR_SOCKETS>
class SimpleBus : public sc_core::sc_module, public MemoryMap {
public:
  using transaction_type = tlm::tlm_generic_payload;
  using phase_type = tlm::tlm_phase;
  using sync_enum_type = tlm::tlm_sync_enum;
  using target_socket_type = tlm_utils::simple_target_socket_tagged<SimpleBus>;
  using initiator_socket_type = tlm_utils::simple_initiator_socket_tagged<SimpleBus>;
  // 2013-10-13 14:37:39, tshsu: NR_OF_INITIATOR_SOCKETS means #. of "target"
  // module == #. of "initiator" sockets of bus.
  target_socket_type t_skt[NR_OF_TARGET_SOCKETS];
  initiator_socket_type i_skt[NR_OF_INITIATOR_SOCKETS];

public:
  SC_HAS_PROCESS(SimpleBus);
  SimpleBus(sc_core::sc_module_name name, double clock_period_in_ps = 1000,
            bool trace = false, bool masked = true)
      : sc_core::sc_module(name), MemoryMap(name, NR_OF_INITIATOR_SOCKETS),
        clock_period(clock_period_in_ps, sc_core::SC_PS), m_trace(trace),
        m_is_address_masked(masked) {
    for (unsigned int i = 0; i < NR_OF_TARGET_SOCKETS; ++i) {
      t_skt[i].register_b_transport(this, &SimpleBus::initiatorBTransport, i);
      t_skt[i].register_transport_dbg(this, &SimpleBus::transportDebug, i);
      t_skt[i].register_get_direct_mem_ptr(this, &SimpleBus::getDMIPointer, i);
    }
    for (unsigned int i = 0; i < NR_OF_INITIATOR_SOCKETS; ++i) {
      i_skt[i].register_invalidate_direct_mem_ptr(
          this, &SimpleBus::invalidateDMIPointers, i);
    }
  }

  void set_clock_period(sc_core::sc_time t) { clock_period = t; }

  sc_core::sc_time get_clock_period() { return clock_period; }

  sc_core::sc_time delay(transaction_type &trans) {
    // Note that 4 means bus width is 4 bytes; not good enough coding.
    return (1 + trans.get_data_length() / 4) * clock_period;
  }

  void initiatorBTransport(int SocketId, transaction_type &trans,
                           sc_core::sc_time &t) {
    Addr orig = trans.get_address();
    Addr offset;
    int portId = getPortId(orig, offset);

    if (portId < 0) {
      std::cout << "ERROR: " << name() << ": initiatorBTransport()"
                << ": Invalid (undefine memory mapped) address == "
                << tshsu::print(trans.get_address()) << std::endl;
      assert(false);
    }

    if (m_trace) {
      printf("TLM: %s decode:0x%llX -> i_skt[%d]\n", name(), orig, portId);
    }
    // It is a static port ?
    initiator_socket_type *decodeSocket = &i_skt[portId];
    if (m_is_address_masked) {
      trans.set_address(offset);
    }
    t = t + delay(trans);
    (*decodeSocket)->b_transport(trans, t);
  }

  unsigned int transportDebug(int SocketId, transaction_type &trans) {
    Addr orig = trans.get_address();
    Addr offset;
    int portId = getPortId(orig, offset);

    if (portId < 0) {
      std::cout << "ERROR: " << name() << ": transportDebug()"
                << ": Invalid (undefine memory mapped) address == "
                << tshsu::print(trans.get_address()) << std::endl;
      assert(false);
    }

    if (m_trace) {
      printf("TLM: %s dbg decode:0x%llX -> i_skt[%d]\n", name(), orig, portId);
    }
    // It is a static port ?
    initiator_socket_type *decodeSocket = &i_skt[portId];
    if (m_is_address_masked) {
      trans.set_address(offset);
    }
    return (*decodeSocket)->transport_dbg(trans);
  }

  void adjustRange(int portId, Addr orig, Addr &low, Addr &high) {
    if (DMI_DEBUG) {
      printf("TLM: %s adjustRange port %d input lo:%llX hi:%llX\n", name(),
             portId, low, high);
    }
    icmPortMapping *map = getMapping(portId, orig);
    Addr portLo, portHi;

    map->getRegion(portLo, portHi);

    if (DMI_DEBUG) {
      printf("TLM: %s adjustRange region addr:%llX (lo:%llX hi:%llX)\n", name(),
             orig, portLo, portHi);
    }

    // low is always correct
    low += portLo;

    // if high > decoder, it's the special case of the device not knowing it's
    // size or just being larger than the decoder
    Addr maxDecode = portHi - portLo;
    if (high > maxDecode) {
      high = portLo + maxDecode;
    } else {
      high += portLo;
    }

    if (DMI_DEBUG) {
      printf("TLM: %s adjustRange changed: lo:%llX hi:%llX\n\n", name(), low,
             high);
    }

    assert(high >= low);
    assert(high - low <= maxDecode);
  }

  //
  // Cannot use DMI through plug & play devices. This is probably OK for
  // present.
  //
  bool getDMIPointer(int SocketId, transaction_type &trans,
                     tlm::tlm_dmi &dmi_data) {
    Addr address = trans.get_address();
    Addr offset;
    int portId = getPortId(address, offset);
    bool result = false;

    if (portId < 0) {
      std::cout << "ERROR: " << name() << ": getDMIPointer()"
                << ": Invalid (undefine memory mapped) address == "
                << tshsu::print(trans.get_address()) << std::endl;
      assert(false);
    }

    initiator_socket_type *decodeSocket = &i_skt[portId];

    // send on the transaction with the new address, adjusted for the decoder
    // offset
    if (m_is_address_masked) {
      trans.set_address(offset);
    }
    result = (*decodeSocket)->get_direct_mem_ptr(trans, dmi_data);

    // put the address back how it was
    if (m_is_address_masked) {
      trans.set_address(address);
    }

    // Should always succeed
    Addr start = dmi_data.get_start_address();
    Addr end = dmi_data.get_end_address();

    if (result) {
      // Range must contain address
      assert(start <= offset);
      assert(end >= offset);
    }

    adjustRange(portId, address, start, end);

    dmi_data.set_start_address(start);
    dmi_data.set_end_address(end);

    return result;
  }

  void invalidateDMIPointers(int port_id, sc_dt::uint64 start_range,
                             sc_dt::uint64 end_range) {
    // For each decode on this port, adjust the range for the decode and pass
    // the call on.
    icmPortMapping *decode;
    for (decode = decodes[port_id]; decode; decode = decode->getNext()) {
      sc_dt::uint64 start = decode->offsetOutOf(start_range);
      sc_dt::uint64 end = decode->offsetOutOf(end_range);
      for (unsigned int i = 0; i < NR_OF_TARGET_SOCKETS; ++i) {
        (t_skt[i])->invalidate_direct_mem_ptr(start, end);
      }
    }
  }

private:
  sc_core::sc_time clock_period;
  bool m_trace;
  bool m_is_address_masked;
};

#endif
