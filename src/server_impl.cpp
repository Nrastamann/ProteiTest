#include "logger.hpp"
#include "server.hpp"

int main(int argc, char* argv[])
{
  logging::MultithreadLogger::loggerInit("LogsServer");
  server::serverStart(argc, argv);
}
