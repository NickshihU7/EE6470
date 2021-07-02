#include <cstdlib>
#include <ctime>

#include "core/common/clint.h"
#include "elf_loader.h"
#include "iss.h"
#include "dma.h"
#include "mem.h"
#include "memory.h"
#include "syscall.h"
#include "twoD_FFT.h"
#include "platform/common/options.h"
#include "fe310_plic.h"

#include "gdb-mc/gdb_server.h"
#include "gdb-mc/gdb_runner.h"

#include <boost/io/ios_state.hpp>
#include <boost/program_options.hpp>
#include <iomanip>
#include <iostream>

using namespace rv32;
namespace po = boost::program_options;

struct TinyOptions : public Options {
public:
	typedef unsigned int addr_t;

	addr_t mem_size = 1024 * 1024 * 32;  // 32 MB ram, to place it before the CLINT and run the base examples (assume
	                                     // memory start at zero) without modifications
	addr_t mem_start_addr = 0x00000000;
	addr_t mem_end_addr = mem_start_addr + mem_size - 1;
	addr_t clint_start_addr = 0x02000000;
	addr_t clint_end_addr = 0x0200ffff;
	addr_t sys_start_addr = 0x02010000;
	addr_t sys_end_addr = 0x020103ff;
	addr_t twoD_FFT0_start_addr = 0x30000000;
	addr_t twoD_FFT0_size = 0x10000000;
	addr_t twoD_FFT0_end_addr = twoD_FFT0_start_addr + twoD_FFT0_size - 1;
	addr_t twoD_FFT1_start_addr = 0x40000000;
	addr_t twoD_FFT1_size = 0x10000000;
	addr_t twoD_FFT1_end_addr = twoD_FFT1_start_addr + twoD_FFT1_size - 1;
	addr_t dma_start_addr = 0x50000000;
	addr_t dma_end_addr = 0x50001000;
	addr_t plic_start_addr = 0x60000000;
	addr_t plic_end_addr = 0x61000000;

	bool quiet = false;
	bool use_E_base_isa = false;

	TinyOptions(void) {
		// clang-format off
		add_options()
			("quiet", po::bool_switch(&quiet), "do not output register values on exit")
			("memory-start", po::value<unsigned int>(&mem_start_addr), "set memory start address")
			("memory-size", po::value<unsigned int>(&mem_size), "set memory size")
			("use-E-base-isa", po::bool_switch(&use_E_base_isa), "use the E instead of the I integer base ISA");
        	// clang-format on
        }

	void parse(int argc, char **argv) override {
		Options::parse(argc, argv);
		mem_end_addr = mem_start_addr + mem_size - 1;
	}
};

int sc_main(int argc, char **argv) {
	TinyOptions opt;
	opt.parse(argc, argv);

	std::srand(std::time(nullptr));  // use current time as seed for random generator

	tlm::tlm_global_quantum::instance().set(sc_core::sc_time(opt.tlm_global_quantum, sc_core::SC_NS));

	ISS core0(0);
	ISS core1(1);
	CombinedMemoryInterface core0_mem_if("MemoryInterface0", core0);
	CombinedMemoryInterface core1_mem_if("MemoryInterface1", core1);

	SimpleMemory mem("SimpleMemory", opt.mem_size);
	ELFLoader loader(opt.input_program.c_str());
	SimpleBus<4, 7> bus("SimpleBus");
	SyscallHandler sys("SyscallHandler");
	FE310_PLIC<2, 64, 96, 32> plic("PLIC");
	CLINT<2> clint("CLINT");
	SimpleDMA dma("SimpleDMA", 4);
	DebugMemoryInterface dbg_if("DebugMemoryInterface");
	twoD_FFT two_D_FFT0("two_D_FFT0");
	twoD_FFT two_D_FFT1("two_D_FFT1");

	std::shared_ptr<BusLock> bus_lock = std::make_shared<BusLock>();
	core0_mem_if.bus_lock = bus_lock;
	core1_mem_if.bus_lock = bus_lock;

	bus.ports[0] = new PortMapping(opt.mem_start_addr, opt.mem_end_addr);
	bus.ports[1] = new PortMapping(opt.clint_start_addr, opt.clint_end_addr);
	bus.ports[2] = new PortMapping(opt.sys_start_addr, opt.sys_end_addr);
	bus.ports[3] = new PortMapping(opt.dma_start_addr, opt.dma_end_addr);
	bus.ports[4] = new PortMapping(opt.twoD_FFT0_start_addr, opt.twoD_FFT0_end_addr);
	bus.ports[5] = new PortMapping(opt.twoD_FFT1_start_addr, opt.twoD_FFT1_end_addr);
	bus.ports[6] = new PortMapping(opt.plic_start_addr, opt.plic_end_addr);

	loader.load_executable_image(mem, mem.size, opt.mem_start_addr);

	core0.init(&core0_mem_if, &core0_mem_if, &clint, loader.get_entrypoint(),
	           opt.mem_end_addr - 3);  // -3 to not overlap with the next region and stay 32 bit aligned
	core1.init(&core1_mem_if, &core1_mem_if, &clint, loader.get_entrypoint(), opt.mem_end_addr - 32767);

	sys.init(mem.data, opt.mem_start_addr, loader.get_heap_addr());
	sys.register_core(&core0);
	sys.register_core(&core1);

	if (opt.intercept_syscalls) {
		core0.sys = &sys;
		core1.sys = &sys;
	}

	// connect TLM sockets
	core0_mem_if.isock.bind(bus.tsocks[0]);
	core1_mem_if.isock.bind(bus.tsocks[1]);
	dbg_if.isock.bind(bus.tsocks[2]);

	PeripheralWriteConnector dma_connector("SimpleDMA-Connector");  // to respect ISS bus locking
	dma_connector.isock.bind(bus.tsocks[3]);
	dma.isock.bind(dma_connector.tsock);
	dma_connector.bus_lock = bus_lock;

	bus.isocks[0].bind(mem.tsock);
	bus.isocks[1].bind(clint.tsock);
	bus.isocks[2].bind(sys.tsock);
	bus.isocks[3].bind(dma.tsock);
	bus.isocks[4].bind(two_D_FFT0.tsock);
	bus.isocks[5].bind(two_D_FFT1.tsock);
	bus.isocks[6].bind(plic.tsock);

	// connect interrupt signals/communication
	clint.target_harts[0] = &core0;
	clint.target_harts[1] = &core1;
	plic.target_harts[0] = &core0;
	plic.target_harts[1] = &core1;
	dma.plic = &plic;

	// switch for printing instructions
	core0.trace = opt.trace_mode;
	core1.trace = opt.trace_mode;

	std::vector<debug_target_if *> threads;
	threads.push_back(&core0);
	threads.push_back(&core1);

	if (opt.use_debug_runner) {
		auto server = new GDBServer("GDBServer", threads, &dbg_if, opt.debug_port);
		new GDBServerRunner("GDBRunner0", server, &core0);
		new GDBServerRunner("GDBRunner1", server, &core1);
	} else {
		new DirectCoreRunner(core0);
		new DirectCoreRunner(core1);
	}

	if (opt.quiet)
		sc_core::sc_report_handler::set_verbosity_level(sc_core::SC_NONE);

	sc_core::sc_start();
	if (!opt.quiet) {
		core0.show();
		core1.show();
	}

	return 0;
}
