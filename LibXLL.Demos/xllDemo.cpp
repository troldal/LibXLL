#include <Auto.hpp>
#include <Commands.hpp>
#include <Functional/Transform.hpp>
#include <Functional/TransformError.hpp>
#include <Functions.hpp>
#include <Register.hpp>
#include <Types.hpp>

using namespace std::literals;
using namespace xll::literals;

xll::AddInManagerInfo dllName([] { return xll::String("Awesome Add-In"); });

auto onOpen =
    xll::OnOpen()
    | xll::Before([] { std::cerr << "xlAutoOpen called...\n"; })
    | xll::After([] { std::cerr << "xlAutoOpen completed\n"; })
    | xll::OnError([](const std::string& err) { xll::alert(xll::String(err)); });
XLL_REGISTER(onOpen);

auto onAdd =
    xll::OnAdd()
    | xll::Before([] { std::cerr << "xlAutoAdd called...\n"; })
    | xll::After([] { std::cerr << "xlAutoAdd completed\n"; })
    | xll::OnError([](const std::string& err) { xll::alert(xll::String(err)); });
XLL_REGISTER(onAdd);

auto onClose =
    xll::OnClose()
    | xll::Before([] { std::cerr << "xlAutoClose called...\n"; })
    | xll::After([] { std::cerr << "xlAutoClose completed\n"; })
    | xll::OnError([](const std::string& err) { xll::alert(xll::String(err)); });
XLL_REGISTER(onClose);

auto onRemove =
    xll::OnRemove()
    | xll::Before([] { std::cerr << "xlAutoRemove called...\n"; })
    | xll::After([] { std::cerr << "xlAutoRemove completed\n"; })
    | xll::OnError([](const std::string& err) { xll::alert(xll::String(err)); });
XLL_REGISTER(onRemove);

auto onFree =
    xll::OnFree()
    | xll::Before([] { std::cerr << "xlAutoFree called...\n"; })
    | xll::After([] { std::cerr << "xlAutoFree completed\n"; })
    | xll::OnError([](const std::string& err) { xll::alert(xll::String(err)); });
XLL_REGISTER(onFree);

auto f =
    xll::Function("MAKE.NUM")
    | xll::Result<xll::Number>()
    | xll::Procedure("MakeNum")
    | xll::Parameter<xll::Number>("召唤😊", "The first argument")
    | xll::ThreadSafe()
    | xll::Category("XLThermo")
    | xll::Description("Some function")
    | xll::Help("https://docs.xlthermo.com");
XLL_REGISTER(f);

XLL_FUNCTION xll::Array<xll::Expected<xll::Number>>* XLLAPI MakeNum(xll::Array<xll::Expected<xll::Number>> const* arg)
{
    // auto result = std::make_unique<xll::Number>(42);
    // auto result =
    //     *arg | xll::transform([](xll::Number const& arg) { return xll::String("Valid"); })
    //          | xll::transform_error([](xll::Error const& arg) { return xll::String("Invalid"); });

    auto result = *arg;
    for (auto& elem : result)
        elem = elem | xll::transform([](const xll::Number& num) { return num + 2;});

    return result | xll::AutoFree();
}
