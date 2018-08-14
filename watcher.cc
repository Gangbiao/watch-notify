#include <iostream>
#include <string.h>

#include <rados/librados.hpp>
#include <rados/librados.h>

using namespace std;

class RadosWatchCtx : public librados::WatchCtx2 {
  librados::IoCtx& ioctx;
  std::string name;
public:
  RadosWatchCtx(librados::IoCtx& io, const char *_name) : ioctx(io), name(_name) {}
  ~RadosWatchCtx() override {}
  void handle_notify(uint64_t notify_id,
                     uint64_t cookie,
                     uint64_t notifier_id,
                     bufferlist& bl) override {
    std::cout << "NOTIFY"
         << " cookie " << cookie
         << " notify_id " << notify_id
         << " from " << notifier_id
         << std::endl;
    bl.hexdump(cout);
    ioctx.notify_ack(name, notify_id, cookie, bl);
  }
  void handle_error(uint64_t cookie, int err) override {
    std::cout << "ERROR"
         << " cookie " << cookie
         << std::endl;
  }
};


int main(int argc, const char **argv){
  int ret = 0;
  // we will use all of these below
  const char *pool_name = "rbd";
  std::string hello("hello world!");
  std::string object_name("my-object");
  librados::IoCtx io_ctx;
 
  // first, we create a Rados object and initialize it
  librados::Rados rados;
  {
    ret = rados.init("admin"); // just use the client.admin keyring
    if (ret < 0) { // let's handle any error that might have come back
      std::cerr << "couldn't initialize rados! error " << ret << std::endl;
      ret = EXIT_FAILURE;
    } else {
      std::cout << "we just set up a rados cluster object" << std::endl;
    }
  }

  /*
   * Now we need to get the rados object its config info. It can
   * parse argv for us to find the id, monitors, etc, so let's just
   * use that.
   */
  {
     ret = rados.conf_parse_argv(argc, argv);
     if (ret < 0) {
       // This really can't happen, but we need to check to be a good citizen.
       std::cerr << "failed to parse config options! error " << ret << std::endl;
       ret = EXIT_FAILURE;
     } else {
       std::cout << "we just parsed our config options" << std::endl;
       // We also want to apply the config file if the user specified
       // one, and conf_parse_argv won't do that for us.
       for (int i = 0; i < argc; ++i) {
         if ((strcmp(argv[i], "-c") == 0) || (strcmp(argv[i], "--conf") == 0)) {
           ret = rados.conf_read_file(argv[i+1]);
           if (ret < 0) {
             // This could fail if the config file is malformed, but it'd be hard.
             std::cerr << "failed to parse config file " << argv[i+1]
                       << "! error" << ret << std::endl;
             ret = EXIT_FAILURE;
           }
           break;
         }
       }
     }
   }

  /*
   * next, we actually connect to the cluster
   */
  {
    ret = rados.connect();
    if (ret < 0) {
      std::cerr << "couldn't connect to cluster! error " << ret << std::endl;
      ret = EXIT_FAILURE;
    } else {
      std::cout << "we just connected to the rados cluster" << std::endl;
    }
  }

  /*
   * create an "IoCtx" which is used to do IO to a pool
   */
  {
    ret = rados.ioctx_create(pool_name, io_ctx);
    if (ret < 0) {
      std::cerr << "couldn't set up ioctx! error " << ret << std::endl;
      ret = EXIT_FAILURE;
    } else {
      std::cout << "we just created an ioctx for our pool" << std::endl;
    }
  }
  /*
  * Now we create a watcher on object 'my-object' in pool 'rbd'
  */
  string oid("my-object");
  RadosWatchCtx ctx(io_ctx, oid.c_str());
  uint64_t cookie;
  ret = io_ctx.watch2(oid, &cookie, &ctx);
  if (ret != 0)
    std::cout << "error calling watch: " << std::endl;
  else {
    std::cout << "press enter to exit..." << std::endl;
    getchar();
    io_ctx.unwatch2(cookie);
    rados.watch_flush();
  }

  return 0;
}
