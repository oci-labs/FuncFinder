// Copyright (c) 2013 Kevin Heifner.
// Free to use/copy/modify. Please retain copyright.
//
// funcfinder.cpp 

#include <algorithm>
#include <cstddef>
#include <functional>
#include <fstream>
#include <iostream>
#include <regex> // C++11 regular expression library
#include <sstream>
#include <string>
#include <tuple>

#include <map>
std::map<std::string, size_t> countMatches(const std::string& s, const std::vector<std::string>& v) 
{
  std::map<std::string, size_t> result;
  if (v.empty()) return result;
  // create regex expression of the form "\\b((?:first)|(?:second)|(?:etc))\\b"
  std::string regStr;
  for (const auto& i : v) {
    if (regStr.empty()) regStr += "\\b((?:" + i + ")";
    else regStr += "|(?:" + i + ")";
  }
  regStr += ")\\b";
  std::regex reg(regStr);
  // count matches
  std::sregex_iterator begin(s.cbegin(), s.cend(), reg), end;
  std::for_each(begin, end, 
    [&] (const std::smatch& m) {
      ++result[m.str()];
  });
  return result;
}

size_t countMatches(const std::string& s, const std::regex& exp) {
 // iteration of each of the matches of exp in s
 std::sregex_iterator begin(s.cbegin(), s.cend(), exp), end;
 // only interested in number of matches
 return std::distance(begin, end);
}

// Finds regExp in function definitions and places entire function in output stream.
// Stores up to the complete input stream in memory plus overhead if os != nullptr.
// Does not handle multi-line raw strings or preprocessor continuation lines.
// @param is input stream
// @param regExp regular expression to search for within function definitions
// @param os output stream, if nullptr then greatly reduced memory usage.
// @return get<0> is line offset in input stream where regExp found or -1
//         get<1> is line offset in input stream of the first line of the function
//         get<2> is line offset in input stream of last line read
std::tuple<size_t,size_t,size_t> 
  findFunction(std::istream& is, const std::regex& regExp, std::ostream* os = nullptr) 
{
  // "namespace {", "namespace a{", "namespace a{namespace b {", "using namespace std;", 
  // "namespace alias = oldname;"
  // "class a {", "class a : public b {"
  std::regex nsClassExp("\\b(?:(?:namespace)|(?:class))\\b");
  std::regex usingNSClassExp("(?:\\busing\\s+namespace\\b)|"           // using namespace
                             "(?:\\bnamespace\\s*[^\\d\\W]\\w*\\s*=)|" // namespace alias
                             "(?:\\bclass\\s*[^\\d\\W]\\w*\\s*)[;,>]"  // class forward declare, or template
                            ); 
  std::regex lineCommentExp("\\/\\/.*"); // rest of line
  // handling multi-line comments that begin/end on same line separately simplifies logic
  std::regex spanCommentExp("\\/\\*(?:.*?)\\*\\/"); // a multiline comment on one line: /* stuff */
  std::regex begMultiLineCommentExp("(.*?)\\/\\*.*"); // [stuff]/* ...
  std::regex endMultiLineCommentExp(".*?\\*\\/(.*)"); // ... */[stuff]
  std::regex includeExp("^\\s*#include\\b"); // #include 
  // "*" and '*' which allows for embedded quotes: "hi\"there\""
  // This would be more readable if C++11 raw string support was available.
  std::regex quotedStrExp("\"(?:[^\"\\\\]*(?:\\\\.[^\"\\\\]*)*)\"|"      // double quotes with possible escapes
                          "\\'(?:[^\\'\\\\]*(?:\\\\.[^\\'\\\\]*)*)\\'"); // single quotes with possible escapes

  std::vector<std::string> lines;
  std::string s;
  int numOpenBrackets = 0; // int required, using negative numbers to handle namespace/class
  size_t foundLoc = 0, begLineNum = 1, endLineNum = 1;
  bool inMultiLineComment = false;

  // factor out common reset logic into helper function
  std::function<void()> reset = [&]() {
    lines.clear();
    numOpenBrackets = 0;
    begLineNum = endLineNum + 1;
  };

  for (; std::getline(is, s); ++endLineNum) {
    if (os) lines.push_back(s);
    s = std::regex_replace(s, spanCommentExp, " "); // remove span comments /* .. */ on same line
    // handle multi-line comments
    std::smatch m;
    if (!inMultiLineComment && std::regex_search(s, m, begMultiLineCommentExp)) {
      s = m[1].str(); // smatch uses iterators, m invalid after assignment to s
      inMultiLineComment = true;
    } else if (inMultiLineComment && std::regex_search(s, m, endMultiLineCommentExp)) {
      s = m[1].str();
      inMultiLineComment = false;
    }
    if (inMultiLineComment) continue;
    s = std::regex_replace(s, lineCommentExp, ""); // remove single line comments
    s = std::regex_replace(s, quotedStrExp, ""); // remove quoted strings
    // ignore #includes in file, assume they are always at file scope
    if (foundLoc == 0 && std::regex_search(s, includeExp)) { reset(); continue; }
    if (std::regex_search(s, regExp)) foundLoc = endLineNum; // found, print at end of function
    // assumes an open namespace statement not on the same line as a using namespace statement
    if (std::regex_search(s, nsClassExp) && !std::regex_search(s, usingNSClassExp)) {      
      reset(); // reset on namespace/class
      // subtract out namespace/class open brackets so we don't report the entire namespace/class
      numOpenBrackets -= countMatches(s, nsClassExp);
    }
    //numOpenBrackets += std::count(s.cbegin(), s.cend(), '{');      // alternative implementation
    //auto numCloseBrackets = std::count(s.cbegin(), s.cend(), '}'); // instead of the for_each
    decltype(numOpenBrackets) numCloseBrackets = 0;
    std::for_each(s.cbegin(), s.cend(), 
      [&] (char c) { 
        if (c == '{') ++numOpenBrackets;
        if (c == '}') ++numCloseBrackets;
    });

    if (numCloseBrackets > 0) {
      // only clear at '}' so that function name, comments, etc. included in output of find
      numOpenBrackets -= numCloseBrackets;
      if (numOpenBrackets <= 0) {
        if (foundLoc > 0) {
          // pre-C++11: std::copy(lines.begin(), lines.end(), std::ostream_iterator<std::string>(os, "\n"));
          if (os) for (const auto& x : lines) *os << x << '\n';
          return std::make_tuple(foundLoc, begLineNum, endLineNum);
        }
        reset();
      }
    }
  }

  return std::make_tuple(-1, begLineNum, endLineNum);
}

std::tuple<size_t, size_t,size_t> findFunction(std::istream& is, const std::regex& regExp, std::ostream& os) {
  return findFunction(is, regExp, &os);
}

void processFile(const std::string& regExpStr, const std::string& fileName)
{
  std::ifstream ifs(fileName);
  if (!ifs) {
    std::cerr << "Unable to open file: " << fileName << std::endl;
    return;
  }

  // much faster not to store lines as file is processed, verify regex found first
  std::regex grepExp(regExpStr);
  bool found = false;
  for (std::string s; !found && std::getline(ifs, s);) {
    found = std::regex_search(s, grepExp);
  }
  if (!found) return;

  ifs.seekg(0);
  for (size_t total = 0;;) {
    // including findFunction memory usage, using up to 2x size of file in memory plus overhead
    std::stringstream ss;
    auto result = findFunction(ifs, grepExp, ss);
    if (std::get<0>(result) == -1) break;
    std::cout << "== " << fileName << "(" 
      << total + std::get<0>(result) << ") range [" 
      << total + std::get<1>(result) << "," 
      << total + std::get<2>(result) << "] ==\n";
    std::cout << ss.str();
    total += std::get<2>(result);
  }
}

int main(int argc, char* argv[]) 
{
  try {
    if (argc < 3) {
      std::cerr << "Usage: " << argv[0] << " regex source_file ..." << std::endl;
      return 1;
    }

    for (int i = 2; i < argc; ++i) {
      processFile(argv[1], argv[i]);
    }

    return 0;
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << "Unknown exception." << std::endl;
  }
  return 1;
}

