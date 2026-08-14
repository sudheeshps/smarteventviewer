#include "System/Console.h"
#include "System/SmartPointer.h"
#include "System/Func.h"
#include "System/Convert.h"
#include "WebAppCore/Builder/WebApplication.h"
#include "WebAppCore/Builder/WebApplicationBuilder.h"
#include "System/Net/Http/HttpClient.h"
#include "System/Net/Http/RestClient.h"
#include "System/Net/Http/HttpRequestException.h"
#include "System/Threading/Thread.h"
#include "WebAppCore/Controllers/ControllerBase.h"
#include "WebAppCore/Controllers/ControllerRouteBuilder.h"
#include "System/Text/Json/JsonSerializer.h"
#include "System/Text/StringBuilder.h"
#include "System/IdentityModel/Tokens/Jwt/JWTToken.h"
#include "Demos.h"

using namespace DotNetDupe::System;
using namespace DotNetDupe::System::Net::Http;
using namespace DotNetDupe::System::Threading;
using namespace DotNetDupe::WebAppCore::Builder;
using namespace DotNetDupe::WebAppCore::Http;
using namespace DotNetDupe::WebAppCore::Controllers;
using namespace DotNetDupe::System::Text::Json;
using namespace DotNetDupe::System::Text;

namespace {
    class IInfoService : public virtual Object {
    public:
        virtual ~IInfoService() = default;
        virtual String GetInfo() = 0;
    };

    class InfoService : public IInfoService {
    public:
        String GetInfo() override {
            return "DotNetDupe Web Host v1.0";
        }
    };

    struct DemoProduct {
        String Name;
        int Price = 0;
    };

    class DemoProductsController : public ControllerBase {
    public:
        DemoProductsController() = default;
        ~DemoProductsController() override = default;

        // Returns strongly typed List directly, utilizing automatic JSON serialization
        Collections::Generic::List<DemoProduct> GetProducts() {
            Collections::Generic::List<DemoProduct> list;
            list.Add(DemoProduct{"Coffee Maker", 85});
            list.Add(DemoProduct{"Toaster", 45});
            return list;
        }

        // Returns strongly typed single DemoProduct directly (automatic JSON serialization)
        DemoProduct GetDefaultProduct() {
            return DemoProduct{"Coffee Maker", 85};
        }

        // Returns String using Ok() / NotFound() helpers for conditional logic
        String GetProductById(const String& id) {
            if (id == "1") {
                return Ok(DemoProduct{"Coffee Maker", 85});
            }
            return NotFound("Product not found");
        }

        // Accepts strongly typed payload and returns String via Created() helper
        String CreateProduct(const DemoProduct& product) {
            return Created(String("Created ") + product.Name + " at price $" + Convert::ToString(product.Price));
        }
    };
}

namespace DotNetDupe {
    namespace System {
        namespace Text {
            namespace Json {
                template <>
                struct JsonConverter<DemoProduct> {
                    static JsonElement Write(const DemoProduct& value) {
                        JsonElement obj(JsonValueKind::Object);
                        JsonElement nameVal(value.Name);
                        JsonElement priceVal(static_cast<double>(value.Price));
                        obj.SetProperty("name", nameVal);
                        obj.SetProperty("price", priceVal);
                        return obj;
                    }

                    static DemoProduct Read(const JsonElement& element) {
                        DemoProduct p;
                        JsonElement prop;
                        if (element.TryGetProperty("name", prop)) p.Name = prop.GetString();
                        if (element.TryGetProperty("price", prop)) p.Price = prop.GetInt32();
                        return p;
                    }
                };
            }
        }
    }
}

void DemonstrateWebApplication() {
    Console::WriteLine("\n=== Web Application Demonstration ===");

    // 1. Create Builder & Register Service
    auto builder = WebApplication::CreateBuilder();
    builder->GetServices().AddSingleton<IInfoService, InfoService>();
    builder->AddController<DemoProductsController>("/api/products")
        .MapGet("", &DemoProductsController::GetProducts)
        .MapGet("/default", &DemoProductsController::GetDefaultProduct)
        .MapGet("/{id}", &DemoProductsController::GetProductById)
        .MapPost("", &DemoProductsController::CreateProduct);

    // 2. Build the App
    auto app = builder->Build();

    // 3. Define endpoints (Minimal APIs)
    app->MapGet("/", [](SmartPointer<HttpContext> context) {
        return String("Welcome to the WebApplication Minimal API Endpoint!");
    });

    app->MapGet("/info", [app](SmartPointer<HttpContext> context) {
        auto infoSvc = app->GetServices()->GetRequiredService<IInfoService>();
        return infoSvc->GetInfo();
    });

    // Dynamic Route Parameter demonstration
    app->MapGet("/api/users/{id}", [](SmartPointer<HttpContext> context) {
        String id;
        if (context->GetRequest()->GetRouteValues().TryGetValue("id", id)) {
            return String("Fetched User Profile for ID: ") + id;
        }
        return String("User ID not provided");
    });

    app->MapPut("/api/users/{id}", [](SmartPointer<HttpContext> context) {
        String id;
        if (context->GetRequest()->GetRouteValues().TryGetValue("id", id)) {
            String body = context->GetRequest()->GetBody();
            context->GetResponse()->SetStatusCode(DotNetDupe::System::Net::HttpStatusCode::OK);
            return String("Successfully updated user ") + id + " with content: " + body;
        }
        context->GetResponse()->SetStatusCode(DotNetDupe::System::Net::HttpStatusCode::BadRequest);
        return String("Bad Request");
    });

    app->MapDelete("/api/users/{id}", [](SmartPointer<HttpContext> context) {
        String id;
        if (context->GetRequest()->GetRouteValues().TryGetValue("id", id)) {
            context->GetResponse()->SetStatusCode(DotNetDupe::System::Net::HttpStatusCode::NoContent);
            return String("");
        }
        context->GetResponse()->SetStatusCode(DotNetDupe::System::Net::HttpStatusCode::BadRequest);
        return String("Bad Request");
    });

    // 3b. JWT Authenticated Minimal API Endpoint
    app->MapGet("/api/secure", [](SmartPointer<HttpContext> context) {
        String authHeader;
        if (!context->GetRequest()->GetHeaders().TryGetValue("authorization", authHeader)) {
            context->GetResponse()->SetStatusCode(DotNetDupe::System::Net::HttpStatusCode::Unauthorized);
            return String("{\"error\":\"Unauthorized - Missing Authorization header\"}");
        }
        if (!authHeader.StartsWith("Bearer ", false)) {
            context->GetResponse()->SetStatusCode(DotNetDupe::System::Net::HttpStatusCode::Unauthorized);
            return String("{\"error\":\"Unauthorized - Invalid scheme\"}");
        }
        String tokenStr = authHeader.Substring(7);
        try {
            auto jwt = DotNetDupe::System::IdentityModel::Tokens::Jwt::JWTToken::Parse(tokenStr);
            if (jwt.IsNull() || !jwt->Verify("demo-secret-key-999")) {
                context->GetResponse()->SetStatusCode(DotNetDupe::System::Net::HttpStatusCode::Unauthorized);
                return String("{\"error\":\"Unauthorized - Invalid signature\"}");
            }
            String user = jwt->GetPayload()["sub"];
            return String("Authorized Access granted for ") + user;
        } catch (...) {
            context->GetResponse()->SetStatusCode(DotNetDupe::System::Net::HttpStatusCode::Unauthorized);
            return String("{\"error\":\"Unauthorized - Parsing failed\"}");
        }
    });

    // 3c. Setup Web API Controllers (Option B ControllerBase + RouteBuilder)
    app->MapControllers();

    // 4. Start the server asynchronously in a background Thread
    Console::WriteLine("Starting WebApplication on http://127.0.0.1:19099...");
    Thread serverThread([app]() {
        app->Run("http://127.0.0.1:19099");
    });
    serverThread.Start();

    // Give the server thread a moment to start and bind
    Thread::Sleep(200);

    // 5. Use HttpClient to send requests to our WebApplication
    try {
        HttpClient client;

        Console::WriteLine("\n[Client] Sending GET request to '/'...");
        auto resp1 = client.Get("http://127.0.0.1:19099/");
        Console::Write("[Client] Response Status: ");
        Console::WriteLine((int)resp1->GetStatusCode());
        Console::Write("[Client] Response Body: '");
        Console::Write(resp1->GetContent()->ReadAsString());
        Console::WriteLine("'");

        Console::WriteLine("\n[Client] Sending GET request to '/info'...");
        auto resp2 = client.Get("http://127.0.0.1:19099/info");
        Console::Write("[Client] Response Status: ");
        Console::WriteLine((int)resp2->GetStatusCode());
        Console::Write("[Client] Response Body: '");
        Console::Write(resp2->GetContent()->ReadAsString());
        Console::WriteLine("'");

        Console::WriteLine("\n[Client] Sending GET request to '/api/users/12345' (Dynamic Path Parameter)...");
        auto resp3 = client.Get("http://127.0.0.1:19099/api/users/12345");
        Console::Write("[Client] Response Status: ");
        Console::WriteLine((int)resp3->GetStatusCode());
        Console::Write("[Client] Response Body: '");
        Console::Write(resp3->GetContent()->ReadAsString());
        Console::WriteLine("'");

        Console::WriteLine("\n[Client] Sending PUT request to '/api/users/12345' (Update)...");
        auto content = SmartPointer<StringContent>::NewShared("{\"name\": \"Alice\"}");
        auto resp4 = client.Put("http://127.0.0.1:19099/api/users/12345", content);
        Console::Write("[Client] Response Status: ");
        Console::WriteLine((int)resp4->GetStatusCode());
        Console::Write("[Client] Response Body: '");
        Console::Write(resp4->GetContent()->ReadAsString());
        Console::WriteLine("'");

        Console::WriteLine("\n[Client] Sending DELETE request to '/api/users/12345'...");
        auto resp5 = client.Delete("http://127.0.0.1:19099/api/users/12345");
        Console::Write("[Client] Response Status: ");
        Console::WriteLine((int)resp5->GetStatusCode());
        Console::Write("[Client] Response Body: '");
        Console::Write(resp5->GetContent()->ReadAsString());
        Console::WriteLine("' (Expected empty for 204)");

        // 6. Test Web API Controller endpoints
        Console::WriteLine("\n[Client] Sending GET request to '/api/products' (Controller List)...");
        auto resp6 = client.Get("http://127.0.0.1:19099/api/products");
        Console::Write("[Client] Response Status: ");
        Console::WriteLine((int)resp6->GetStatusCode());
        Console::Write("[Client] Response Body: '");
        Console::Write(resp6->GetContent()->ReadAsString());
        Console::WriteLine("'");

        Console::WriteLine("\n[Client] Sending GET request to '/api/products/default' (Controller Strongly Typed Single)...");
        auto respDefault = client.Get("http://127.0.0.1:19099/api/products/default");
        Console::Write("[Client] Response Status: ");
        Console::WriteLine((int)respDefault->GetStatusCode());
        Console::Write("[Client] Response Body: '");
        Console::Write(respDefault->GetContent()->ReadAsString());
        Console::WriteLine("'");

        Console::WriteLine("\n[Client] Sending GET request to '/api/products/1' (Controller Item)...");
        auto resp7 = client.Get("http://127.0.0.1:19099/api/products/1");
        Console::Write("[Client] Response Status: ");
        Console::WriteLine((int)resp7->GetStatusCode());
        Console::Write("[Client] Response Body: '");
        Console::Write(resp7->GetContent()->ReadAsString());
        Console::WriteLine("'");

        Console::WriteLine("\n[Client] Sending POST request to '/api/products' (Controller Deserialization)...");
        auto productContent = SmartPointer<StringContent>::NewShared("{\"name\":\"Coffee Mug\",\"price\":15}");
        auto resp8 = client.Post("http://127.0.0.1:19099/api/products", productContent);
        Console::Write("[Client] Response Status: ");
        Console::WriteLine((int)resp8->GetStatusCode());
        Console::Write("[Client] Response Body: '");
        Console::Write(resp8->GetContent()->ReadAsString());
        Console::WriteLine("'");

        // 6b. Test strongly-typed RestClient
        Console::WriteLine("\n[Client] Testing strongly-typed RestClient<DemoProduct>...");
        RestClient<DemoProduct> restClient("http://127.0.0.1:19099/api/products");

        // Test GET all
        Console::WriteLine("[RestClient] Fetching all products...");
        auto products = restClient.GetAll();
        
        StringBuilder sbProducts;
        sbProducts.Append("[RestClient] Number of products: ").Append(products.GetCount()).AppendLine();
        for (int i = 0; i < products.GetCount(); ++i) {
            sbProducts.Append("[RestClient]  - ").Append(products[i].Name).Append(" ($").Append(products[i].Price).AppendLine(")");
        }
        Console::Write(sbProducts.ToString());

        // Test GET by ID
        Console::WriteLine("\n[RestClient] Fetching product with ID '1'...");
        auto prod1 = restClient.Get("1");
        
        StringBuilder sbProd;
        sbProd.Append("[RestClient] Retrieved: ").Append(prod1.Name).Append(" ($").Append(prod1.Price).AppendLine(")");
        Console::Write(sbProd.ToString());

        // Test POST new resource
        Console::WriteLine("\n[RestClient] POSTing a new product...");
        DemoProduct newProd{"Toaster Oven", 120};
        String postResult = restClient.Post(newProd);
        
        StringBuilder sbPost;
        sbPost.Append("[RestClient] POST Response: ").AppendLine(postResult);
        Console::Write(sbPost.ToString());

        // Test JWT Authentication using RestClient and HttpClient
        Console::WriteLine("\n[Client] Testing JWT Authentication on '/api/secure'...");
        HttpClient authClient;

        // 1. Try request without token (expect 401)
        auto authResp1 = authClient.Get("http://127.0.0.1:19099/api/secure");
        Console::Write("[Client] (No Token) Status: ");
        Console::WriteLine((int)authResp1->GetStatusCode());

        // 2. Generate valid JWT Token
        DotNetDupe::System::IdentityModel::Tokens::Jwt::JWTToken jwt;
        jwt.GetPayload().Add("sub", "Alice");
        String signedToken = jwt.CreateToken("demo-secret-key-999");

        // 3. Try request with valid token (expect 200)
        authClient.GetDefaultRequestHeaders().Add("Authorization", String("Bearer ") + signedToken);
        auto authResp2 = authClient.Get("http://127.0.0.1:19099/api/secure");
        Console::Write("[Client] (Valid Token) Status: ");
        Console::WriteLine((int)authResp2->GetStatusCode());
        Console::Write("[Client] (Valid Token) Body: '");
        Console::Write(authResp2->GetContent()->ReadAsString());
        Console::WriteLine("'");

        // 4. Try request with invalid secret token (expect 401)
        HttpClient invalidClient;
        String invalidToken = jwt.CreateToken("wrong-secret-key");
        invalidClient.GetDefaultRequestHeaders().Add("Authorization", String("Bearer ") + invalidToken);
        auto authResp3 = invalidClient.Get("http://127.0.0.1:19099/api/secure");
        Console::Write("[Client] (Invalid Secret Token) Status: ");
        Console::WriteLine((int)authResp3->GetStatusCode());

    } catch (const DotNetDupe::System::Exception& ex) {
        Console::Write("[Client Exception] Error: ");
        Console::WriteLine(ex.What());
    }

    // 7. Shut down the server cleanly
    Console::WriteLine("\nStopping WebApplication...");
    app->Stop();
    serverThread.Join();
    Console::WriteLine("WebApplication stopped.");
    Console::WriteLine("=====================================");
}
