#ifndef __bs_search_engine_h__
#define __bs_search_engine_h__

#include <algorithm>
#include <boost_search/search/search_index.h>
#include <boost_search/include/cppjieba/Jieba.hpp>
#include <boost_search/base/log.h>
#include <jsoncpp/json/json.h>

namespace bs_search_engine
{
    using namespace bs_log_system;

    // 搜索节点结构
    struct SearchIndexElement
    {
        uint64_t id;
        std::vector<std::string> words;
        int weight;

        SearchIndexElement()
            :id(0), weight(0)
        {}
    };

    class SearchEngine
    {
    public:
        SearchEngine()
            : search_index_(bs_search_index::SearchIndex::getSearchIndexInstance())
        {
            // 构建索引
            search_index_->buildIndex();
        }

        // 根据关键字进行搜索
        void search(std::string &keyword, std::string &json_string)
        {
            // 对用户输入的关键字进行切分

            std::vector<std::string> keywords;
            jieba_.CutForSearch(keyword, keywords);

            std::vector<SearchIndexElement> results;
            std::unordered_map<uint64_t, SearchIndexElement> select_map;
            for (auto &word : keywords)
            {
                // 忽略大小写
                boost::to_lower(word);
                // 查倒排索引
                std::vector<bs_search_index::BackwardIndexElement> *ret_ptr = search_index_->getBackwardIndexElement(word);
                if (!ret_ptr)
                    continue;
                // 插入结果
                for (auto &bi : *ret_ptr)
                {
                    // 如果文档ID已经存在，说明已经存在，否则不存在
                    if (select_map.find(bi.id) == select_map.end())
                    {
                        // 获取当前文档搜索结构节点，不存在自动插入，存在直接获取
                        auto &el = select_map[bi.id];
                        // 如果是新节点，直接赋值；如果是重复出现的节点，覆盖
                        el.id = bi.id;
                        // 如果是新节点，第一次添加；如果是重复节点，追加
                        el.words.push_back(bi.word);
                        // 如果是新节点，直接赋值；如果是重复节点，累加
                        el.weight += bi.weight;
                    }
                }
            }

            // 遍历select_map存储结果
            for (auto &pair : select_map)
                results.push_back(std::move(pair.second));

            std::stable_sort(results.begin(), results.end(), [](const SearchIndexElement &b1, const SearchIndexElement &b2)
                             { return b1.weight > b2.weight; });

            // 转换为JSON字符串
            Json::Value root;
            for (auto &el : results)
            {
                // 通过正排索引获取文章内容
                bs_search_index::SelectedDocInfo *sd = search_index_->getForwardIndexDocInfo(el.id);

                Json::Value item;
                item["title"] = sd->rd.title;
                item["body"] = getPartialBodyWithKeyword(sd->rd.body, el.words[0]);
                item["url"] = sd->rd.url;

                // 将item作为一个JSON对象插入到root中作为子JSON对象
                root.append(item);
            }

            Json::FastWriter writer;
            json_string = writer.write(root);
        }

        ~SearchEngine()
        {
        }

    private:
        static const int prev_words = 50;
        static const int after_words = 100;
        std::string getPartialBodyWithKeyword(std::string_view body, std::string_view keyword)
        {
            // 找到关键字
            // size_t pos = body.find(keyword);
            auto pos_t = std::search(body.begin(), body.end(), keyword.begin(), keyword.end(), [](char c1, char c2)
            { 
                return std::tolower(c1) == std::tolower(c2); 
            });

            if (pos_t == body.end())
            {
                LOG(Level::Warning, "无法找到关键字，无法截取文章内容");
                return "Fail to cut body, can't find keyword";
            }

            int pos = std::distance(body.begin(), pos_t);

            // 默认起始位置为0，终止位置为body字符串最后一个字符
            int start = 0;
            int end = static_cast<int>(body.size() - 1);

            // 如果pos位置前有50个字符，就取前50个字符
            if (static_cast<int>(pos) - prev_words > start)
                start = pos - prev_words;
            // 如果pos位置后有100个字符，就取后100个字符
            if (pos + static_cast<int>(keyword.size()) + after_words < end)
                end = pos + static_cast<int>(keyword.size()) + after_words;

            if (start > end)
            {
                LOG(Level::Warning, "内容不足，无法截取文章内容");
                return "Fail to cut body, body is not enough";
            }

            // 左闭右闭区间
            return std::string(body.substr(start, end - start + 1));
        }

    private:
        bs_search_index::SearchIndex *search_index_;
        cppjieba::Jieba jieba_;
    };
}

#endif