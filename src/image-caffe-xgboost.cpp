#include <xgboost_wrapper.h>
#define CPU_ONLY 1
#include <caffe/caffe.hpp>
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <boost/lexical_cast.hpp>
#undef LOG
#undef LOG_IF
#include "stags.h"
#undef CHECK_EQ
#undef CHECK
#define CHECK_EQ(a,b) if ((a) != (b)) LOG(error)
#define CHECK(a)    if (!(a)) LOG(error)

using namespace caffe;  // NOLINT(build/namespaces)
using std::string;

/* Pair (label, confidence) representing a prediction. */
class Caffex {
 public:
  Caffex(const string& model,
             const vector<string> &blobs); 

  void apply (const cv::Mat &img, vector<float> *);

 private:
  void SetMean(const string& mean_file);
  void WrapInputLayer(std::vector<cv::Mat>* input_channels);
  void Preprocess(const cv::Mat& img,
                  std::vector<cv::Mat>* input_channels);

 private:
  std::shared_ptr<Net<float> > net_;
  cv::Size input_geometry_;
  int num_channels_;
  cv::Mat mean_;
  std::vector<string> blobs_;
};

     Caffex::Caffex(const string& model,
             const vector<string> &blobs) {
    string model_file   = model + "/model";
    string trained_file = model + "/trained";
    string mean_file    = model + "/mean";
    string label_file   = model + "/label";
#ifdef CPU_ONLY
  Caffe::set_mode(Caffe::CPU);
#else
  Caffe::set_mode(Caffe::GPU);
#endif

  /* Load the network. */
  net_.reset(new Net<float>(model_file, TEST));
  net_->CopyTrainedLayersFrom(trained_file);

  CHECK_EQ(net_->num_inputs(), 1) << "Network should have exactly one input." << net_->num_inputs();
  /*
  CHECK_EQ(net_->num_outputs(), 1) << "Network should have exactly one output." << net_->num_outputs();
  */

  Blob<float>* input_layer = net_->input_blobs()[0];
  num_channels_ = input_layer->channels();
  CHECK(num_channels_ == 3 || num_channels_ == 1)
    << "Input layer should have 1 or 3 channels.";
  input_geometry_ = cv::Size(input_layer->width(), input_layer->height());

  /* Load the binaryproto mean file. */
  SetMean(mean_file);

  blobs_ = blobs;
  }

  void Caffex::apply (const cv::Mat &img, vector<float> *ft) {
    ft->clear();
  Blob<float>* input_layer = net_->input_blobs()[0];
  input_layer->Reshape(1, num_channels_,
                       input_geometry_.height, input_geometry_.width);
  /* Forward dimension change to all layers. */
  net_->Reshape();

  std::vector<cv::Mat> input_channels;
  WrapInputLayer(&input_channels);

  Preprocess(img, &input_channels);

  net_->ForwardPrefilled();

  for (string const &blob: blobs_) {
      const shared_ptr<Blob<float> > feature_blob = net_->blob_by_name(blob);
      int batch_size = feature_blob->num();
      BOOST_VERIFY(batch_size == 1);
      int dim_features = feature_blob->count() / batch_size;
      const float* feature_blob_data;
      feature_blob_data = feature_blob->cpu_data() + feature_blob->offset(0);
      //!!!TODO
      for (int d = 0; d < dim_features; ++d) {
        ft->push_back(feature_blob_data[d]);
      }
  }  // for (int n = 0; n < batch_size; ++n)
  }

  void Caffex::SetMean(const string& mean_file) {
  BlobProto blob_proto;
  ReadProtoFromBinaryFileOrDie(mean_file.c_str(), &blob_proto);

  /* Convert from BlobProto to Blob<float> */
  Blob<float> mean_blob;
  mean_blob.FromProto(blob_proto);
  CHECK_EQ(mean_blob.channels(), num_channels_)
    << "Number of channels of mean file doesn't match input layer.";

  /* The format of the mean file is planar 32-bit float BGR or grayscale. */
  std::vector<cv::Mat> channels;
  float* data = mean_blob.mutable_cpu_data();
  for (int i = 0; i < num_channels_; ++i) {
    /* Extract an individual channel. */
    cv::Mat channel(mean_blob.height(), mean_blob.width(), CV_32FC1, data);
    channels.push_back(channel);
    data += mean_blob.height() * mean_blob.width();
  }

  /* Merge the separate channels into a single image. */
  cv::Mat mean;
  cv::merge(channels, mean);

  /* Compute the global mean pixel value and create a mean image
   * filled with this value. */
  cv::Scalar channel_mean = cv::mean(mean);
  mean_ = cv::Mat(input_geometry_, mean.type(), channel_mean);
  }


  void Caffex::WrapInputLayer(std::vector<cv::Mat>* input_channels){
  Blob<float>* input_layer = net_->input_blobs()[0];

  int width = input_layer->width();
  int height = input_layer->height();
  float* input_data = input_layer->mutable_cpu_data();
  for (int i = 0; i < input_layer->channels(); ++i) {
    cv::Mat channel(height, width, CV_32FC1, input_data);
    input_channels->push_back(channel);
    input_data += width * height;
  }
  }

  void Caffex::Preprocess(const cv::Mat& img,
                  std::vector<cv::Mat>* input_channels) {
  /* Convert the input image to the input image format of the network. */
  cv::Mat sample;
  if (img.channels() == 3 && num_channels_ == 1)
    cv::cvtColor(img, sample, CV_BGR2GRAY);
  else if (img.channels() == 4 && num_channels_ == 1)
    cv::cvtColor(img, sample, CV_BGRA2GRAY);
  else if (img.channels() == 4 && num_channels_ == 3)
    cv::cvtColor(img, sample, CV_BGRA2BGR);
  else if (img.channels() == 1 && num_channels_ == 3)
    cv::cvtColor(img, sample, CV_GRAY2BGR);
  else
    sample = img;

  cv::Mat sample_resized;
  if (sample.size() != input_geometry_)
    cv::resize(sample, sample_resized, input_geometry_);
  else
    sample_resized = sample;

  cv::Mat sample_float;
  if (num_channels_ == 3)
    sample_resized.convertTo(sample_float, CV_32FC3);
  else
    sample_resized.convertTo(sample_float, CV_32FC1);

  cv::Mat sample_normalized;
  cv::subtract(sample_float, mean_, sample_normalized);

  /* This operation will write the separate BGR planes directly to the
   * input layer of the network because it is wrapped by the cv::Mat
   * objects in input_channels. */
  cv::split(sample_normalized, *input_channels);

  CHECK(reinterpret_cast<float*>(input_channels->at(0).data)
        == net_->input_blobs()[0]->cpu_data())
    << "Input channels are not wrapping the input layer of the network.";
  }




namespace stags {
    using boost::lexical_cast;
    class CSV: public vector<string> {
    public:
        CSV (string const &c) {
            BOOST_VERIFY(c.size());
            unsigned o = 0;
            for (;;) {
                auto p = c.find(',', o);
                if (p == c.npos) break;
                push_back(c.substr(o, p-o));
                o = p + 1;
            }
            push_back(c.substr(o));
            for (auto const &s: *this) {
                cout << "BLOB: " << s << endl;
            }
        }
    };

    class ImageTagger: public Tagger {
        string model_dir;
        CSV blobs;
        Caffex xtor;
        BoosterHandle cfier;
        vector<string> labels;
    public:
        ImageTagger (Config const &config)
            : model_dir(config.get<string>("stags.image.model", "models/image")),
            CSV(config.get<string>("stags.image.blobs")),
            xtor(model_dir, CSV)
        {
            int r = XGBoosterCreate(NULL, 0, &cfier);
            BOOST_VERIFY(r == 0);
            r = XGBoosterLoadModel(cfier, (model_dir + "/boost").c_str());
            BOOST_VERIFY(r == 0);
            ifstream is((model_dir + "/label").c_str());
            string l;
            while (getline(is, l)) {
                labels.push_back(l);
            }
        }

        ~ImageTagger () {
            XGBoosterFree(cfier);
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
            DMatrixHandle dmat;
            int r = XGDMatrixCreateFromMat(&ft[0], 1, ft.size(), 0, &dmat);
            bst_ulong len;
            float *out;
            XGBoosterPredict(cfier, dmat, 0, 0, &len, &out);
            XGDMatrixFree(dmat);
            BOOST_VERIFY(len == labels.size());
            for (size_t i = 0; i < len; ++i) {
                tags->emplace_back("OBJECT",
                        labels[i],
                        lexical_cast<string>(out[i]));
            }
        }
    };

    Tagger *create_image_tagger (Config const &config) {
        return new ImageTagger(config);
    }
}
