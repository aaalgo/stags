#include <cctype>
#include <vector>
#include <string>
#include <exception>
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

    class Exception: public std::exception {
    };

    class MultiPart: public vector<Part> {
        static unsigned spaceCRLF (string const &s, unsigned off) {
            off = s.find("\r\n", off);
            if (off == string::npos) {
                throw Exception();
            }
            return off + 2;
        }
        static unsigned getline (string const &s, unsigned off, string *line) {
            unsigned e = s.find("\r\n", off);
            if (e == string::npos) {
                throw Exception();
            }
            *line = s.substr(off, e - off);
            return e + 2;
        }
    public:
        MultiPart (string const &content_type,
                   string const &body) {
            //std::cerr << "CT: " << content_type << std::endl;
            unsigned off = content_type.find("multipart/");
            if (off != 0) {
                throw Exception();
            }
            off = content_type.find("boundary=");
            if (off == string::npos) {
                throw Exception();
            }
            off += 9; // len("boundary")
            if (off >= content_type.size()) {
                throw Exception();
            }
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
                if (boundary.empty() || boundary.back() != '"') {
                    throw Exception();
                }
                boundary.pop_back();
            }
            if (boundary.empty()) {
                throw Exception();
            }

            boundary = "--" + boundary;
            //std::cerr << "B: " << boundary << std::endl;

            off = body.find(boundary);
            if (off != 0) throw Exception();
            off = spaceCRLF(body, off);
            boundary = "\r\n" + boundary;
            while (off < body.size()) {
                //std::cerr << "P" << std::endl;
                unsigned end = body.find(boundary, off);
                if (end == string::npos) throw Exception();
                if (end <= off) throw Exception();
                // parse header
                for (;;) {
                    string line;
                    unsigned off2 = getline(body, off, &line);
                    if (off2 <= off) throw Exception();
                    off = off2;
                    if (line.empty()) break;
                    //std::cerr << "H: " << line << std::endl;
                }
                Part part;
                part.body_ = body.substr(off, end - off);
                push_back(std::move(part));
                off = end + boundary.size();
                if (off + 2 > body.size()) throw Exception();
                if (body[off] == '-' && body[off+1] == '-') break;
                off = spaceCRLF(body, off);
                if (off == string::npos) break;
            }
        }
    };
}
