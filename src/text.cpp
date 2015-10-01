#include <sstream>
#include <mitie/named_entity_extractor.h>
#include <mitie/conll_tokenizer.h>
#include <dlib/time_this.h>
#include <dlib/cmd_line_parser.h>
#include <dlib/serialize.h>
#include "stags.h"

namespace stags {
    vector<string> tokenize (
        const string& line
    )
    {
        std::istringstream sin(line);
        mitie::conll_tokenizer tok(sin);
        vector<string> words;
        string word;
        while(tok(word))
            words.push_back(word);

        return words;
    }

    class TextTagger: public Tagger {
        mitie::named_entity_extractor ner;
    public:
        TextTagger (Config const &config) {
            string model = config.get<string>("stags.text.model", "models/text");
            string classname;
            dlib::deserialize(model) >> classname >> ner;
        }

        virtual string type () const {
            return "text";
        }

        virtual void tag (string const &object, vector<Tag> *tags) {
            const vector<string> tagstr = ner.get_tag_name_strings();
            vector<string> tokens = tokenize(object);
            vector<std::pair<unsigned long, unsigned long>>  chunks;
            vector<unsigned long> chunk_tags;
            vector<double> chunk_scores;
            ner.predict(tokens, chunks, chunk_tags, chunk_scores);
            unsigned n = chunks.size();
            for (unsigned i = 0; i < n; ++i) {
                ostringstream ss;
                for (unsigned j = chunks[i].first; j < chunks[i].second; ++j) {
                    if (j > chunks[i].first) ss << ' ';
                    ss << tokens[j];
                }
                tags->emplace_back(tagstr[chunk_tags[i]], ss.str(), chunk_scores[i]);
            }
        }
    };

    Tagger *create_text_tagger (Config const &config) {
        return new TextTagger(config);
    }
}
