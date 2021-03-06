#include <gtest/gtest.h>
#include <http/http_head.h>
#include <boost/regex.hpp>
using namespace http;

TEST(HTTPHeadTest, simple)
{
    {
        std::string request_head = 
            "POST /test/index.html\tHttP/2.0\r\n"
            "ACCEPT: ac\r\n"
            "Content-lengTH\t:32\r\n"
            "HOst \t:\twww.baidu.com\r\n"
            "User:http_test\r\n\r\n";
        http_head head;
        std::size_t n = head.parse(request_head);
        EXPECT_EQ(n, request_head.length());
        EXPECT_EQ(head.type(), http_head::eHttpHeadType::request);

        request_head += "\r\n";
        n = head.parse(request_head);
        EXPECT_EQ(n, request_head.length() - 2);
        EXPECT_EQ(head.type(), http_head::eHttpHeadType::request);

        EXPECT_EQ(head.method(), http_head::eMethod::Post);
        EXPECT_EQ(head.path(), "/test/index.html");
        EXPECT_EQ(head.version_major(), 2);
        EXPECT_EQ(head.version_minor(), 0);
        EXPECT_EQ(head.content_length(), 32);
        EXPECT_EQ(head.get("host"), "www.baidu.com");
        printf("%s\n", head.to_string().c_str());
    }

    {
        std::string response_head = 
            "STatus\t200 \tHttP/2.0\r\n"
            "ACCEPT: ac\r\n"
            "Content-lengTH\t:32\r\n"
            "HOst \t:\twww.baidu.com\r\n"
            "User : \thttp_test\r\n";
        http_head head;
        std::size_t n = head.parse(response_head);
        EXPECT_EQ(n, -1);
        EXPECT_EQ(head.type(), http_head::eHttpHeadType::indeterminate);

        response_head += "\r\n";
        n = head.parse(response_head);
        EXPECT_EQ(n, response_head.length());
        EXPECT_EQ(head.type(), http_head::eHttpHeadType::response);

        EXPECT_EQ(head.status(), 200);
        EXPECT_EQ(head.version_major(), 2);
        EXPECT_EQ(head.version_minor(), 0);
        EXPECT_EQ(head.content_length(), 32);
        EXPECT_EQ(head.get("host"), "www.baidu.com");
        printf("%s\n", head.to_string().c_str());
    }
}

TEST(HTTPHeadTest, benchmark)
{
    std::string request_head = 
        "POST /test/index.html\tHttP/2.0\r\n"
        "ACCEPT: ac\r\n"
        "Content-lengTH\t:32\r\n"
        "HOst \t:\twww.baidu.com\r\n"
        "User:http_test\r\n\r\n";

    for (int i = 0; i < 1000 * 100; ++i)
    {
        http_head head;
        std::size_t n = head.parse(request_head);
        (void)n;
//        EXPECT_EQ(n, request_head.length());
    }
}

TEST(HTTPHeadTest, regex_bm)
{
    std::string request_head = 
        "POST /test/index.html\tHttP/2.0\r\n"
        "ACCEPT: ac\r\n"
        "Content-lengTH\t:32\r\n"
        "HOst \t:\twww.baidu.com\r\n"
        "User:http_test\r\n\r\n";

    boost::regex re("([^\\s]+)\\s+([^\\s]+)\\s+\\w+/(\\d+).(\\d+)\r\n(([\\d\\w-]+)\\s*:\\s*([\\d\\w\\.]+)\r\n)*\r\n");
    boost::smatch result;
    EXPECT_TRUE((boost::regex_match(request_head, result, re)));
    for (int i = 0; i < (int)result.size(); ++i)
    {
        printf("[%d] %s\n", i, result[i].str().c_str());
    }

    for (int i = 0; i < 1000 * 100; ++i)
    {
        http_head head;
        boost::smatch result;
        boost::regex_match(request_head, result, re);
        head.set_method_s(result[1].str());
        head.set_path(result[2].str());
        head.set_version_major(atoi(result[3].str().c_str()));
        head.set_version_minor(atoi(result[4].str().c_str()));
        head.fields().insert(http_head::ICaseMap::value_type("ACCEPT", "ac"));
        head.fields().insert(http_head::ICaseMap::value_type("Content-Length", "32"));
        head.fields().insert(http_head::ICaseMap::value_type("Host", "www.baidu.com"));
        head.fields().insert(http_head::ICaseMap::value_type("User", "http_test"));
    }
}
