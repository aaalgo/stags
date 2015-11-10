#include <fstream>
#include <boost/lexical_cast.hpp>
#include <caffex.h>
#include "stags.h"

using namespace std;

namespace stags {

    class FaceTagger: public Tagger {
        string model_dir;
        cv::CascadeClassifier cascade;
        caffex::Caffex xtor;
        vector<string> labels;
    public:
        FaceTagger (Config const &config)
            : model_dir(config.get<string>("stags.face.model", "models/face")),
            xtor(model_dir)
        {
            if (!cascade.load(model_dir + "/detector")) {
                BOOST_VERIFY(0);
            }
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

        ~FaceTagger () {
        }

        virtual string type () const {
            return "image";
        }

        virtual void tag (string const &object, vector<Tag> *tags) {
            cv::Mat buffer(1, object.size(), CV_8U, const_cast<void *>(reinterpret_cast<void const *>(&object[0])));
            cv::Mat img = cv::imdecode(buffer, -1);
            if (img.empty()) return;

            cv::Mat gray;
            cv::cvtColor(img, gray, CV_RGB2GRAY);
            cv::equalizeHist(gray, gray);
            vector<cv::Rect> face_rois;
            cascade.detectMultiScale(gray, face_rois,
                1.1, 2, 0
                //|CV_HAAR_FIND_BIGGEST_OBJECT
                //|CV_HAAR_DO_ROUGH_SEARCH
                |CV_HAAR_SCALE_IMAGE
                ,
                cv::Size(30, 30));

            if (face_rois.empty()) return;

            vector<cv::Mat> faces;
            for (auto roi: face_rois) {
                faces.push_back(img(roi));
            }

            cv::Mat result;
            xtor.apply(faces, &result);

            vector<float> ft(result.ptr<float>(0), result.ptr<float>(0) + result.cols);
            for (unsigned r = 1; r < result.rows; ++r) {
                float const *ptr = result.ptr<float>(r);
                for (unsigned i = 0; i < ft.size(); ++i) {
                    if (ptr[i] > ft[i]) ft[i] = ptr[i];
                }
            }

            /* Print the top N predictions. */
            BOOST_VERIFY(labels.empty() || labels.size() == ft.size());
            for (size_t i = 0; i < ft.size(); ++i) {
                string label;
                if (labels.empty()) {
                    label = boost::lexical_cast<string>(i);
                }
                else {
                    label = labels[i];
                }
                tags->emplace_back("OBJECT", label, ft[i]);
            }
        }
    };

    Tagger *create_image_tagger (Config const &config) {
        return new FaceTagger(config);
    }
}
