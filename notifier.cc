#include <iostream>
#include <string.h>

#include <rados/librados.hpp>
#include <rados/librados.h>
#include "msg/msg_types.h" //decode encode

using namespace std;

int main(int argc, const char **argv){
  int ret = 0;
  // we will use all of these below
  const char *pool_name = "rbd";
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
  * notifier
  */
  string oid("my-object");
  string msg("hello world");
  bufferlist bl, replybl;
  encode(msg, bl);
  ret = io_ctx.notify2(oid, bl, 10000, &replybl);
  if (ret != 0)
    std::cout << "error calling notify" << std::endl;
  if (replybl.length()) {
    map<pair<uint64_t,uint64_t>,bufferlist> rm;
    set<pair<uint64_t,uint64_t> > missed;
    auto p = replybl.begin();
    decode(rm, p);
    decode(missed, p);
    for (map<pair<uint64_t,uint64_t>,bufferlist>::iterator p = rm.begin();
    p != rm.end();
    ++p) {
      std::cout << "reply client." << p->first.first
      << " cookie " << p->first.second
      << " : " << p->second.length() << " bytes" << std::endl;
      if (p->second.length())
      p->second.hexdump(cout);
    }
    for (multiset<pair<uint64_t,uint64_t> >::iterator p = missed.begin();
    p != missed.end(); ++p) {
      std::cout << "timeout client." << p->first
      << " cookie " << p->second << std::endl;
    }
  }

  return 0;
}
