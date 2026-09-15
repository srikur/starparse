#include <print>
#include <starparse/starparse.hpp>

// Models from StarGBC
enum class Model : uint8_t {
    Auto,
    DMG0, DMGA, DMGB, DMGC,
    MGB, SGB, SGB2,
    CGB0, CGBA, CGBB, CGBC, CGBD, CGBE,
    AGB0, AGBA, AGBAE, AGBB, AGBBE,
};

struct Args {
    [[=StarParse::Opt{'m', "Model"}]] Model model{Model::Auto};
};

auto main(int argc, char **argv) -> int {
    constexpr StarParse::Settings settings{.allow_case_insensitivity = true};
    const auto args{StarParse::parse<Args>(argc, argv, settings)};
    std::println("model: {}", std::to_underlying(args->model));
}
