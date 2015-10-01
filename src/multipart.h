#include <cctype>
#include <vector>
#include <string>
#include <unordered_map>

namespace rfc2046 {
    using std::string;
    using std::vector;
    using std::unordered_map;

    class Part: public unordered_map<string, string> {
        string body_;
    public:
        friend class MultiPart;
        string const &body () const {
            return body_;
        }
    };

    class MultiPart: public vector<Part> {
        static unsigned spaceCRLF (string const &s, unsigned off) {
            off = s.find("\r\n", off);
            BOOST_VERIFY(off != string::npos);
            return off + 2;
        }
        static unsigned getline (string const &s, unsigned off, string *line) {
            unsigned e = s.find("\r\n", off);
            BOOST_VERIFY(e != string::npos);
            *line = s.substr(off, e - off);
            return e + 2;
        }
    public:
        MultiPart (string const &content_type,
                   string const &body) {
            //std::cerr << "CT: " << content_type << std::endl;
            unsigned off = content_type.find("multipart/");
            BOOST_VERIFY(off == 0);
            off = content_type.find("boundary=");
            BOOST_VERIFY(off != content_type.npos);
            off += 9; // len("boundary")
            BOOST_VERIFY(off < content_type.size());
            bool quoted = false;
            if (content_type[off] == '"') {
                quoted = true;
                ++off;
            }
            string boundary = content_type.substr(off);
            for (;;) {
                if (boundary.empty()) break;
                if (!std::isspace(boundary.back())) break;
                boundary.pop_back();
            }
            if (quoted) {
                BOOST_VERIFY(boundary.size());
                BOOST_VERIFY(boundary.back() == '"');
                boundary.pop_back();
            }
            BOOST_VERIFY(boundary.size());

            boundary = "--" + boundary;
            //std::cerr << "B: " << boundary << std::endl;

            off = body.find(boundary);
            BOOST_VERIFY(off == 0);
            off = spaceCRLF(body, off);
            boundary = "\r\n" + boundary;
            while (off < body.size()) {
                //std::cerr << "P" << std::endl;
                unsigned end = body.find(boundary, off);
                BOOST_VERIFY(end != string::npos);
                BOOST_VERIFY(end > off);
                // parse header
                for (;;) {
                    string line;
                    unsigned off2 = getline(body, off, &line);
                    BOOST_VERIFY(off2 > off);
                    off = off2;
                    if (line.empty()) break;
                    //std::cerr << "H: " << line << std::endl;
                }
                Part part;
                part.body_ = body.substr(off, end - off);
                push_back(std::move(part));
                off = end + boundary.size();
                BOOST_VERIFY(off + 2 <= body.size());
                if (body[off] == '-' && body[off+1] == '-') break;
                off = spaceCRLF(body, off);
                if (off == string::npos) break;
            }
        }
    };
}
