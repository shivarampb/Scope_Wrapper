// visastub_api.h — test-side control surface for the VISA simulator.
#ifndef VISASTUB_API_H
#define VISASTUB_API_H

#include <string>
#include <vector>

namespace visastub {
// Clear the command log and any queued responses (call before each test).
void reset();
// All commands written since the last reset (terminators stripped).
std::vector<std::string> commands();
// True if any logged command equals in_cmd.
bool sawCommand(const std::string& in_cmd);
}

#endif // VISASTUB_API_H
