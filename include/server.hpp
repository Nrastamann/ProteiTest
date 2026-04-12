#pragma once
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <nlohmann/json.hpp>

namespace server {

inline constexpr uint16_t kPortNum{5001};
inline constexpr size_t kBufferSize{4096};

//void dataManipulation(std::string& result, ::PolymorphicVectorQuad& vector);

int serverStart(int argc, char** argv);

struct WriteSocketN {};
struct GetSocketN {};

}  // namespace server
