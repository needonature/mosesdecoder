#include <unordered_map>
#include "quering.hh"
#include "util/exception.hh"
#include "../Util2.h"

namespace Moses2
{

QueryEngine::QueryEngine(const char * filepath)
{

  //Create filepaths
  std::string basepath(filepath);
  std::string path_to_hashtable = basepath + "/probing_hash.dat";
  std::string path_to_source_vocabid = basepath + "/source_vocabids";

  ///Source phrase vocabids
  read_map(source_vocabids, path_to_source_vocabid.c_str());

  //Read config file
  std::unordered_map<std::string, std::string> keyValue;

  std::ifstream config((basepath + "/config").c_str());
  std::string line;
  while (getline(config, line)) {
    std::vector<std::string> toks = Moses2::Tokenize(line, "\t");
    UTIL_THROW_IF2(toks.size() != 2, "Wrong config format:" << line);
    keyValue[ toks[0] ] = toks[1];
  }

  bool found;
  //Check API version:
  int version;
  found = Get(keyValue, "API_VERSION", version);
  if (!found || version != API_VERSION) {
    std::cerr << "The ProbingPT API has changed. " << version << "!="
        << API_VERSION << " Please rebinarize your phrase tables." << std::endl;
    exit(EXIT_FAILURE);
  }

  //Get tablesize.
  int tablesize;
  found = Get(keyValue, "uniq_entries", tablesize);
  if (!found) {
    std::cerr << "uniq_entries not found" << std::endl;
    exit(EXIT_FAILURE);
  }

  //Number of scores
  found = Get(keyValue, "num_scores", num_scores);
  if (!found) {
    std::cerr << "num_scores not found" << std::endl;
    exit(EXIT_FAILURE);
  }

  //How may scores from lex reordering models
  found = Get(keyValue, "num_lex_scores", num_lex_scores);
  if (!found) {
    std::cerr << "num_lex_scores not found" << std::endl;
    exit(EXIT_FAILURE);
  }

  // have the scores been log() and FloorScore()?
  found = Get(keyValue, "log_prob", logProb);
  if (!found) {
    std::cerr << "logProb not found" << std::endl;
    exit(EXIT_FAILURE);
  }

  config.close();

  //Read hashtable
  table_filesize = Table::Size(tablesize, 1.2);
  mem = readTable(path_to_hashtable.c_str(), table_filesize);
  Table table_init(mem, table_filesize);
  table = table_init;

  std::cerr << "Initialized successfully! " << std::endl;
}

QueryEngine::~QueryEngine()
{
  //Clear mmap content from memory.
  munmap(mem, table_filesize);

}

uint64_t QueryEngine::getKey(uint64_t source_phrase[], size_t size) const
{
  //TOO SLOW
  //uint64_t key = util::MurmurHashNative(&source_phrase[0], source_phrase.size());
  uint64_t key = 0;
  for (size_t i = 0; i < size; i++) {
    key += (source_phrase[i] << i);
  }
  return key;
}

std::pair<bool, uint64_t> QueryEngine::query(uint64_t key)
{
  std::pair<bool, uint64_t> ret;

  const Entry * entry;
  ret.first = table.Find(key, entry);
  if (ret.first) {
    ret.second = entry->value;
  }
  return ret;
}

}

