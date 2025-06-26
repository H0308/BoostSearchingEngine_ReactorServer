#include <boost_search/base/data_parse.h>

using namespace bs_data_parse;

int main()
{
    DataParse d;
    d.getHtmlSourceFiles();
    d.readInfoFromHtml();
    d.writeToRawFile();

    return 0;
}