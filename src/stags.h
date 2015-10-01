#include <thread>
#include <mutex>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <functional>
#include <unordered_map>
#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/ptree.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <json11.hpp>
#include <magic.h>

namespace stags {

    using std::string;
    using std::vector;
    using std::function;
    using std::unordered_map;
    using std::runtime_error;
    using namespace json11;

    typedef boost::property_tree::ptree Config;

    void LoadConfig (string const &path, Config *);
    void SaveConfig (string const &path, Config const &);
    // we allow overriding configuration options in the form of "KEY=VALUE"
    void OverrideConfig (std::vector<std::string> const &overrides, Config *);

    static constexpr int64_t ErrorCode_Success = 0;
    static constexpr int64_t ErrorCode_Unknown = -1;

    class Error: public runtime_error {
        int32_t c;
    public:
        explicit Error (string const &what): runtime_error(what), c(ErrorCode_Unknown) {}
        explicit Error (char const *what): runtime_error(what), c(ErrorCode_Unknown) {}
        explicit Error (string const &what, int32_t code): runtime_error(what), c(code) {}
        explicit Error (char const *what, int32_t code): runtime_error(what), c(code) {}
        virtual int32_t code () const {
            return c;
        }
    };

#define DEFINE_ERROR(name, cc) \
    class name: public Error { \
    public: \
        explicit name (string const &what): Error(what) {} \
        explicit name (char const *what): Error(what) {} \
        virtual int32_t code () const { return cc;} \
    }

    DEFINE_ERROR(InternalError, 0x0001);
    DEFINE_ERROR(ExternalError, 0x0002);
    DEFINE_ERROR(OutOfMemoryError, 0x0004);
    DEFINE_ERROR(FileSystemError, 0x0008);
    DEFINE_ERROR(RequestError, 0x0010);
    DEFINE_ERROR(ConfigError, 0x0020);
    DEFINE_ERROR(PluginError, 0x0040);
    DEFINE_ERROR(PermissionError, 0x0080);
    DEFINE_ERROR(NotImplementedError, 0x0100);
#undef DEFINE_ERROR

#define LOG(x) BOOST_LOG_TRIVIAL(x)
    namespace logging = boost::log;
    void setup_logging (Config const &);
    void cleanup_logging ();

    struct Tag {
        string type;
        string value;
        double score;

        Tag () {
        }

        Tag (string const &t, string const &v, float s = 0): type(t), value(v), score(s) {
        }

    };

    // tagger interface
    class Tagger {
    public:
        typedef function<Tagger *(Config const&)> Constructor;
        virtual ~Tagger() {}
        virtual string type () const = 0;
        virtual void tag (string const &object, vector<Tag> *tags) = 0;
    };

    Tagger *create_meta_tagger (Config const &, vector<Tagger::Constructor> const &);
    Tagger *create_text_tagger (Config const &);
    Tagger *create_image_tagger (Config const &);
    Tagger *create_audio_tagger (Config const &);

    class MetaTaggerManager {
        Config config;
        vector<Tagger::Constructor> cons;    // registered constructors
        unordered_map<std::thread::id, Tagger *> insts;   // instance for each thread
        std::mutex mutex;
    public:
        MetaTaggerManager (Config const &c): config(c) {
        }
        ~MetaTaggerManager () {
            for (auto &p: insts) {
                delete p.second;
            }
        }
        void registerTagger (Tagger::Constructor const &c) {
            cons.push_back(c);
        }
        Tagger *get () {
            std::thread::id id = std::this_thread::get_id();
            std::lock_guard<std::mutex> lock(mutex); 
            auto it = insts.find(id);
            if (it == insts.end()) {
                Tagger *tagger = create_meta_tagger(config, cons);
                insts[id] = tagger;
                return tagger;
            }
            else {
                return it->second;
            }
        }
    };
}

