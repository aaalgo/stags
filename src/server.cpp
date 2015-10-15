#include <boost/program_options.hpp>
#include <served/served.hpp>
#include "stags.h"
#include "multipart.h"

using namespace std;
using namespace stags;

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

    MetaTaggerManager taggers(config);
    /*
    taggers.registerTagger(create_text_tagger);
    */
    taggers.registerTagger(create_image_tagger);

    // GET /hello
    mux.handle("/hello")
        .get([](served::response &res, const served::request &req) {
            res << "Hello world!";
        });
    mux.handle("/tag")
        .post([&taggers](served::response &res, const served::request &req) {
                try {
                    rfc2046::MultiPart parts(req.header("Content-Type"), req.body());
                    Json::array v;
                    Tagger *tagger = taggers.get();
                    for (auto const &p: parts) {
                        vector<Tag> tags;
                        tagger->tag(p.body(), &tags);
                        for (Tag const &t: tags) {
                            v.push_back(Json::object{
                                    {"type", t.type},
                                    {"value", t.value},
                                    {"score", t.score},
                                    });
                        }
                    }
                    Json json = Json::object{{"objects", v},};
                    res.set_body(json.dump());
                }
                catch (rfc2046::Exception const &e) {
                    res.set_header("StagsErrorCode", "1");
                    res.set_header("StagsErrorText", "failed to parse multipart data");
                }
                catch (std::exception const &e) {
                    res.set_header("StagsErrorCode", "2");
                    res.set_header("StagsErrorText", e.what());
                }
                catch (...) {
                    res.set_header("StagsErrorCode", "2");
                    res.set_header("StagsErrorText", "unknown error");
                }
        });

    string address = config.get<string>("stags.server.address", "0.0.0.0");
    string port = config.get<string>("stags.server.port", "8080");
    size_t max_input = config.get<size_t>("stags.server.max_input", 2 * 1024 * 1024);
    unsigned threads = config.get<unsigned>("stags.server.threads", 4);

    // Create the server and run with 10 handler threads.
    LOG(info) << "listening at " << address << ':' << port;
    served::net::server server(address, port, mux);
    server.set_max_request_bytes(max_input);
    LOG(info) << "running server with " << threads << " threads.";
    server.run(threads);

    cleanup_logging();

    return (EXIT_SUCCESS);
}

