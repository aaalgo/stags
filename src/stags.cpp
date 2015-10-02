#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/xml_parser.hpp>
#include "stags.h"

namespace stags {

    struct color_tag;

    void LoadConfig (string const &path, Config *config) {
        try {
            boost::property_tree::read_xml(path, *config);
        }
        catch (...) {
            LOG(warning) << "Cannot load config file " << path << ", using defaults.";
        }
    }

    void SaveConfig (string const &path, Config const &config) {
        boost::property_tree::write_xml(path, config);
    }

    void OverrideConfig (std::vector<std::string> const &overrides, Config *config) {
        for (std::string const &D: overrides) {
            size_t o = D.find("=");
            if (o == D.npos || o == 0 || o + 1 >= D.size()) {
                std::cerr << "Bad parameter: " << D << std::endl;
                BOOST_VERIFY(0);
            }
            config->put<std::string>(D.substr(0, o), D.substr(o + 1));
        }
    }

    class MetaTagger: public Tagger {
        magic_t cookie;
        unordered_map<string, Tagger *> taggers;
    public:
        MetaTagger (Config const &config, vector<Tagger::Constructor> const &cons) {
            string model = config.get<string>("stags.magic.model", "models/magic");
            cookie = magic_open(MAGIC_MIME_TYPE);
            BOOST_VERIFY(cookie);
            magic_load(cookie, model.c_str());
            for (auto const &con: cons) {
                Tagger *ptr = con(config);
                BOOST_VERIFY(ptr);
                taggers[ptr->type()] = ptr;
            }
        }

        virtual ~MetaTagger() {
            for (auto const &p: taggers) {  // release all resources
                delete p.second;
            }
            magic_close(cookie);
        }

        virtual string type () const {
            throw runtime_error("trying to get type of meta tagger");
            return string();
        }

        virtual void tag (string const &object, vector<Tag> *tags) {
            char const *m = magic_buffer(cookie, &object[0], object.size());
            if (m == NULL) {
                tags->emplace_back("mime", "(unknown)");
                return;
            }
            tags->emplace_back("mime", m);
            char const *e = strchr(m, '/');
            if (e == NULL) {
                return;
            }
            string type(m, e);
            auto it = taggers.find(type);
            if (it != taggers.end()) {
                it->second->tag(object, tags);
            }
        }
    };

    Tagger *create_meta_tagger (Config const &config, vector<Tagger::Constructor> const &cons) {
        return new MetaTagger(config, cons);
    }
}
