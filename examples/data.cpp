/*
 * Copyright 2014 Grigory Demchenko (aka gridem)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <memory>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

#include "data.h"
#include "channel.h"
#include "mt.h"
#include "helpers.h"
#include "network.h"

#define EOL                     "\r\n"

using namespace synca;
using namespace synca::net;
using namespace synca::data;
using namespace mt;
using namespace boost;

typedef std::string Str;
typedef std::pair<Str, Str> StrPair;
typedef Channel<Str> ChanStr;
typedef Channel<StrPair> ChanStrPair;
typedef Channel<int> ChanInt;

template<typename T>
Str toStr(const T& t)
{
    return Str(t.first, t.second);
}

// needed std namespace due to
// argument-dependent lookup issue in GCC compiler
namespace std {

bool isEmpty(const Str& s)
{
    return s.empty();
}

bool isEmpty(const StrPair& s)
{
    return isEmpty(s.first);
}

}

StrPair parseUrl(const Str& url)
{
    static const regex e("http://([\\w\\d\\._-]*[\\w\\d_-]+)(.*)", regex::icase);
    smatch what;
    bool result = regex_search(url, what, e);
    if (!result)
    {
        JLOG("cannot parse url: " << url);
        return {};
    }
    Str host = what[1];
    Str path = what[2];
    if (path.empty())
        path = "/";
    return {host, path};
}

void parseHref(const StrPair& data, ChanStr& c)
{
    static const regex e("href *= *\"([http://[\\w\\d\\._-]*[\\w\\d_-]+]?/[\\?\\&\\d\\w\\[\\]\\@\\!\\$\\'\\(\\)\\*\\+\\.%,;:/#=~_-]*)\"", regex::icase);
    auto&& host = data.first;
    auto&& body = data.second;

    sregex_token_iterator i = make_regex_token_iterator(body, e, 1);
    sregex_token_iterator ie;
    while (i != ie)
    {
        Str url = *i++;
        VERIFY(!url.empty(), "Empty parsed url");
        if (url[0] == '/')
            url = "http://" + host + url;
        c.put(url);
    }
}

void parseText(const Str& content, ChanStr& para)
{
    static const regex e("<p>(.*?)</p>");
    sregex_token_iterator i = make_regex_token_iterator(content, e, 1);
    sregex_token_iterator ie;
    while (i != ie)
    {
        para.put(*i++);
    }
}

void excludeTags(const Str& para, ChanStr& text)
{
    static const regex e("(<.*?>)");
    sregex_token_iterator i = make_regex_token_iterator(para, e, 1);
    sregex_token_iterator ie;
    Str::const_iterator s = para.begin();
    for (; i != ie; ++ i)
    {
        text.put({s, i->first});
        s = i->second;
    }
    text.put({s, para.end()});
}

void splitWords(const Str& text, ChanStr& words)
{
    static const regex e("([a-zA-Z]+)");
    sregex_token_iterator i = make_regex_token_iterator(text, e, 1);
    sregex_token_iterator ie;
    while (i != ie)
        words.put(*i++);
}

struct UrlFilter
{
    explicit UrlFilter(const Str& filter_, size_t maxUrls_) : filter(filter_), maxUrls(maxUrls_) {}
    
    Str operator()(const Str& url) const
    {
        if (url.find(filter) == std::string::npos)
            return {};
        if (processed.size() >= maxUrls)
            return {};
        if (processed.count(url))
            return {};
        processed.emplace(url);
        JLOG("added url " << processed.size() << ": " << url);
        return url;
    }
private:
    Str filter;
    size_t maxUrls;

    mutable std::unordered_set<Str> processed;
};

StrPair loadContent(const StrPair& url)
{
    const int MAX_BUF_SIZE = 1024*1000;
    const int READ_BUF_SIZE = 1024*16;

    auto&& host = url.first;
    auto&& path = url.second;
    JLOG("loading url: " << host << ", " << path);
    Resolver r;
    Resolver::EndPoints ends = r.resolve(host, 80);
    VERIFY(ends != Resolver::EndPoints(), "Cannot resolve hostname: " + host);
    Socket s;
    s.connect(*ends);
    Str req =
        "GET " + path + " HTTP/1.1" EOL
        "Host: " + host + EOL EOL;
    s.write(req);
    Str resp;
    Str buf;
    size_t header = std::string::npos;
    while (header == std::string::npos)
    {
        VERIFY(resp.size() < MAX_BUF_SIZE, "Header is too large");
        buf.resize(READ_BUF_SIZE);
        s.partialRead(buf);
        VERIFY(!buf.empty(), "Empty buffer on partial read");
        resp += buf;
        header = resp.find(EOL EOL);
    }
    Str head = resp.substr(0, header);
    header += 4; // add 2 EOLs
    static const regex eStatus("^http/1\\.[01] ([\\d]{3})", regex::icase);
    smatch whatStatus;
    bool result = regex_search(head, whatStatus, eStatus);
    VERIFY(result, "Invalid http status header");
    Str status = whatStatus[1];
    static const std::unordered_set<Str> allowedStatuses = {"200", "301", "302", "303"};
    VERIFY(allowedStatuses.count(status), "Unexpected status: " + status);
    static const regex e("content-length: *(\\d+)", regex::icase);
    smatch what;
    result = regex_search(head, what, e);
    if (result)
    {
        int len = std::atoi(Str(what[1]).c_str());
        VERIFY(len > 0 && len < MAX_BUF_SIZE, "Content length: invalid value");
        buf.resize(len - (resp.size() - header));
        s.read(buf);
        resp += buf;
    }
    else
    {
        static const regex et("transfer-encoding: *chunked", regex::icase);
        // read chunks
        result = regex_search(head, et);
        VERIFY(result, "Either content-length or transfer-encoding must be present in header");
        while (resp.find(EOL EOL, resp.size() - 4) == std::string::npos)
        {
            VERIFY(resp.size() < MAX_BUF_SIZE, "Response is too large");
            buf.resize(READ_BUF_SIZE);
            s.partialRead(buf);
            VERIFY(!buf.empty(), "Empty buffer on partial read");
            resp += buf;
        }
    }
    Str body = resp.substr(header);
    return {host, body};
}

void processing()
{
    int threads = std::thread::hardware_concurrency();

    ThreadPool tp(threads, "tp");
    scheduler<DefaultTag>().attach(tp);
    service<NetworkTag>().attach(tp);

    ChanStr url;
    ChanStr filteredUrl;
    ChanStrPair parsedUrl;
    ChanStrPair content;
    ChanStrPair contentHref;
    ChanStr contentText;
    ChanStr contentParagraph;
    ChanStr text;
    ChanStr words;
    
    UrlFilter urlFilter("boost.org", 1000);
    
    piping1to01(url, filteredUrl, urlFilter);
    piping1to01(filteredUrl, parsedUrl, parseUrl);
    piping1to1(parsedUrl, content, loadContent, 50);
    go([&] {
        auto c1 = closer(contentHref);
        auto c2 = closer(contentText);
        for (auto&& c: content)
        {
            contentHref.put(c);
            contentText.put(c.second);
        }
    });
    piping1toMany(contentHref, url, parseHref);
    piping1toMany(contentText, contentParagraph, parseText);
    piping1toMany(contentParagraph, text, excludeTags);
    piping1toMany(text, words, splitWords);
    
    std::unordered_map<Str, int> countedWords;
    
    go([&] {
        for (auto&& c: words)
        {
            boost::algorithm::to_lower(c);
            ++ countedWords[c];
        }
    });
    
    url.put("http://www.boost.org");
    closeAndWait(tp, url);
    static const size_t WORDS_COUNT = 20;
    std::vector<std::pair<Str, int>> wordsOut;
    typedef std::vector<std::pair<Str, int>>::const_reference CRef;
    for (auto&& i: countedWords)
        wordsOut.push_back(i);
    RTLOG("added");
    auto nth = std::min(WORDS_COUNT, wordsOut.size());
    auto sortFn = [](CRef l, CRef r) { return l.second > r.second; };
    std::nth_element(wordsOut.begin(), wordsOut.begin() + nth, wordsOut.end(), sortFn);
    wordsOut.resize(nth);
    std::sort(wordsOut.begin(), wordsOut.end(), sortFn);
    RTLOG("sorted");
    for (auto&& w: wordsOut)
    {
        RTLOG("words: " << w.first << ":" << w.second);
    }
}

int main(int argc, char* argv[])
{
    try
    {
        processing();
    }
    catch (std::exception& e)
    {
        RLOG("Error: " << e.what());
        return 1;
    }
    catch (...)
    {
        RLOG("Unknown error");
        return 2;
    }
    RLOG("main ended");
    return 0;
}
