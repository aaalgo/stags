#include <caffex-xgboost.h>
#undef LOG
#undef LOG_IF
#include "stags.h"

using std::string;

namespace stags {
    using boost::lexical_cast;
    using std::ifstream;
    using std::cout;
    using std::cerr;
    using std::endl;

    class ImageTagger: public Tagger {
        string model_dir;
        caffex::CaffexBoost xtor;
        vector<string> labels;
    public:
        ImageTagger (Config const &config)
            : model_dir(config.get<string>("stags.image.model", "models/image")),
            xtor(model_dir)
        {
            {
                ifstream is((model_dir + "/label").c_str());
                if (is) {
                    string l;
                    while (getline(is, l)) {
                        labels.push_back(l);
                    }
                }
            }
        }

        ~ImageTagger () {
        }

        virtual string type () const {
            return "image";
        }

        virtual void tag (string const &object, vector<Tag> *tags) {
            cv::Mat buffer(1, object.size(), CV_8U, const_cast<void *>(reinterpret_cast<void const *>(&object[0])));
            cv::Mat img = cv::imdecode(buffer, -1);
            if (img.empty()) return;
            std::vector<float> ft;
            xtor.apply(img, &ft);
            /* Print the top N predictions. */
            BOOST_VERIFY(labels.empty() || labels.size() == ft.size());
            for (size_t i = 0; i < ft.size(); ++i) {
                string label;
                if (labels.empty()) {
                    label = lexical_cast<string>(i);
                }
                else {
                    label = labels[i];
                }
                tags->emplace_back("OBJECT", label, ft[i]);
            }
        }
    };

    Tagger *create_image_tagger (Config const &config) {
        return new ImageTagger(config);
    }
}
