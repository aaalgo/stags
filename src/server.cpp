#include <magic.h>
#include <boost/program_options.hpp>
#include <served/served.hpp>
#include "stags.h"
#include "multipart.h"

using namespace std;
using namespace stags;

class MultiPartFormData: vector<served::request> {
public:
};

int main(int argc, char const* argv[]) {
    string config_path;
    vector<string> overrides;
    int nice;

    namespace po = boost::program_options;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message.")
        ("config", po::value(&config_path)->default_value("stags.xml"), "")
        ("override,D", po::value(&overrides), "override configuration.")
        ("nice", po::value(&nice)->default_value(10), "do not lower priority")
        ;

    po::positional_options_description p;

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
                     options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << "Usage:" << endl;
        cout << "\tserver ..." << endl;
        cout << desc;
        cout << endl;
        return 0;
    }

    Config config;
    try {
        LoadConfig(config_path, &config);
    } catch (...) {
        cerr << "Failed to load config file: " << config_path << ", using defaults." << endl;
    }
    OverrideConfig(overrides, &config);

    setup_logging(config);

    // Create a multiplexer for handling requests
    served::multiplexer mux;

    magic_t cookie = magic_open(MAGIC_MIME_TYPE);
    BOOST_VERIFY(cookie);
    magic_load(cookie, NULL);

    // GET /hello
    mux.handle("/hello")
        .get([](served::response &res, const served::request &req) {
            res << "Hello world!";
        });
    mux.handle("/tag")
        .post([&cookie](served::response &res, const served::request &req) {
                rfc2046::MultiPart parts(req.header("Content-Type"), req.body());
                Json::array v;
                for (auto const &p: parts) {
                    string const &body = p.body();
                    char const *m = magic_buffer(cookie, &body[0], body.size());
                    if (m == NULL) {
                        LOG(error) << magic_error(cookie);
                        m = "unknown";
                    }
                    v.push_back(Json::object{
                            {"size", int(body.size())},
                            {"type", string(m)},
                            });
                }
                Json json = Json::object{{"objects", v},};
                /*
                for (auto const &p: req.params) {
                    cerr << p.first << ": " << p.second << endl;
                }
                for (auto const &p: req.query) {
                    cerr << p.first << ": " << p.second << endl;
                }
                */
                //cerr << endl;
                //cerr << req.body() << endl;
                res.set_body(json.dump());
        });

    string address = config.get<string>("stags.server.address", "127.0.0.1");
    string port = config.get<string>("stags.server.port", "8080");
    unsigned threads = config.get<unsigned>("stags.server.threads", 16);

    // Create the server and run with 10 handler threads.
    LOG(info) << "listening at " << address << ':' << port;
    served::net::server server(address, port, mux);
    LOG(info) << "running server with " << threads << " threads.";
    server.run(threads);

    magic_close(cookie);
    cleanup_logging();

    return (EXIT_SUCCESS);
}

