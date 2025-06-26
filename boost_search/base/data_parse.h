#ifndef __bs_data_parse_h__
#define __bs_data_parse_h__

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <vector>
#include <boost_search/base/log.h>
#include <boost_search/base/public_data.h>
#include <boost_search/utils/file_op.h>

namespace bs_data_parse
{
    using namespace bs_log_system;
    
    // HTML源文件路径
    const std::filesystem::path g_datasource_path = "data/source/html";
    // HTML文件后缀
    const std::string g_html_extension = ".html";
    // 用于拼接的官网URL
    const std::string g_url_to_concat = "https://www.boost.org/doc/libs/1_78_0/doc/html";

    // 内容状态
    enum ContentStatus
    {
        Label,          // 标签状态
        OrdinaryContent // 普通文本状态
    };

    class DataParse
    {
    public:
        // 获取HTML文件路径函数
        bool getHtmlSourceFiles()
        {
            // 如果路径不存在，直接返回false
            if (!std::filesystem::exists(g_datasource_path))
            {
                LOG(Level::Warning, "不存在指定路径");
                return false;
            }

            // 递归目录结束位置，相当于空指针nullptr
            for (const auto &entry : std::filesystem::recursive_directory_iterator(g_datasource_path))
            {
                // 1. 不是普通文件就是目录，继续遍历
                if (!std::filesystem::is_regular_file(entry))
                    continue;

                // 2. 是普通文件，但是不是HTML文件，继续遍历
                if (entry.path().extension() != g_html_extension)
                    continue;

                // 3. 是普通文件并且是HTML文件，插入结果
                sources_.push_back(std::move(entry.path()));
            }

            // 空结果返回false
            return !sources_.empty();
        }

        // 读取HTML文件
        bool readHtmlFile(const std::filesystem::path &p, std::string &out)
        {
            if (p.empty())
                return false;

            bs_file_op::FileOp::readFile(p, out);

            return true;
        }

        // 读取标题
        // &为输入型参数，*为输出型参数
        bool getTitleFromHtml(std::string &in, std::string *title)
        {
            if (in.empty())
                return false;

            // 找到开始标签<title>
            auto start = in.find("<title>");
            if (start == std::string::npos)
                return false;

            // 找到终止标签</title>
            auto end = in.find("</title>");
            if (end == std::string::npos || start > end)
                return false;

            // 截取出其中的内容，左闭右开
            *title = in.substr(start + std::string("<title>").size(), end - (start + std::string("<title>").size()));

            return true;
        }

        // 读取HTML文件内容
        bool getContentFromHtml(std::string &out, std::string *body)
        {
            // 默认状态为标签
            ContentStatus cs = ContentStatus::Label;

            // 注意，因为文档没有中文，直接用char没有问题
            for (char ch : out)
            {
                switch (cs)
                {
                // 读取到右尖括号且状态为标签说明接下来为文本内容
                case ContentStatus::Label:
                    if (ch == '>')
                        cs = ContentStatus::OrdinaryContent; // 切换状态
                    break;
                case ContentStatus::OrdinaryContent:
                    // 去除\n
                    if (ch == '<')
                        cs = ContentStatus::Label; // 切换状态
                    else
                    {
                        if (ch == '\n')
                            *body += ' ';
                        else
                            *body += ch;
                    }
                    break;
                default:
                    break;
                }
            }

            return true;
        }

        // 构建URL
        bool constructHtmlUrl(const std::filesystem::path &p, std::string *url)
        {
            // 查找/data/source/html
            std::string t_path = p.string();
            auto pos = t_path.find(g_datasource_path);

            if (pos == std::string::npos)
                return false;

            std::string source_path = t_path.substr(pos + g_datasource_path.string().size());
            *url = g_url_to_concat + source_path;

            return true;
        }

        // 从HTML文件中读取信息
        bool readInfoFromHtml()
        {
            // 不存在路径返回false
            if (sources_.empty())
            {
                LOG(Level::Warning, "文件路径不存在");
                return false;
            }

            for (const auto &path : sources_)
            {
                std::string out; // 存储HTML文件内容
                struct bs_public_data::ResultData rd;
                // 1. 读取文件
                if (!readHtmlFile(path, out))
                {
                    LOG(Level::Warning, "打开文件：{}失败", path.string());
                    // 读取当前文件失败时继续读取后面的文件
                    continue;
                }

                // 2. 获取标题
                if (!getTitleFromHtml(out, &rd.title))
                {
                    LOG(Level::Warning, "获取文件标题失败");
                    continue;
                }

                // 3. 读取文件内容
                if (!getContentFromHtml(out, &rd.body))
                {
                    LOG(Level::Warning, "获取文件内容失败");
                    continue;
                }

                // 4. 构建URL
                if (!constructHtmlUrl(path, &rd.url))
                {
                    LOG(Level::Warning, "构建URL失败");
                    continue;
                }

                results_.push_back(std::move(rd));
            }

            return true;
        }

        // 将结构体字段写入文本文件中
        bool writeToRawFile()
        {
            // 以二进制形式打开文件
            std::fstream f(bs_public_data::g_rawfile_path);

            if (!f.is_open())
            {
                LOG(Level::Warning, "文件文件不存在");
                return false;
            }

            // 写入结构化数据
            for (auto &rd : results_)
            {
                std::string temp;
                temp += rd.title;
                temp += bs_public_data::g_rd_sep;
                temp += rd.body;
                temp += bs_public_data::g_rd_sep;
                temp += rd.url;
                temp += bs_public_data::g_html_sep;

                f.write(temp.c_str(), temp.size());
            }

            f.close();

            return true;
        }

    private:
        std::vector<std::filesystem::path> sources_;
        std::vector<bs_public_data::ResultData> results_;
    };
}

#endif