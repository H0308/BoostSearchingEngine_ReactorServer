#include <boost_search/search/search_engine.h>
#include <boost_search/net/http/http_server.h>

using namespace bs_log_system;

void run(bs_search_engine::SearchEngine& s_engine, bs_http_request::HttpRequest& req, bs_http_response::HttpResponse &resp)
{
    // 如果不存在word，说明在请求不存在的页面，返回404
    if(!req.isInParams("keyword") || req.getParam("keyword").empty())
    {
        LOG(Level::Info, "请求不存在的资源");
        resp.setStatus(404);
        std::string file;
        bs_file_op::FileOp::readFile(bs_public_data::root_path + "/" + "404.html", file);
        resp.setBody(file);
        return;
    }

    // 此时说明存在对应的值，获取值
    auto val = req.getParam("keyword");

    // 执行搜索
    std::string json_string;
    s_engine.search(val, json_string);

    LOG(Level::Info, "搜索关键词: {}", val);
    resp.setBody(json_string, "application/json");
}

int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        LOG(Level::Error, "启动方式错误");
        return 1;
    }

    // 设置网页根路径
    bs_http_server::HttpServer server(std::stoi(argv[1]));
    server.setBaseDir(bs_public_data::root_path);    
    bs_search_engine::SearchEngine s_engine;

    server.setGetHandler("/search", std::bind(run, std::ref(s_engine), std::placeholders::_1, std::placeholders::_2));

    int port = std::stoi(argv[1]);

    server.startServer();

    return 0;
}