#include <Auto.hpp>
#include <Commands.hpp>
#include <Functions.hpp>
#include <Types.hpp>
#include <Register.hpp>

using namespace std::literals;
using namespace xll::literals;

extern xll::AddInManagerInfo dllName([] { return xll::String("Awesome Add-In"); });

auto OnOpen =
    xll::AutoOpen()
    | xll::Before([]() -> tl::expected<std::monostate, std::string> { std::cerr << "xlAutoOpen called...\n"; return std::monostate {};})
    | xll::After([]() -> tl::expected<std::monostate, std::string> { std::cerr << "xlAutoOpen completed\n"; return std::monostate {};})
    | xll::OnError([](const std::string& err) { xll::alert(xll::String(err)); });
XLL_REGISTER(OnOpen);

auto OnAdd =
    xll::AutoAdd()
    | xll::Before([]() -> tl::expected<std::monostate, std::string> { std::cerr << "xlAutoAdd called...\n"; return std::monostate {};})
    | xll::After([]() -> tl::expected<std::monostate, std::string> { std::cerr << "xlAutoAdd completed\n"; return std::monostate {};})
    | xll::OnError([](const std::string& err) { xll::alert(xll::String(err)); });
XLL_REGISTER(OnAdd);

auto OnClose =
    xll::AutoClose()
    | xll::Before([]() -> tl::expected<std::monostate, std::string> { std::cerr << "xlAutoClose called...\n"; return std::monostate {};})
    | xll::After([]() -> tl::expected<std::monostate, std::string> { std::cerr << "xlAutoClose completed\n"; return std::monostate {};})
    | xll::OnError([](const std::string& err) { xll::alert(xll::String(err)); });
XLL_REGISTER(OnClose);

auto OnRemove =
    xll::AutoRemove()
    | xll::Before([]() -> tl::expected<std::monostate, std::string> { std::cerr << "xlAutoRemove called...\n"; return std::monostate {};})
    | xll::After([]() -> tl::expected<std::monostate, std::string> { std::cerr << "xlAutoRemove completed\n"; return std::monostate {};})
    | xll::OnError([](const std::string& err) { xll::alert(xll::String(err)); });
XLL_REGISTER(OnRemove);

auto f =
    xll::Function("MAKE_NUM")
    | xll::Result<xll::Number>()
    | xll::Procedure("MakeNum")
    | xll::Parameter<xll::Number>("召唤😊", "The first argument")
    | xll::Parameter<xll::Number>("активный", "The second argument")
    | xll::ThreadSafe()
    | xll::Category("XLThermo")
    | xll::Description("Some function")
    | xll::Help("https://docs.xlthermo.com");
XLL_REGISTER(f);

XLL_FUNCTION xll::Number* XLLAPI MakeNum(xll::Number const* d, xll::Number const* arr)
{
    auto result = std::make_unique<xll::Number>(42);
    return result | xll::AutoFree();
}
