#ifndef MEMORYMAP_H_
#define MEMORYMAP_H_
#include <systemc>
using Addr = sc_dt::uint64;

// Sobel filter Memory Map
// Used between SimpleBus & SobelFilter
const int GAUSSIAN_MM_BASE = 0x90000000;
const int GAUSSIAN_MM_SIZE = 0x00000010;
const int GAUSSIAN_MM_MASK = 0x00000007;

class icmPortMapping {
private:
  Addr lo;
  Addr hi;
  icmPortMapping *next;

public:
  icmPortMapping(Addr lo_n, Addr hi_n) : lo(lo_n), hi(hi_n), next(0) {}
  void setNext(icmPortMapping *n) { next = n; }
  icmPortMapping *getNext() { return next; }

  void getRegion(Addr &l, Addr &h) {
    l = lo;
    h = hi;
  }

  bool matches(Addr lo_s, Addr hi_s) { return lo == lo_s && hi == hi_s; }

  bool inRegion(Addr a) { return a >= lo && a <= hi; }

  Addr offsetInto(Addr a) { return a - lo; }
  Addr offsetOutOf(Addr a) { return a + lo; }
};

class MemoryMap {
public:
  MemoryMap(char const *the_memory_map_name,
            const unsigned int the_no_of_targets)
      : _memory_map_name(the_memory_map_name),
        _no_of_targets(the_no_of_targets) {
    decodes = new icmPortMapping *[_no_of_targets]();
  }
  ~MemoryMap() { delete[] decodes; }
  std::string memory_map_name() { return _memory_map_name; }

public:
  unsigned int no_of_targets() { return _no_of_targets; }

private:
  std::string _memory_map_name;
  const unsigned int _no_of_targets;

protected:
  icmPortMapping **decodes;

  icmPortMapping *getMapping(int port, Addr address) {
    icmPortMapping *decode;
    for (decode = decodes[port]; decode; decode = decode->getNext()) {
      if (decode->inRegion(address)) {
        return decode;
      }
    }
    return 0;
  }

public:
  int getPortId(const Addr address, Addr &offset) {
    unsigned int i;
    for (i = 0; i < no_of_targets(); i++) {
      icmPortMapping *decode = getMapping(i, address);
      if (decode) {
        offset = decode->offsetInto(address);
        return i;
      }
    }
    return -1;
  }

public:
  void setDecode(int portId, Addr lo, Addr hi) {
    if (portId >= static_cast<int>(no_of_targets())) {
      printf("Error configuring TLM decoder %s: portId (%d) cannot be greater "
             "than the number of targets (%d)\n",
             memory_map_name().c_str(), portId, no_of_targets());
      return;
      abort();
    }
    if (lo > hi) {
      printf("Error configuring TLM decoder %s: lo (0x%llX) cannot be greater "
             "than hi (0x%llX)\n",
             memory_map_name().c_str(), lo, hi);
      return;
    }
    icmPortMapping *decode = new icmPortMapping(lo, hi);
    decode->setNext(decodes[portId]);
    decodes[portId] = decode;
  }
};
#endif
