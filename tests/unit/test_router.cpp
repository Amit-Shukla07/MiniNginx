#include "server/Router.hpp"
#include "test_framework.hpp"

using namespace mininginx::server;
using namespace mininginx::http;

TEST(RouterTest, ExactMatchRouting) {
    Router router;
    router.get("/api/health", [](const HttpRequest&) {
        return HttpResponse::json("{\"status\":\"ok\"}");
    });
    router.post("/api/echo", [](const HttpRequest& req) {
        return HttpResponse::ok(req.body(), "text/plain");
    });

    // Test GET /api/health
    HttpRequest req1;
    req1.set_method(HttpMethod::GET);
    req1.set_uri("/api/health");
    HttpResponse res1 = router.route(req1);
    ASSERT_EQ(res1.status(), HttpStatus::OK);
    ASSERT_EQ(res1.body(), "{\"status\":\"ok\"}");

    // Test POST /api/echo
    HttpRequest req2;
    req2.set_method(HttpMethod::POST);
    req2.set_uri("/api/echo");
    req2.set_body("ping");
    HttpResponse res2 = router.route(req2);
    ASSERT_EQ(res2.status(), HttpStatus::OK);
    ASSERT_EQ(res2.body(), "ping");

    // Test 404 for unknown route
    HttpRequest req3;
    req3.set_method(HttpMethod::GET);
    req3.set_uri("/api/nonexistent");
    HttpResponse res3 = router.route(req3);
    ASSERT_EQ(res3.status(), HttpStatus::NotFound);
}

TEST(RouterTest, FallbackHandler) {
    Router router;
    router.set_fallback_handler([](const HttpRequest& req) {
        return HttpResponse::ok("Fallback for: " + req.path());
    });

    HttpRequest req;
    req.set_method(HttpMethod::GET);
    req.set_uri("/some/arbitrary/path.html");
    HttpResponse res = router.route(req);
    ASSERT_EQ(res.status(), HttpStatus::OK);
    ASSERT_EQ(res.body(), "Fallback for: /some/arbitrary/path.html");
}
