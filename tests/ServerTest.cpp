#include "Router.hpp"

#include <cassert>
#include <iostream>

void test_router_static_files() {
    Router router("./public");

    // Test GET /
    HttpRequest req1;
    req1.method = HttpMethod::GET;
    req1.path = "/";
    HttpResponse res1 = router.handle(req1);
    assert(res1.status() == HttpStatus::OK);
    assert(res1.body().find("MiniNginx") != std::string::npos);

    // Test GET /style.css
    HttpRequest req2;
    req2.method = HttpMethod::GET;
    req2.path = "/style.css";
    HttpResponse res2 = router.handle(req2);
    assert(res2.status() == HttpStatus::OK);

    // Test HEAD /
    HttpRequest req3;
    req3.method = HttpMethod::HEAD;
    req3.path = "/";
    HttpResponse res3 = router.handle(req3);
    assert(res3.status() == HttpStatus::OK);
    assert(res3.body().empty());  // HEAD must not have body

    // Test 404 Not Found
    HttpRequest req4;
    req4.method = HttpMethod::GET;
    req4.path = "/does_not_exist.html";
    HttpResponse res4 = router.handle(req4);
    assert(res4.status() == HttpStatus::NotFound);

    // Test Path Traversal Protection
    HttpRequest req5;
    req5.method = HttpMethod::GET;
    req5.path = "/../../../../etc/passwd";
    HttpResponse res5 = router.handle(req5);
    assert(res5.status() == HttpStatus::NotFound);
}

int main() {
    test_router_static_files();
    std::cout << "[PASS] All Server/Router tests passed.\n";
    return 0;
}
