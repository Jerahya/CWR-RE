#pragma once

#ifdef __SWITCH__

namespace Poseidon::Foundation
{
// Call first thing in main(). Brings up sockets, routes stdout/stderr to nxlink
// (when launched with `nxlink -s`) or to sdmc:/switch/cwr-re/log.txt, points HOME at
// sdmc:/switch/cwr-re, and injects `-C sdmc:/switch/cwr-re/data` when no -C was given.
// May replace argc/argv; the new argv stays valid for the process lifetime.
void SwitchStartup(int& argc, char**& argv);

// Call before returning from main().
void SwitchShutdown();

// Blocking system error dialog. `message` is shown first, `details` on the
// applet's details page.
void SwitchShowError(const char* message, const char* details);
} // namespace Poseidon::Foundation

#endif
