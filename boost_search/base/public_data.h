#ifndef __bs_public_data_h__
#define __bs_public_data_h__

#include <string>
#include <filesystem>

namespace bs_public_data
{
    namespace fs = std::filesystem;

    // 文本文件路径
    const fs::path g_rawfile_path = "/home/epsda/BoostSearchingEngine_ReactorServer/boost_search/data/raw";
    // 结构体字段间的分隔符
    const std::string g_rd_sep = "\3";
    // 不同HTML文件的分隔符
    const std::string g_html_sep = "\n";
    // 网页根路径
    const std::string root_path = "/home/epsda/BoostSearchingEngine_ReactorServer/boost_search/wwwroot";

    // 结果基本内容结构
    struct ResultData
    {
        std::string title; // 结构标题
        std::string body;  // 结果内容或描述
        std::string url;   // 网址
    };
}
#endif