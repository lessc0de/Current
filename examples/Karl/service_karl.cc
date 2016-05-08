#include "current_build.h"
#include "../../Karl/karl.h"
#include "../../Bricks/dflags/dflags.h"

DEFINE_uint16(nginx_port, 8182, "Port for Nginx to serve proxied queries to Claires.");
DEFINE_string(nginx_config, "", "If set, Karl updates this config with the proxy routes to Claires.");

int main(int argc, char **argv) {
  using namespace current::karl::constants;
  ParseDFlags(&argc, &argv);
  // TODO(dkorolev): Make these paths less non-Windows.
  const current::karl::KarlNginxParameters nginx_parameters(FLAGS_nginx_port, FLAGS_nginx_config);
  const current::karl::Karl karl(kDefaultKarlPort,
                                 ".current/stream",
                                 ".current/storage",
                                 "/",
                                 "http://localhost:" + current::ToString(FLAGS_nginx_port),
                                 nginx_parameters);
  std::cout << "Karl up, http://localhost:" << kDefaultKarlPort << '/' << std::endl;
  HTTP(kDefaultKarlPort).Join();
}